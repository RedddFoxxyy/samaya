/* timer.c
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include "timer.h"

/* ============================================================================
 * Static Variables
 * ============================================================================ */

// Yeah, I know this is unsafe, anyway....
// Also, maybe I should use a mutex to make this thread safe? or maybe not
// because the timer struct itself has a mutex?
static Timer *GLOBAL_TIMER_PTR = NULL;


/* ============================================================================
 * Methods for Static Variables
 * ============================================================================ */

// Will return NULL if timer is not initialised!
Timer *get_active_timer(void)
{
	if (GLOBAL_TIMER_PTR) {
		return GLOBAL_TIMER_PTR;
	}
	g_warning("Timer was accessed but is uninitialised!");
	return NULL;
}


/* ============================================================================
 * Function Definitions
 * ============================================================================ */

static void format_time(GString *inputString, gint64 timeMS);


/* ============================================================================
 * Timer Methods
 * ============================================================================ */

Timer *init_timer(float duration_minutes,
                  void (*play_completion_sound)(void),
                  void (*on_finished)(void),
                  void (*count_update_callback)(gpointer user_data),
                  gpointer user_data)
{
	Timer *timer = g_new0(Timer, 1);

	g_mutex_init(&timer->timer_mutex);

	timer->initial_time_ms = (int64_t) (duration_minutes * 60 * 1000);
	timer->remaining_time_ms = timer->initial_time_ms;

	timer->timer_progress = 1.0f;
	timer->is_running = FALSE;
	timer->remaining_time_minutes_string = g_string_new(NULL);
	format_time(timer->remaining_time_minutes_string, timer->initial_time_ms);
	timer->is_timer_completion_audio_played = FALSE;

	timer->count_update_callback = count_update_callback;
	timer->play_completion_sound = play_completion_sound;
	timer->on_timer_completion = on_finished;
	timer->worker_thread = NULL;
	timer->tick_interval_ms = 200;
	timer->user_data = user_data;

	GLOBAL_TIMER_PTR = timer;

	return timer;
}

static gpointer running_timer_routine(gpointer timerInstance)
{
	gboolean call_play_sound = FALSE;
	gboolean call_on_finished = FALSE;

	Timer *timer = (Timer *) timerInstance;
	if (!timer) return NULL;

	// Initialize last updated time (in us)
	lock_timer(timer);
	timer->last_updated_time_us = g_get_monotonic_time(); // get current system time
	timer->is_running = TRUE;
	unlock_timer(timer);

	while (TRUE) {
		lock_timer(timer);

		if (!timer->is_running) {
			unlock_timer(timer);
			break;
		}

		gint64 currentTimeUS = g_get_monotonic_time();
		gint64 elapsedTimeUS = currentTimeUS - timer->last_updated_time_us;
		if (elapsedTimeUS < 0) elapsedTimeUS = 0;
		timer->last_updated_time_us = currentTimeUS;

		gint64 elapsedTimeMS = elapsedTimeUS / 1000;
		decrement_remaining_time_ms(timer, elapsedTimeMS);

		if (timer->remaining_time_ms < 0) timer->remaining_time_ms = 0;

		if (timer->initial_time_ms > 0)
			timer->timer_progress = (gfloat) timer->remaining_time_ms / (gfloat) timer->initial_time_ms;
		else
			timer->timer_progress = 0.0f;

		// Scheduled a callback to play the timer completion audio.
		if (timer->remaining_time_ms <= 400 && !timer->is_timer_completion_audio_played) {
			timer->is_timer_completion_audio_played = TRUE;
			call_play_sound = TRUE;
		}

		// Timer Finished.
		if (timer->remaining_time_ms == 0) {
			timer->is_running = FALSE;
			timer->is_timer_completion_audio_played = FALSE;

			// Scheduled a callback to run the 'on_finished' callback.
			call_on_finished = TRUE;
			unlock_timer(timer);
			break;
		}

		update_timer_string_and_run_callback(timer);

		unlock_timer(timer);

		if (call_play_sound) {
			call_play_sound = FALSE;
			if (timer->play_completion_sound) timer->play_completion_sound();
		}

		g_usleep((gulong) timer->tick_interval_ms * 1000UL);
	}

	if (call_on_finished) {
		update_timer_string_and_run_callback(timer);
		if (timer->on_timer_completion) timer->on_timer_completion();
		call_on_finished = FALSE;
	}

	return NULL;
}

