/* samaya-timer.h
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
#include <gsound-context.h>

typedef struct
{
    GMutex timer_mutex;
    GCond timer_cond;

    gboolean is_running;
    gboolean end_worker;

    // ms suffix means Milli Seconds and us means micro Seconds.
    gint64 initial_time_ms;
    gint64 remaining_time_ms;
    gint64 last_updated_time_us;
    gfloat timer_progress;
    GString *remaining_time_minutes_string;

    GThread *worker_thread;
    guint tick_interval_ms;

    // CallBack API to react on every second tick.
    void (*tm_tick_callback)(void);

    // Callback to a function that will be executed on completion of the set timer.
    void (*tm_completion_callback)(gboolean play_sound);
} Timer;


Timer *tm_get_global_ptr(void);

Timer *tm_init(float duration_minutes, void (*on_finished)(gboolean play_sound),
               void (*timer_tick_callback)(void));

void tm_start(Timer *timer);

void tm_pause(Timer *timer);

void tm_resume(Timer *timer);

void tm_reset(Timer *timer);

void tm_deinit(Timer *timer);

void tm_get_lock(Timer *timer);

void tm_unlock(Timer *timer);

void tm_decrement_remaining_time(Timer *timer, gint64 elapsed_time_ms);

void tm_process_timer_tick(Timer *timer);

gboolean tm_get_is_running(Timer *timer);

gfloat tm_get_progress(Timer *timer);

gchar *tm_get_time_str(Timer *timer);

void tm_set_initial_time(Timer *timer, gfloat initial_time_minutes);

void tm_set_worker_thread(Timer *timer, GThread *timer_thread);
