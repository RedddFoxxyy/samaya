/* samaya-timer.c
 *
 * Copyright 2025 Suyog Tandel
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "glib.h"
#include "samaya-timer.h"
#include "samaya-utils.h"


/* ============================================================================
 * Internal Implementation
 * ============================================================================ */

static gboolean tm_run_tick(gpointer user_data);

typedef void (*TmTransitionAction)(TimerPtr self);

typedef struct
{
    TmState current_state;
    TmEvent event;
    TmState next_state;
    TmTransitionAction action;
} TmStateTransition;

static void update_progress(TimerPtr self)
{
    if (self->initial_time_ms > 0) {
        self->timer_progress = (gfloat) self->remaining_time_ms / (gfloat) self->initial_time_ms;
    } else {
        self->timer_progress = 0.0f;
    }
}

static gfloat get_instant_progress(TimerPtr self)
{
    if (self->initial_time_ms <= 0)
        return 0.0f;

    guint64 current_time_us = g_get_monotonic_time();
    guint64 elapsed_since_update_us = guint64_sat_sub(current_time_us, self->last_updated_time_us);

    guint64 elapsed_since_update_ms = elapsed_since_update_us / 1000;

    guint64 real_remaining_ms = guint64_sat_sub(self->remaining_time_ms, elapsed_since_update_ms);

    return (gfloat) real_remaining_ms / (gfloat) self->initial_time_ms;
}

static void notify_time_update(TimerPtr self)
{
    guint64 remaining = self->remaining_time_ms;
    if (self->tm_time_update) {
        self->tm_time_update(&remaining);
    }
}

static void action_start_timer(TimerPtr self)
{
    self->last_updated_time_us = g_get_monotonic_time();

    if (self->tick_source_id > 0) {
        g_source_remove(self->tick_source_id);
    }

    self->tick_source_id = g_timeout_add(1000, tm_run_tick, self);
}

// TODO: This function, tm_run_tick and get_instant_progress all are basically doing the same
// calcualation but code is repeated again and again in all 3. Make a single function for this and
// call that function instead.
static void action_stop_timer(TimerPtr self)
{
    guint64 current_time_us = g_get_monotonic_time();
    guint64 elapsed_time_us = guint64_sat_sub(current_time_us, self->last_updated_time_us);

    gint64 elapsed_time_ms = elapsed_time_us / 1000;
    self->remaining_time_ms = guint64_sat_sub(self->remaining_time_ms, elapsed_time_ms);

    update_progress(self);
    notify_time_update(self);

    if (self->tick_source_id > 0) {
        g_source_remove(self->tick_source_id);
        self->tick_source_id = 0;
    }
}

static void action_reset(TimerPtr self)
{
    action_stop_timer(self);

    self->remaining_time_ms = self->initial_time_ms;
    self->timer_progress = 1.0f;

    notify_time_update(self);
    g_info("Session Reset");
}

static void action_sync_time(TimerPtr self)
{
    self->last_updated_time_us = g_get_monotonic_time();
    action_start_timer(self);
}

// clang-format off
static const TmStateTransition tmStateTransitionMatrix[] = {
    {StIdle,    EvStart,    StRunning,  action_start_timer },
    {StIdle,    EvReset,    StIdle,     action_reset       },
    {StRunning, EvStart,    StRunning,  NULL               },
    {StRunning, EvReset,    StIdle,     action_reset       },
    {StRunning, EvStop,     StPaused,   action_stop_timer  },
    {StPaused,  EvStart,    StRunning,  action_sync_time   },
    {StPaused,  EvStop,     StPaused,   NULL               },
    {StPaused,  EvReset,    StIdle,     action_reset       }
};
// clang-format on

static void tm_process_transition(TimerPtr self, TmEvent event)
{
    TmState current_state = self->tm_state;
    const TmStateTransition *transition = NULL;

    for (guint i = 0; i < G_N_ELEMENTS(tmStateTransitionMatrix); i++) {
        if (tmStateTransitionMatrix[i].current_state == current_state &&
            tmStateTransitionMatrix[i].event == event) {
            transition = &tmStateTransitionMatrix[i];
            break;
        }
    }

    if (transition == NULL) {
        g_warning("Invalid transition. State: %d, Event: %d", current_state, event);
        return;
    }

    self->tm_state = transition->next_state;

    if (transition->action != NULL) {
        transition->action(self);
    }
}

static gboolean tm_run_tick(gpointer timer_ptr)
{
    TimerPtr self = timer_ptr;

    if (self->tm_state != StRunning) {
        self->tick_source_id = 0;
        return G_SOURCE_REMOVE;
    }

    guint64 current_time_us = g_get_monotonic_time();
    guint64 elapsed_time_us = guint64_sat_sub(current_time_us, self->last_updated_time_us);
    self->last_updated_time_us = current_time_us;

    gint64 elapsed_time_ms = elapsed_time_us / 1000;
    self->remaining_time_ms = guint64_sat_sub(self->remaining_time_ms, elapsed_time_ms);

    update_progress(self);
    notify_time_update(self);

    if (self->remaining_time_ms == 0) {
        self->tm_state = StIdle;

        if (self->tm_time_complete) {
            self->tm_time_complete(self);
        }

        if (self->tm_state != StRunning) {
            self->tick_source_id = 0;
        }

        return G_SOURCE_REMOVE;
    }

    return G_SOURCE_CONTINUE;
}

/* ============================================================================
 * Public API
 * ============================================================================ */

TimerPtr tm_new(float duration_minutes, TmCallback time_complete, TmCallback time_update,
                TmCallback event_update)
{
    TimerPtr timer = g_new0(Timer, 1);

    timer->initial_time_ms = (guint64) (duration_minutes * 60 * 1000);
    timer->remaining_time_ms = timer->initial_time_ms;
    timer->timer_progress = 1.0F;

    timer->tm_state = StIdle;

    timer->tm_time_update = time_update;
    timer->tm_time_complete = time_complete;
    timer->tm_event_update = event_update;

    return timer;
}

void tm_free(Timer *self)
{
    if (self->tick_source_id > 0) {
        g_source_remove(self->tick_source_id);
    }

    self->tm_time_update = NULL;
    self->tm_time_complete = NULL;

    g_free(self);
}

void tm_trigger_event(TimerPtr self, TmEvent event)
{
    tm_process_transition(self, event);
}

TmState tm_get_state(TimerPtr self)
{
    return self->tm_state;
}

gfloat tm_get_progress(TimerPtr self)
{
    if (self->tm_state == StRunning) {
        return get_instant_progress(self);
    }
    return self->timer_progress;
}

gint64 tm_get_remaining_time_ms(TimerPtr self)
{
    return self->remaining_time_ms;
}

void tm_set_duration(TimerPtr self, gfloat initial_time_minutes)
{
    self->initial_time_ms = (guint64) (initial_time_minutes * 60 * 1000);
    self->remaining_time_ms = self->initial_time_ms;

    notify_time_update(self);
}