void timer_start(Timer *timer)
{
	if (!timer) return;

	lock_timer(timer);

	if (timer->is_running || timer->remaining_time_ms <= 0) {
		unlock_timer(timer);
		return;
	}

	timer->last_updated_time_us = g_get_monotonic_time();
	timer->is_running = TRUE;
	timer->is_timer_completion_audio_played = FALSE;

	unlock_timer(timer);

	GThread *timerThread = g_thread_new("timer-thread", running_timer_routine, timer);

	if (!timerThread) {
		lock_timer(timer);
		timer->is_running = FALSE;
		unlock_timer(timer);
		g_warning("timer_start: failed to create thread");
		return;
	}

	set_timer_thread(timer, timerThread);
}

void timer_pause(Timer *timer)
{
	lock_timer(timer);
	timer->is_running = FALSE;
	unlock_timer(timer);
}

void timer_resume(Timer *timer)
{
	lock_timer(timer);
	timer->is_running = TRUE;
	unlock_timer(timer);
}

void timer_reset(Timer *timer)
{
	lock_timer(timer);

	timer->is_running = FALSE;
	GThread *timer_running_thread = timer->worker_thread;
	timer->worker_thread = NULL;
	timer->remaining_time_ms = timer->initial_time_ms;
	timer->last_updated_time_us = 0;
	timer->timer_progress = 1.0f;
	timer->is_timer_completion_audio_played = FALSE;

	update_timer_string_and_run_callback(timer);

	unlock_timer(timer);

	if (timer_running_thread) {
		if (timer_running_thread != g_thread_self()) {
			g_thread_join(timer_running_thread);
		} else {
			g_thread_unref(timer_running_thread);
		}
	}
}

// This function should not be called by the thread running the timer!
void deinit_timer(Timer *timer)
{
	lock_timer(timer);
	timer->is_running = FALSE;
	timer->count_update_callback = NULL;
	timer->user_data = NULL;
	GThread *thread_to_join = timer->worker_thread;
	timer->worker_thread = NULL;
	unlock_timer(timer);

	if (thread_to_join) {
		g_thread_join(thread_to_join);
	}

	g_string_free(timer->remaining_time_minutes_string, TRUE);
	g_mutex_clear(&timer->timer_mutex);
	GLOBAL_TIMER_PTR = NULL;

	free(timer);
}


/* ============================================================================
 * Timer Helpers
 * ============================================================================ */

void lock_timer(Timer *timer)
{
	g_mutex_lock(&timer->timer_mutex);
}

void unlock_timer(Timer *timer)
{
	g_mutex_unlock(&timer->timer_mutex);
}

void decrement_remaining_time_ms(Timer *timer, gint64 elapsed_time_ms)
{
	timer->remaining_time_ms -= elapsed_time_ms;
}

void run_count_update_callback(Timer *timer)
{
	if (timer->count_update_callback) {
		timer->count_update_callback(timer->user_data);
	}
}

static void format_time(GString *inputString, gint64 timeMS)
{
	gint64 total_seconds = timeMS / 1000;
	gint64 minutes = total_seconds / 60;
	gint64 seconds = total_seconds % 60;

	g_string_printf(inputString, "%02" G_GINT64_FORMAT ":%02" G_GINT64_FORMAT,
	                minutes, seconds);
}

void update_timer_string_and_run_callback(Timer *timer)
{
	run_count_update_callback(timer);
	format_time(timer->remaining_time_minutes_string, timer->remaining_time_ms);
}


/* ============================================================================
 * Timer getters
 * ============================================================================ */

gboolean get_is_timer_running(Timer *timer)
{
	lock_timer(timer);
	gboolean is_running = timer->is_running;
	unlock_timer(timer);
	return is_running;
}

gfloat get_timer_progress(Timer *timer)
{
	lock_timer(timer);
	gfloat timerProgress = timer->timer_progress;
	unlock_timer(timer);

	return timerProgress;
}

gchar *get_time_str(Timer *timer)
{
	lock_timer(timer);
	gchar *time_str = timer->remaining_time_minutes_string->str;
	unlock_timer(timer);
	return time_str;
}


/* ============================================================================
 * Timer setters
 * ============================================================================ */

void set_timer_initial_time_minutes(Timer *timer, gfloat initial_time_minutes)
{
	lock_timer(timer);
	timer->initial_time_ms = (int64_t) (initial_time_minutes * 60 * 1000);
	timer->remaining_time_ms = timer->initial_time_ms;
	unlock_timer(timer);
}

void set_timer_thread(Timer *timer, GThread *timerThread)
{
	lock_timer(timer);
	timer->worker_thread = timerThread;
	unlock_timer(timer);
}

void set_count_update_callback(Timer *timer, void (*count_update_callback)(gpointer user_data))
{
	timer->count_update_callback = count_update_callback;
}

void set_count_update_callback_with_data(Timer *timer, void (*count_update_callback)(gpointer user_data), gpointer user_data)
{
	timer->count_update_callback = count_update_callback;
	timer->user_data = user_data;
}




