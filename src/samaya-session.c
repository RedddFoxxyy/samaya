/* samaya-session.c
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

#include "samaya-session.h"
#include "samaya-timer.h"


/* ============================================================================
 * Static Variables
 * ============================================================================ */

// Yeah, I know this is unsafe, anyway....
//
// The lifetime of the SessionManager instance will be the same as that of SAMAYA_APPLICATION,
// so this variable will be valid for the entire duration of SAMAYA_APPLICATION, irrespective of
// whether the window is being displayed or not.
static SessionManagerPtr globalSessionManagerPtr = NULL;


/* ============================================================================
 * Methods for Static Variables
 * ============================================================================ */

SessionManagerPtr sm_get_default(void)
{
    if (globalSessionManagerPtr == NULL) {
        g_critical(
            "Session Manager was accessed but is uninitialised! Trying to use the timer instance "
            "using this pointer is unsafe and will lead to undefined behaviour!");
    }

    return globalSessionManagerPtr;
}


/* ============================================================================
 * Function Definitions
 * ============================================================================ */

static void play_completion_sound(GSoundContext *g_sound_ctx);

static void sm_format_time(SessionManagerPtr self, gint64 timeMS);


/* ============================================================================
 * Internal Implementation
 * ============================================================================ */

static void on_timer_tick(gpointer remaining_time_ms)
{
    SessionManager *session_manager = sm_get_default();
    if (session_manager == NULL) {
        return;
    }

    sm_format_time(session_manager, *(guint64 *) remaining_time_ms);
    g_idle_add(session_manager->sm_timer_tick_callback, session_manager->user_data);
}

static void on_session_complete(gpointer timer)
{
    SessionManagerPtr session_manager = sm_get_default();
    play_completion_sound(session_manager->gsound_ctx);

    switch (session_manager->current_routine) {
        case Working:
            session_manager->sessions_completed++;
            session_manager->total_sessions_counted++;

            if (session_manager->sessions_completed == session_manager->sessions_to_complete) {
                session_manager->current_routine = LongBreak;
                session_manager->sessions_completed = 0;
            } else {
                session_manager->current_routine = ShortBreak;
            }
            break;

        case ShortBreak:
        case LongBreak:
            session_manager->current_routine = Working;
            break;
        default:
            g_critical("Invalid Routine Type! Switching to Working.");
            session_manager->current_routine = Working;
            break;
    }

    sm_set_routine(session_manager->current_routine, session_manager);
}

static void play_completion_sound(GSoundContext *g_sound_ctx)
{
    if (!g_sound_ctx) {
        g_warning("Failed to play completion sound, gSound Context is not set.");
        return;
    }

    GError *error = NULL;
    gboolean ok = gsound_context_play_simple(g_sound_ctx,
                                             NULL, // no cancellable
                                             &error, GSOUND_ATTR_EVENT_ID, "bell-terminal", NULL);

    if (!ok) {
        g_warning("Failed to play sound: %s", error->message);
        g_error_free(error);
    }
}

static void sm_format_time(SessionManagerPtr self, gint64 timeMS)
{
    GString *input_string = self->remaining_time_minutes_string;

    gint64 total_seconds = timeMS / 1000;
    gint64 minutes = total_seconds / 60;
    gint64 seconds = total_seconds % 60;

    g_string_printf(input_string, "%02" G_GINT64_FORMAT ":%02" G_GINT64_FORMAT, minutes, seconds);
}


/* ============================================================================
 * Public API
 * ============================================================================ */

SessionManagerPtr sm_init(guint16 sessions_to_complete, gdouble work_duration,
                          gdouble short_break_duration, gdouble long_break_duration,
                          gboolean (*timer_instance_tick_callback)(gpointer user_data),
                          gpointer user_data)
{
    SessionManagerPtr session_manager = g_new0(SessionManager, 1);

    *session_manager = (SessionManager) {
        .work_duration = work_duration,
        .short_break_duration = short_break_duration,
        .long_break_duration = long_break_duration,
        .current_routine = Working,
        .routines_list = {Working, ShortBreak, LongBreak},

        .sessions_to_complete = sessions_to_complete,
        .sessions_completed = 0,
        .total_sessions_counted = 0,
        .remaining_time_minutes_string = g_string_new(NULL),

        .timer_instance = tm_new(work_duration, on_session_complete, on_timer_tick, NULL),
        .gsound_ctx = gsound_context_new(NULL, NULL),

        .user_data = user_data,

        .sm_timer_tick_callback = timer_instance_tick_callback,
    };
    sm_format_time(session_manager, session_manager->timer_instance->initial_time_ms);
    globalSessionManagerPtr = session_manager;
    return session_manager;
}

