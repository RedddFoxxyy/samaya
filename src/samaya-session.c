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
#include <stdio.h>
#include "samaya-timer.h"


/* ============================================================================
 * Static Variables
 * ============================================================================ */

// Will return NULL if the timer is not initialised!
static SessionManager *GLOBAL_SESSION_MANAGER_PTR = NULL;


/* ============================================================================
 * Methods for Static Variables
 * ============================================================================ */

SessionManager *sm_get_global_ptr(void)
{
    if (GLOBAL_SESSION_MANAGER_PTR) {
        return GLOBAL_SESSION_MANAGER_PTR;
    }
    g_critical("Session Manager was accessed but is uninitialised!");
    return GLOBAL_SESSION_MANAGER_PTR;
}


/* ============================================================================
 * Function Definitions
 * ============================================================================ */

static void on_session_completion(gboolean play_sound);

static void play_completion_sound(GSoundContext *gSoundCTX);

static void timer_tick_callback(void);


/* ============================================================================
 * SessionManager Methods
 * ============================================================================ */

SessionManager *sm_init(guint16 sessions_to_complete,
                        gboolean (*timer_instance_tick_callback)(gpointer user_data),
                        gpointer user_data)
{
    SessionManager *sessionManager = g_new0(SessionManager, 1);

    *sessionManager = (SessionManager) {
        .work_duration = 25.0f,
        .short_break_duration = 5.0f,
        .long_break_duration = 20.0f,
        .current_routine = Working,
        .routines_list = {Working, ShortBreak, LongBreak},

        .sessions_to_complete = sessions_to_complete,
        .sessions_completed = 0,
        .total_sessions_counted = 0,

        .timer_instance = NULL,
        .gsound_ctx = gsound_context_new(NULL, NULL),

        .user_data = user_data,
    };

    sessionManager->timer_instance =
        tm_init(sessionManager->work_duration, on_session_completion, timer_tick_callback);
    sessionManager->sm_timer_tick_callback = timer_instance_tick_callback;

    GLOBAL_SESSION_MANAGER_PTR = sessionManager;
    return sessionManager;
}

void sm_deinit(SessionManager *session_manager)
{
    Timer *timer = session_manager->timer_instance;

    if (timer) {
        tm_deinit(session_manager->timer_instance);
    }

    GLOBAL_SESSION_MANAGER_PTR = NULL;

    g_free(session_manager);
}

static void timer_tick_callback(void)
{
    SessionManager *session_manager = sm_get_global_ptr();
    if (!session_manager) {
        g_critical("Session Manager has not been Initialised yet! Failed to update timer tick!");
        return;
    }

    g_idle_add(session_manager->sm_timer_tick_callback, session_manager->user_data);
}

static void on_session_completion(gboolean play_sound)
{
    SessionManager *session_manager = sm_get_global_ptr();
    if (play_sound) {
        play_completion_sound(session_manager->gsound_ctx);
    }

    switch (session_manager->current_routine) {
        case Working:
            session_manager->sessions_completed++;
            session_manager->total_sessions_counted++;

            // If this was the last Work session → LongBreak
            if (session_manager->sessions_completed == session_manager->sessions_to_complete) {
                session_manager->current_routine = LongBreak;
                session_manager->sessions_completed = 0;
            } else {
                session_manager->current_routine = ShortBreak;
            }
            break;

        case ShortBreak:
            session_manager->current_routine = Working;
            break;

        case LongBreak:
            session_manager->current_routine = Working;
            break;
    }

    sm_set_routine(session_manager->current_routine, session_manager);
}

void sm_skip_current_session(void)
{
    on_session_completion(FALSE);
}

static void play_completion_sound(GSoundContext *gSoundCTX)
{
    if (!gSoundCTX) {
        g_warning("Failed to play completion sound, gSound Context is not set.");
        return;
    }

    GError *error = NULL;
    gboolean ok = gsound_context_play_simple(gSoundCTX,
                                             NULL, // no cancellable
                                             &error, GSOUND_ATTR_EVENT_ID, "bell-terminal", NULL);

    if (!ok) {
        g_warning("Failed to play sound: %s", error->message);
        g_error_free(error);
    }
}

void sm_set_work_duration(SessionManager *session_manager, gdouble value)
{
    session_manager->work_duration = (gfloat) value;
    Timer *timer = session_manager->timer_instance;

    gboolean is_running = tm_get_is_running(timer);
    gboolean has_started = (timer->remaining_time_ms != timer->initial_time_ms);
    gboolean is_work_session = (session_manager->current_routine == Working);

    if (is_work_session && !is_running && !has_started) {
        tm_set_initial_time(timer, session_manager->work_duration);
        tm_process_timer_tick(timer);
    }
}

void sm_set_short_break_duration(SessionManager *session_manager, gdouble value)
{
    session_manager->short_break_duration = (gfloat) value;
    Timer *timer = session_manager->timer_instance;

    gboolean is_running = tm_get_is_running(timer);
    gboolean has_started = (timer->remaining_time_ms != timer->initial_time_ms);
    gboolean is_short_break_session = (session_manager->current_routine == ShortBreak);

    if (is_short_break_session && !is_running && !has_started) {
        tm_set_initial_time(timer, session_manager->short_break_duration);
        tm_process_timer_tick(timer);
    }
}

void sm_set_long_break_duration(SessionManager *session_manager, gdouble value)
{
    session_manager->long_break_duration = (gfloat) value;
    Timer *timer = session_manager->timer_instance;

    gboolean is_running = tm_get_is_running(timer);
    gboolean has_started = (timer->remaining_time_ms != timer->initial_time_ms);
    gboolean is_long_break_session = (session_manager->current_routine == LongBreak);

    if (is_long_break_session && !is_running && !has_started) {
        tm_set_initial_time(timer, session_manager->long_break_duration);
        tm_process_timer_tick(timer);
    }
}

void sm_set_sessions_to_complete(SessionManager *session_manager, gint value)
{
    session_manager->sessions_to_complete = (guint16) value;
}

void sm_set_routine(RoutineType routine, SessionManager *session_manager)
{
    session_manager->current_routine = routine;

    Timer *timer = session_manager->timer_instance;

    gfloat duration = 25.0f;

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
    }

    tm_set_initial_time(timer, duration);

    tm_reset(timer);

    if (session_manager->sm_routine_update_callback) {
        g_idle_add(session_manager->sm_routine_update_callback, session_manager->user_data);
    }
}

void sm_set_timer_tick_callback(gboolean (*timer_instance_tick_callback)(gpointer user_data))
{
    SessionManager *session_manager = sm_get_global_ptr();
    if (!session_manager) {
        g_critical("Session Manager has not been Initialised yet! Failed to set tick update "
                   "callback.");
        return;
    }

    session_manager->sm_timer_tick_callback = timer_instance_tick_callback;
}

void sm_set_timer_tick_callback_with_data(
    gboolean (*timer_instance_tick_callback)(gpointer user_data), gpointer user_data)
{
    SessionManager *session_manager = sm_get_global_ptr();
    if (!session_manager) {
        g_critical("Session Manager has not been Initialised yet! Failed to set tick update "
                   "callback and user data.");
        return;
    }

    session_manager->sm_timer_tick_callback = timer_instance_tick_callback;
    session_manager->user_data = user_data;
}

void sm_set_routine_update_callback(gboolean (*routine_update_callback)(gpointer))
{
    SessionManager *session_manager = sm_get_global_ptr();
    if (session_manager) {
        session_manager->sm_routine_update_callback = routine_update_callback;
    }
}
