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

#include "samaya-timer.h"
#include "glib.h"
#include "samaya-utils.h"


/* ============================================================================
 * Internal Implementation
 * ============================================================================ */

typedef void (*TmTransitionAction)(TimerPtr self);

typedef struct
{
    TmState current_state;
    TmEvent event;
    TmState next_state;
    TmTransitionAction action;
} TmStateTransition;

static void tm_lock(TimerPtr self)
{
    g_mutex_lock(&self->tm_mutex);
}

static void tm_unlock(TimerPtr self)
{
    g_mutex_unlock(&self->tm_mutex);
}

static void action_sync_time(TimerPtr self)
{
    g_cond_signal(&self->tm_running_cond);
    guint64 remaining_time_ms = self->remaining_time_ms;
    if (self->tm_time_update) {
        self->tm_time_update(&remaining_time_ms);
    }
    self->last_updated_time_us = g_get_monotonic_time();
}

static void action_reset(TimerPtr self)
{
    self->remaining_time_ms = self->initial_time_ms;
    self->timer_progress = 1.0f;
    guint64 remaining_time_ms = self->remaining_time_ms;
    if (self->tm_time_update) {
        self->tm_time_update(&remaining_time_ms);
    }
    g_info("Session Reset");
}

// clang-format off
static const TmStateTransition tmStateTransitionMatrix[] = {
    {StIdle,    EvStart,    StRunning,  action_sync_time },
    {StIdle,    EvReset,    StIdle,     action_sync_time },
    {StRunning, EvStart,    StRunning,  NULL             },
    {StRunning, EvReset,    StIdle,     action_reset     },
    {StRunning, EvStop,     StPaused,   NULL             },
    {StPaused,  EvStart,    StRunning,  action_sync_time },
    {StPaused,  EvStop,     StPaused,   action_sync_time },
    {StPaused,  EvReset,    StIdle,     action_reset     }
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

    if (transition->action != NULL) {
        transition->action(self);
    }

    self->tm_state = transition->next_state;
}

static gboolean tm_run_tick(TimerPtr self)
{
    switch (self->tm_state) {
        default:
        case StExited:
            return FALSE;

        case StIdle:
        case StPaused:
            tm_lock(self);
            g_cond_wait((&self->tm_running_cond), &self->tm_mutex);
            tm_unlock(self);
            return TRUE;

        case StRunning:
            tm_lock(self);

            guint64 current_time_us = (guint64) g_get_monotonic_time();
            guint64 elapsed_time_us = guint64_sat_sub(current_time_us, self->last_updated_time_us);
            self->last_updated_time_us = current_time_us;

            gint64 elapsed_time_ms = elapsed_time_us / 1000;
            self->remaining_time_ms = guint64_sat_sub(self->remaining_time_ms, elapsed_time_ms);

            if (self->initial_time_ms > 0) {
                self->timer_progress =
                    (gfloat) self->remaining_time_ms / (gfloat) self->initial_time_ms;
            } else {
                self->timer_progress = 0.0f;
            }

            gint64 current_remaining_time_ms = self->remaining_time_ms;

            if (self->tm_time_update) {
                self->tm_time_update(&current_remaining_time_ms);
            }

            if (self->remaining_time_ms == 0) {
                self->tm_state = StIdle;
                tm_unlock(self);

                if (self->tm_time_complete) {
                    self->tm_time_complete(self);
                }
                return TRUE;
            }

            tm_unlock(self);
            g_usleep((gulong) self->tm_sleep_time_ms * 1000UL);

            return TRUE;
    }
}

static gpointer tm_loop_entry(gpointer timer_instance)
{
    TimerPtr timer = timer_instance;

    if (timer == NULL) {
        g_critical("Timer has not been initialised, timer thread creation failed.");
        return NULL;
    }

    while (tm_run_tick(timer)) {
    }

    return NULL;
}


/* ============================================================================
 * Public API
 * ============================================================================ */

TimerPtr tm_new(float duration_minutes, TmCallback time_complete, TmCallback time_update,
                TmCallback event_update)
{
    TimerPtr timer = g_new0(Timer, 1);

    g_mutex_init(&timer->tm_mutex);
    g_cond_init(&timer->tm_running_cond);

    timer->tm_sleep_time_ms = 200;
    timer->initial_time_ms = (gint64) (duration_minutes * 60 * 1000);
    timer->remaining_time_ms = timer->initial_time_ms;
    timer->timer_progress = 1.0F;

    timer->tm_state = StIdle;

    timer->tm_machine_thread = g_thread_new("timer-thread", tm_loop_entry, timer);

    timer->tm_time_update = time_update;
    timer->tm_time_complete = time_complete;
    timer->tm_event_update = event_update;

    return timer;
}

void tm_free(Timer *self)
{
    tm_lock(self);

    self->tm_state = StExited;
    g_cond_signal(&self->tm_running_cond);

    self->tm_time_update = NULL;
    self->tm_time_complete = NULL;
    GThread *thread_to_join = self->tm_machine_thread;
    self->tm_machine_thread = NULL;

    tm_unlock(self);

    if (thread_to_join) {
        g_thread_join(thread_to_join);
    }

    g_cond_clear(&self->tm_running_cond);
    g_mutex_clear(&self->tm_mutex);

    g_free(self);
}

void tm_trigger_event(TimerPtr self, TmEvent event)
{
    tm_lock(self);
    tm_process_transition(self, event);
    tm_unlock(self);
}

TmState tm_get_state(TimerPtr self)
{
    tm_lock(self);
    TmState current_state = self->tm_state;
    tm_unlock(self);
    return current_state;
}

gfloat tm_get_progress(TimerPtr self)
{
    tm_lock(self);
    gfloat timer_progress = self->timer_progress;
    tm_unlock(self);

    return timer_progress;
}

gint64 tm_get_remaining_time_ms(TimerPtr self)
{
    tm_lock(self);
    gint64 remaining_time = self->remaining_time_ms;
    tm_unlock(self);
    return remaining_time;
}

void tm_set_duration(TimerPtr self, gfloat initial_time_minutes)
{
    tm_lock(self);
    self->initial_time_ms = (guint64) (initial_time_minutes * 60 * 1000);
    self->remaining_time_ms = self->initial_time_ms;
    guint64 remaining_time_ms = self->remaining_time_ms;
    if (self->tm_time_update) {
        self->tm_time_update(&remaining_time_ms);
    }
    tm_unlock(self);
}
