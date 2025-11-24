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

#include "samaya-timer.h"
#include "samaya-session.h"

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
	g_critical("Session Manager was accessed but is uninitialised!");
	return GLOBAL_SESSION_MANAGER_PTR;
}


/* ============================================================================
 * Function Definitions
 * ============================================================================ */

static void on_session_completion(void);

static void play_completion_sound(GSoundContext *gSoundCTX);

static void timer_tick_callback(void);

void set_routine(WorkRoutine routine, SessionManager *session_manager);


/* ============================================================================
 * SessionManager Methods
 * ============================================================================ */

SessionManager *init_session_manager(guint16 sessions_to_complete,
                                     void (*timer_instance_tick_callback)(gpointer user_data),
                                     gpointer user_data)
{
	SessionManager *sessionManager = g_new0(SessionManager, 1);

	*sessionManager = (SessionManager)
	{
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

	sessionManager->timer_instance = init_timer(sessionManager->work_duration, on_session_completion, timer_tick_callback);
	sessionManager->timer_instance_tick_callback = timer_instance_tick_callback;

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

static void timer_tick_callback(void)
{
	SessionManager *session_manager = get_active_session_manager();
	if (!session_manager) {
		g_critical("Session Manager has not been Initialised yet! Failed to update timer tick!");
		return;
	}

	session_manager->timer_instance_tick_callback(session_manager->user_data);
}

static void on_session_completion(void)
{
	SessionManager *session_manager = get_active_session_manager();
	play_completion_sound(session_manager->gsound_ctx);

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

	session_manager->timer_instance_tick_callback(session_manager->user_data);
}

static void play_completion_sound(GSoundContext *gSoundCTX)
{
	if (!gSoundCTX) {
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
			break;
		case ShortBreak:
			duration = session_manager->short_break_duration;
			break;
		case LongBreak:
			duration = session_manager->long_break_duration;
			break;
	}

	set_timer_initial_time_minutes(timer, duration);

	update_timer_string_and_run_tick_callback(timer);
}

void set_timer_instance_tick_callback(void (*timer_instance_tick_callback)(gpointer user_data))
{
	SessionManager *session_manager = get_active_session_manager();
	if (!session_manager) {
		g_critical("Session Manager has not been Initialised yet! Failed to set tick update callback.");
		return;
	}

	session_manager->timer_instance_tick_callback = timer_instance_tick_callback;
}


void set_timer_instance_tick_callback_with_data(void (*timer_instance_tick_callback)(gpointer user_data), gpointer user_data)
{
	SessionManager *session_manager = get_active_session_manager();
	if (!session_manager) {
		g_critical("Session Manager has not been Initialised yet! Failed to set tick update callback and user data.");
		return;
	}

	session_manager->timer_instance_tick_callback = timer_instance_tick_callback;
	session_manager->user_data = user_data;
}

