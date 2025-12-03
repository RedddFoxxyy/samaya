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

#include "samaya-timer.h"

/* ============================================================================
 * Static Variables
 * ============================================================================ */

// Yeah, I know this is unsafe, anyway....
//
// The lifetime of the timer instance will be the same as that of SAMAYA_APPLICATION,
// so this variable will be valid for the entire duration of SAMAYA_APPLICATION, irrespective of
// wether the window is being displayed or not.
static TimerPtr globalTimerPtr = NULL;


/* ============================================================================
 * Methods for Static Variables
 * ============================================================================ */

// Will return NULL if timer is not initialised!
TimerPtr tm_get_global(void)
{
    if (globalTimerPtr == NULL) {
        g_critical("Timer was accessed but is uninitialised. Trying to use the timer instance "
                   "using this pointer is unsafe and will lead to undefined behaviour!");
    }
    return globalTimerPtr;
}


/* ============================================================================
 * Function Definitions
 * ============================================================================ */

static gpointer tm_run_thread_worker(gpointer timerInstance);

static void tm_update_time_string(GString *inputString, gint64 timeMS);

static void timer_hold_worker(Timer *timer);


/* ============================================================================
 * Timer Methods
 * ============================================================================ */

Timer *tm_init(float duration_minutes, void (*on_finished)(gboolean play_sound),
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
    tm_update_time_string(timer->remaining_time_minutes_string, timer->initial_time_ms);

    timer->tm_tick_callback = timer_tick_callback;
    timer->tm_completion_callback = on_finished;
    timer->worker_thread = NULL;
    timer->tick_interval_ms = 200;

    timer->worker_thread = g_thread_new("timer-thread", tm_run_thread_worker, timer);

    globalTimerPtr = timer;

    return timer;
}

static gpointer tm_run_thread_worker(gpointer timerInstance)
{
    Timer *timer = timerInstance;
    if (!timer)
        return NULL;

    tm_get_lock(timer);

    while (TRUE) {
        timer_hold_worker(timer);

        if (timer->end_worker) {
            break;
        }

        gint64 current_time_us = g_get_monotonic_time();
        gint64 elapsed_time_us = current_time_us - timer->last_updated_time_us;
        if (elapsed_time_us < 0)
            elapsed_time_us = 0;
        timer->last_updated_time_us = current_time_us;

        gint64 elapsed_time_ms = elapsed_time_us / 1000;
        tm_decrement_remaining_time(timer, elapsed_time_ms);

        if (timer->remaining_time_ms < 0)
            timer->remaining_time_ms = 0;

        if (timer->initial_time_ms > 0)
            timer->timer_progress =
                (gfloat) timer->remaining_time_ms / (gfloat) timer->initial_time_ms;
        else
            timer->timer_progress = 0.0f;

        tm_process_timer_tick(timer);

        if (timer->remaining_time_ms == 0) {
            timer->is_running = FALSE;
            tm_unlock(timer);

            if (timer->tm_completion_callback) {
                timer->tm_completion_callback(TRUE);
            }

            tm_get_lock(timer);
            continue;
        }

        tm_unlock(timer);
        g_usleep((gulong) timer->tick_interval_ms * 1000UL);
        tm_get_lock(timer);
    }

    tm_unlock(timer);
    return NULL;
}

void tm_start(Timer *timer)
{
    tm_get_lock(timer);

    if (!timer->is_running) {
        timer->is_running = TRUE;
        g_cond_signal(&timer->timer_cond);
        g_info("Session Started/Resumed");
    }

    tm_unlock(timer);
}

void tm_pause(Timer *timer)
{
    tm_get_lock(timer);
    timer->is_running = FALSE;
    tm_unlock(timer);

    g_info("Session Paused");
}

void tm_reset(Timer *timer)
{
    tm_get_lock(timer);

    timer->is_running = FALSE;

    timer->remaining_time_ms = timer->initial_time_ms;
    timer->last_updated_time_us = 0;
    timer->timer_progress = 1.0f;

    tm_process_timer_tick(timer);

    tm_unlock(timer);

    g_info("Session Reset");
}

void tm_deinit(Timer *timer)
{
    tm_get_lock(timer);

    timer->is_running = FALSE;
    timer->end_worker = TRUE;
    g_cond_signal(&timer->timer_cond);

    timer->tm_tick_callback = NULL;
    GThread *thread_to_join = timer->worker_thread;
    timer->worker_thread = NULL;

    tm_unlock(timer);

    if (thread_to_join) {
        g_thread_join(thread_to_join);
    }

    g_string_free(timer->remaining_time_minutes_string, TRUE);
    g_cond_clear(&timer->timer_cond);
    g_mutex_clear(&timer->timer_mutex);
    globalTimerPtr = NULL;

    g_free(timer);
}


/* ============================================================================
 * Timer Helpers
 * ============================================================================ */

void tm_get_lock(Timer *timer)
{
    g_mutex_lock(&timer->timer_mutex);
}

void tm_unlock(Timer *timer)
{
    g_mutex_unlock(&timer->timer_mutex);
}

void tm_decrement_remaining_time(Timer *timer, gint64 elapsed_time_ms)
{
    timer->remaining_time_ms -= elapsed_time_ms;
}

static void run_timer_tick_callback(Timer *timer)
{
    if (timer->tm_tick_callback) {
        timer->tm_tick_callback();
    }
}

static void tm_update_time_string(GString *inputString, gint64 timeMS)
{
    gint64 total_seconds = timeMS / 1000;
    gint64 minutes = total_seconds / 60;
    gint64 seconds = total_seconds % 60;

    g_string_printf(inputString, "%02" G_GINT64_FORMAT ":%02" G_GINT64_FORMAT, minutes, seconds);
}

void tm_process_timer_tick(Timer *timer)
{
    run_timer_tick_callback(timer);
    tm_update_time_string(timer->remaining_time_minutes_string, timer->remaining_time_ms);
}

static void timer_hold_worker(Timer *timer)
{
    while (!timer->is_running && !timer->end_worker) {
        g_cond_wait(&timer->timer_cond, &timer->timer_mutex);

        if (timer->is_running) {
            // NOTE: Reset the monotonic time so we don't jump ahead.
            timer->last_updated_time_us = g_get_monotonic_time();
        }
    }
}


/* ============================================================================
 * Timer getters
 * ============================================================================ */

gboolean tm_get_is_running(Timer *timer)
{
    tm_get_lock(timer);
    gboolean is_running = timer->is_running;
    tm_unlock(timer);
    return is_running;
}

gfloat tm_get_progress(Timer *timer)
{
    tm_get_lock(timer);
    gfloat timer_progress = timer->timer_progress;
    tm_unlock(timer);

    return timer_progress;
}

gchar *tm_get_time_str(Timer *timer)
{
    tm_get_lock(timer);
    gchar *time_str = timer->remaining_time_minutes_string->str;
    tm_unlock(timer);
    return time_str;
}


/* ============================================================================
 * Timer setters
 * ============================================================================ */

void tm_set_initial_time(Timer *timer, gfloat initial_time_minutes)
{
    tm_get_lock(timer);
    timer->initial_time_ms = (int64_t) (initial_time_minutes * 60 * 1000);
    timer->remaining_time_ms = timer->initial_time_ms;
    tm_unlock(timer);
}

void tm_set_worker_thread(Timer *timer, GThread *timer_thread)
{
    tm_get_lock(timer);
    timer->worker_thread = timer_thread;
    tm_unlock(timer);
}
