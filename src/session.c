/* session.c
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

#include "timer.h"
#include "session.h"

#include <stdio.h>

/* ============================================================================
 * Static Variables
 * ============================================================================ */

// Will return NULL if timer is not initialised!
static SessionManager *GLOBAL_SESSION_MANAGER_PTR = NULL;


/* ============================================================================
 * Methods for Static Variables
 * ============================================================================ */

SessionManager *get_active_session_manager(void)
{
	if (GLOBAL_SESSION_MANAGER_PTR) {
		return GLOBAL_SESSION_MANAGER_PTR;
	}
	g_warning("Session Manager was accessed but is uninitialised!");
	return GLOBAL_SESSION_MANAGER_PTR;
}


/* ============================================================================
 * Function Definitions
 * ============================================================================ */

static void on_routine_completion(void);

static void play_completion_sound(void);

void set_routine(WorkRoutine routine, SessionManager *session_manager);


/* ============================================================================
 * SessionManager Methods
 * ============================================================================ */

SessionManager *init_session_manager(guint16 sessions_to_complete, void (*count_update_callback)(gpointer user_data), gpointer user_data)
{
	SessionManager *sessionManager = g_new0(SessionManager, 1);

	*sessionManager = (SessionManager){
		.work_duration = 0.1f,
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

	sessionManager->timer_instance = init_timer(sessionManager->work_duration, play_completion_sound, on_routine_completion,
	                                            count_update_callback, user_data);

	GLOBAL_SESSION_MANAGER_PTR = sessionManager;
	return sessionManager;
}

void deinit_session_manager(SessionManager *session_manager)
{
	Timer *timer = session_manager->timer_instance;

	if (timer) {
		deinit_timer(session_manager->timer_instance);
	}

	GLOBAL_SESSION_MANAGER_PTR = NULL;

	free(session_manager);
}

static void on_routine_completion(void)
{
	SessionManager *session_manager = get_active_session_manager();

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
	set_routine(session_manager->current_routine, session_manager);
}

static void play_completion_sound(void)
{
	GSoundContext *gSoundCTX = NULL;
	SessionManager *session_manager = get_active_session_manager();

	if (session_manager && session_manager->gsound_ctx) {
		gSoundCTX = GLOBAL_SESSION_MANAGER_PTR->gsound_ctx;
	} else {
		g_warning("Failed to play completion sound, gSound Context is not set.");
		return;
	}

	GError *error = NULL;
	gboolean ok = gsound_context_play_simple(
		gSoundCTX,
		NULL, // no cancellable
		&error,
		GSOUND_ATTR_EVENT_ID, "bell-terminal",
		NULL
	);

	if (!ok) {
		g_warning("Failed to play sound: %s", error->message);
		g_error_free(error);
	}
}


void set_routine(WorkRoutine routine, SessionManager *session_manager)
{
	session_manager->current_routine = routine;

	Timer *timer = session_manager->timer_instance;
	timer_reset(timer);

	gfloat duration = 25.0f;

	switch (routine) {
		case Working:
			duration = session_manager->work_duration;
		case ShortBreak:
			duration = session_manager->short_break_duration;
		case LongBreak:
			duration = session_manager->long_break_duration;
	}

	set_timer_initial_time_minutes(timer, duration);

	update_timer_string_and_run_callback(timer);
}
