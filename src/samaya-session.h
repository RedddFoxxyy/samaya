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

#include <glib.h>
#include <gsound.h>
#include "samaya-timer.h"

typedef enum
{
    Working,
    ShortBreak,
    LongBreak,
} RoutineType;

typedef struct
{
    gfloat work_duration;
    gfloat short_break_duration;
    gfloat long_break_duration;

    RoutineType current_routine;
    RoutineType routines_list[3];

    guint8 sessions_to_complete;
    guint8 sessions_completed;
    guint64 total_sessions_counted;

    GString *remaining_time_minutes_string;

    TimerPtr timer_instance;
    GSoundContext *gsound_ctx;

    gpointer user_data;

    gboolean (*sm_timer_tick_callback)(gpointer user_data);

    gboolean (*sm_routine_update_callback)(gpointer user_data);
} SessionManager;

typedef SessionManager *SessionManagerPtr;


SessionManagerPtr sm_get_default(void);

SessionManagerPtr sm_init(guint16 sessions_to_complete, gdouble work_duration,
                          gdouble short_break_duration, gdouble long_break_duration,
                          gboolean (*timer_instance_tick_callback)(gpointer user_data),
                          gpointer user_data);

void sm_deinit(SessionManager *session_manager);

void sm_set_work_duration(SessionManagerPtr self, gdouble value);

void sm_set_short_break_duration(SessionManager *session_manager, gdouble value);

void sm_set_long_break_duration(SessionManager *session_manager, gdouble value);

void sm_set_sessions_to_complete(SessionManager *session_manager, guint16 value);

void sm_set_routine(RoutineType routine, SessionManager *session_manager);

void sm_skip_session(void);

void sm_set_timer_tick_callback(gboolean (*timer_instance_tick_callback)(gpointer));

void sm_set_timer_tick_callback_with_data(gboolean (*timer_instance_tick_callback)(gpointer),
                                          gpointer samaya_application_ref);

void sm_set_routine_update_callback(gboolean (*routine_update_callback)(gpointer));

gdouble sm_get_work_duration(SessionManagerPtr session_manager);

gdouble sm_get_short_break_duration(SessionManagerPtr session_manager);

gdouble sm_get_long_break_duration(SessionManagerPtr session_manager);

gdouble sm_get_sessions_to_complete(SessionManagerPtr session_manager);

gchar *sm_get_formatted_time(SessionManagerPtr self);