void sm_deinit(SessionManager *session_manager)
{
    Timer *timer = session_manager->timer_instance;

    if (timer) {
        tm_free(session_manager->timer_instance);
    }

    globalSessionManagerPtr = NULL;

    g_free(session_manager);
}

void sm_skip_session(void)
{
    on_session_complete(FALSE);
}

void sm_set_work_duration(SessionManagerPtr self, gdouble value)
{
    self->work_duration = (gfloat) value;
    TimerPtr timer = self->timer_instance;
    gboolean is_work_session = (self->current_routine == Working);


    if (is_work_session) {
        tm_trigger_event(timer, EvReset);
        tm_set_duration(timer, self->work_duration);
    }
}

void sm_set_short_break_duration(SessionManagerPtr self, gdouble value)
{
    self->short_break_duration = (gfloat) value;
    Timer *timer = self->timer_instance;
    gboolean is_short_break_session = (self->current_routine == ShortBreak);

    if (is_short_break_session) {
        tm_trigger_event(timer, EvReset);
        tm_set_duration(timer, self->short_break_duration);
    }
}

void sm_set_long_break_duration(SessionManagerPtr self, gdouble value)
{
    self->long_break_duration = (gfloat) value;
    Timer *timer = self->timer_instance;
    gboolean is_long_break_session = (self->current_routine == LongBreak);

    if (is_long_break_session) {
        tm_trigger_event(timer, EvReset);
        tm_set_duration(timer, self->long_break_duration);
    }
}

void sm_set_sessions_to_complete(SessionManager *session_manager, guint16 value)
{
    session_manager->sessions_to_complete = value;
}

void sm_set_routine(RoutineType routine, SessionManager *session_manager)
{
    session_manager->current_routine = routine;

    Timer *timer = session_manager->timer_instance;

    gfloat duration;

    switch (routine) {
        case Working:
            duration = session_manager->work_duration;
            break;
        case ShortBreak:
            duration = session_manager->short_break_duration;
            break;
        case LongBreak:
            duration = session_manager->long_break_duration;
            break;
        default:
            g_critical("Invalid Routine Type! Work duration is being used as default value.");
            duration = session_manager->work_duration;
            break;
    }

    tm_set_duration(timer, duration);
    tm_trigger_event(timer, EvReset);

    if (session_manager->sm_routine_update_callback) {
        g_idle_add(session_manager->sm_routine_update_callback, session_manager->user_data);
    }
}

void sm_set_timer_tick_callback(gboolean (*timer_instance_tick_callback)(gpointer user_data))
{
    SessionManager *session_manager = sm_get_default();
    if (!session_manager) {
        g_critical("Session Manager has not been Initialised yet! Failed to set tick update "
                   "callback.");
        return;
    }

    session_manager->sm_timer_tick_callback = timer_instance_tick_callback;
    g_idle_add(session_manager->sm_timer_tick_callback, session_manager->user_data);
}

void sm_set_timer_tick_callback_with_data(
    gboolean (*timer_instance_tick_callback)(gpointer user_data), gpointer user_data)
{
    SessionManager *session_manager = sm_get_default();
    if (!session_manager) {
        g_critical("Session Manager has not been Initialised yet! Failed to set tick update "
                   "callback and user data.");
        return;
    }

    session_manager->sm_timer_tick_callback = timer_instance_tick_callback;
    session_manager->user_data = user_data;
    g_idle_add(session_manager->sm_timer_tick_callback, session_manager->user_data);
}

void sm_set_routine_update_callback(gboolean (*routine_update_callback)(gpointer))
{
    SessionManager *session_manager = sm_get_default();
    if (session_manager) {
        session_manager->sm_routine_update_callback = routine_update_callback;
    }
}

gdouble sm_get_work_duration(SessionManagerPtr session_manager)
{
    return session_manager->work_duration;
}

gdouble sm_get_short_break_duration(SessionManagerPtr session_manager)
{
    return session_manager->short_break_duration;
}

gdouble sm_get_long_break_duration(SessionManagerPtr session_manager)
{
    return session_manager->long_break_duration;
}

gdouble sm_get_sessions_to_complete(SessionManagerPtr session_manager)
{
    return session_manager->sessions_to_complete;
}

gchar *sm_get_formatted_time(SessionManagerPtr self)
{
    gchar *time_str = self->remaining_time_minutes_string->str;
    return time_str;
}
