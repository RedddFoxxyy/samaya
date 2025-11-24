/* timer.h
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

#include <stdbool.h>
#include <glib.h>
#include <gsound-context.h>

typedef struct
{
	GMutex timer_mutex;

	bool is_running;

	// ms suffix means Milli Seconds and us means micro Seconds.
	gint64 initial_time_ms;
	gint64 remaining_time_ms;
	gint64 last_updated_time_us;
	gfloat timer_progress;
	GString *remaining_time_minutes_string;
	gboolean is_timer_completion_audio_played;

	GThread *worker_thread;
	guint tick_interval_ms;

	gpointer user_data;

	// CallBack API to react when timer is updated.
	void (*count_update_callback)(gpointer user_data);

	// Callback to function that plays completion sound.
	void (*play_completion_sound)(void);

	// Callback to function that will be executed on completion of the set timer.
	void (*on_timer_completion)(void);
} Timer;


Timer *get_active_timer(void);

Timer *init_timer(float duration_minutes,
                  void (*play_completion_sound)(void),
                  void (*on_finished)(void),
                  void (*count_update_callback)(gpointer user_data),
                  gpointer user_data);

void timer_start(Timer *timer);

void timer_pause(Timer *timer);

void timer_resume(Timer *timer);

void timer_reset(Timer *timer);

void deinit_timer(Timer *timer);

void lock_timer(Timer *timer);

void unlock_timer(Timer *timer);

void decrement_remaining_time_ms(Timer *timer, gint64 elapsed_time_ms);

void update_timer_string_and_run_callback(Timer *timer);

gboolean get_is_timer_running(Timer *timer);

gfloat get_timer_progress(Timer *timer);

gchar *get_time_str(Timer *timer);

void set_timer_initial_time_minutes(Timer *timer, gfloat initial_time_minutes);

void set_timer_thread(Timer *timer, GThread *timerThread);

void set_count_update_callback(Timer *timer, void (*count_update_callback)(gpointer user_data));

void set_count_update_callback_with_data(Timer *timer, void (*count_update_callback)(gpointer user_data), gpointer user_data);
