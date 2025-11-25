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
#include "samaya-timer.h"

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
	g_critical("Timer was accessed but is uninitialised!");
	return NULL;
}


/* ============================================================================
 * Function Definitions
 * ============================================================================ */

static gpointer timer_thread_worker(gpointer timerInstance);

static void format_time(GString *inputString, gint64 timeMS);

static gboolean timer_cleanup_worker(gpointer thread_data);

static void timer_hold_worker(Timer *timer);


/* ============================================================================
 * Timer Methods
 * ============================================================================ */

Timer *init_timer(float duration_minutes,
                  void (*on_finished)(gboolean play_sound),
                  void (*timer_tick_callback)(void))
{
	Timer *timer = g_new0(Timer, 1);

	g_mutex_init(&timer->timer_mutex);
	g_cond_init(&timer->timer_cond);

	timer->initial_time_ms = (int64_t) (duration_minutes * 60 * 1000);
	timer->remaining_time_ms = timer->initial_time_ms;
	timer->timer_progress = 1.0f;

	timer->is_running = FALSE;
	timer->end_worker = FALSE;

	timer->remaining_time_minutes_string = g_string_new(NULL);
	format_time(timer->remaining_time_minutes_string, timer->initial_time_ms);

	timer->timer_tick_callback = timer_tick_callback;
	timer->timer_completion_callback = on_finished;
	timer->worker_thread = NULL;
	timer->tick_interval_ms = 200;

	timer->worker_thread = g_thread_new("timer-thread", timer_thread_worker, timer);

	GLOBAL_TIMER_PTR = timer;

	return timer;
}

static gpointer timer_thread_worker(gpointer timerInstance)
{
	Timer *timer = timerInstance;
	if (!timer) return NULL;

	lock_timer(timer);

	while (TRUE) {
		timer_hold_worker(timer);

		if (timer->end_worker) {
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

		update_timer_string_and_run_tick_callback(timer);

		if (timer->remaining_time_ms == 0) {
			timer->is_running = FALSE;
			unlock_timer(timer);

			if (timer->timer_completion_callback) {
				timer->timer_completion_callback(TRUE);
			}

			lock_timer(timer);
			continue;
		}

		unlock_timer(timer);
		g_usleep((gulong) timer->tick_interval_ms * 1000UL);
		lock_timer(timer);
	}

	unlock_timer(timer);
	return NULL;
}

void timer_start(Timer *timer)
{
	lock_timer(timer);

	if (!timer->is_running) {
		timer->is_running = TRUE;
		g_cond_signal(&timer->timer_cond);
		g_info("Session Started/Resumed");
	}

	unlock_timer(timer);
}

void timer_pause(Timer *timer)
{
	lock_timer(timer);
	timer->is_running = FALSE;
	unlock_timer(timer);

	g_info("Session Paused");
}

void timer_reset(Timer *timer)
{
	lock_timer(timer);

	timer->is_running = FALSE;

	timer->remaining_time_ms = timer->initial_time_ms;
	timer->last_updated_time_us = 0;
	timer->timer_progress = 1.0f;

	update_timer_string_and_run_tick_callback(timer);

	unlock_timer(timer);

	g_info("Session Reset");
}

void deinit_timer(Timer *timer)
{
	lock_timer(timer);

	timer->is_running = FALSE;
	timer->end_worker = TRUE;
	g_cond_signal(&timer->timer_cond);

	timer->timer_tick_callback = NULL;
	GThread *thread_to_join = timer->worker_thread;
	timer->worker_thread = NULL;

	unlock_timer(timer);

	if (thread_to_join) {
		g_thread_join(thread_to_join);
	}

	g_string_free(timer->remaining_time_minutes_string, TRUE);
	g_cond_clear(&timer->timer_cond);
	g_mutex_clear(&timer->timer_mutex);
	GLOBAL_TIMER_PTR = NULL;

	g_free(timer);
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

static void run_timer_tick_callback(Timer *timer)
{
	if (timer->timer_tick_callback) {
		timer->timer_tick_callback();
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

void update_timer_string_and_run_tick_callback(Timer *timer)
{
	run_timer_tick_callback(timer);
	format_time(timer->remaining_time_minutes_string, timer->remaining_time_ms);
}

static void timer_hold_worker(Timer *timer)
{
	while (!timer->is_running && !timer->end_worker) {
		g_cond_wait(&timer->timer_cond, &timer->timer_mutex);

		if (timer->is_running) {
			// Reset the monotonic time so we don't jump ahead.
			timer->last_updated_time_us = g_get_monotonic_time();
		}
	}
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

void set_timer_thread(Timer *timer, GThread *timer_thread)
{
	lock_timer(timer);
	timer->worker_thread = timer_thread;
	unlock_timer(timer);
}



