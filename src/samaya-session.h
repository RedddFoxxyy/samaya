/* samaya-session.h
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

#pragma once

#include "samaya-timer.h"
#include <glib.h>
#include <gsound.h>

typedef enum
{
	Working,
	ShortBreak,
	LongBreak,
} WorkRoutine;

typedef struct
{
	gfloat work_duration;
	gfloat short_break_duration;
	gfloat long_break_duration;

	WorkRoutine current_routine;
	WorkRoutine routines_list[3];

	guint16 sessions_to_complete;
	guint16 sessions_completed;
	guint64 total_sessions_counted;

	Timer *timer_instance;
	GSoundContext *gsound_ctx;

	gpointer user_data;

	gboolean (*timer_instance_tick_callback)(gpointer user_data);

	gboolean (*routine_update_callback)(gpointer user_data);
} SessionManager;

SessionManager *get_active_session_manager(void);

SessionManager *init_session_manager(guint16 sessions_to_complete, gboolean (*timer_instance_tick_callback)(gpointer), gpointer user_data);

void deinit_session_manager(SessionManager *session_manager);

void session_manager_set_work_duration(SessionManager *session_manager, gdouble value);

void session_manager_set_short_break_duration(SessionManager *session_manager, gdouble value);

void session_manager_set_long_break_duration(SessionManager *session_manager, gdouble value);

void session_manager_set_sessions_to_complete(SessionManager *session_manager, gint value);

void session_manager_set_routine(WorkRoutine routine, SessionManager *session_manager);

void session_manager_skip_current_session(void);

void set_timer_instance_tick_callback(gboolean (*timer_instance_tick_callback)(gpointer));

void set_timer_instance_tick_callback_with_data(gboolean (*timer_instance_tick_callback)(gpointer), gpointer user_data);

void set_routine_update_callback(gboolean (*routine_update_callback)(gpointer));

