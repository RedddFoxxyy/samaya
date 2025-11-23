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

void on_routine_completion(gpointer sessionManager);

void play_completion_sound(gpointer gSoundCTX);

SessionManager *init_session_manager(guint16 sessionsToComplete, void (*count_update_callback)(gpointer user_data), gpointer user_data)
{
	SessionManager *sessionManager = g_new0(SessionManager, 1);

	*sessionManager = (SessionManager){
		.workDuration = 0.1f,
		.shortBreakDuration = 5.0f,
		.longBreakDuration = 20.0f,
		.currentRoutine = Working,
		.routinesList = {Working, ShortBreak, LongBreak},

		.sessionsToComplete = sessionsToComplete,
		.sessionsCompleted = 0,
		.totalSessionsCounted = 0,

		.timerInstance = NULL,
		.gsoundCTX = gsound_context_new(NULL, NULL),

		.userData = NULL,
	};

	sessionManager->timerInstance = init_timer(sessionManager->workDuration, play_completion_sound, NULL,
	                                           count_update_callback, user_data);
	sessionManager->on_routine_completion = on_routine_completion;

	sessionManager->timerInstance->gSoundCTX = sessionManager->gsoundCTX;

	return sessionManager;
}

void deinit_session_manager(SessionManager *sessionManager)
{
	Timer *timer = sessionManager->timerInstance;

	if (timer) {
		deinit_timer(sessionManager->timerInstance);
	}

	free(sessionManager);
}

void on_routine_completion(gpointer sessionManager)
{
	SessionManager *sManager = sessionManager;

	switch (sManager->currentRoutine) {
		case Working:
			sManager->sessionsCompleted++;
			sManager->totalSessionsCounted++;

			// If this was the last Work session → LongBreak
			if (sManager->sessionsCompleted == sManager->sessionsToComplete) {
				sManager->currentRoutine = LongBreak;
				sManager->sessionsCompleted = 0;
			} else {
				sManager->currentRoutine = ShortBreak;
			}
			break;

		case ShortBreak:
			sManager->currentRoutine = Working;
			break;

		case LongBreak:
			sManager->currentRoutine = Working;
			break;
	}
}

void play_completion_sound(gpointer gSoundCTX)
{
	// GSoundContext *ctx = GSOUND_CONTEXT(gSoundCTX);
	if (!gSoundCTX) {
		g_warning("Failed to play completion sound, gSound Context is not set.");
		return;
	}

	GError *error = NULL;
	gboolean ok = gsound_context_play_simple(
		gSoundCTX,
		NULL, // no cancellable
		&error, // error pointer
		GSOUND_ATTR_EVENT_ID, "bell-terminal",
		NULL
	);

	if (!ok) {
		g_warning("Failed to play sound: %s", error->message);
		g_error_free(error);
	}
}


void set_routine(WorkRoutine routine, SessionManager *sm)
{
	sm->currentRoutine = routine;

	Timer *timer = sm->timerInstance;
	deinit_timer(timer);

	gfloat duration = 25.0f;

	switch (routine) {
		case Working:
			duration = sm->workDuration;
		case ShortBreak:
			duration = sm->shortBreakDuration;
		case LongBreak:
			duration = sm->longBreakDuration;
	}

	sm->timerInstance = init_timer(duration, play_completion_sound, NULL, sm->count_update_callback, sm->userData);
}
