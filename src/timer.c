//
// Created by suyog on 22/11/25.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>      
#include <pthread.h>
#include <sys/time.h>
#include "timer.h"

Timer* init_timer(float duration_minutes,
                  void (*play_completion_sound)(void),
                  void (*on_finished)(void),
                  void (*count_update_callback) (gpointer user_data),
                  gpointer user_data)
{
    Timer *timer = (Timer *) malloc(sizeof(Timer));

    g_mutex_init(&timer->timerMutex);

    timer->initialTimeMS = (int64_t) (duration_minutes * 60 * 1000);
    timer->remainingTimeMS = timer->initialTimeMS;

    timer->timerProgress = 1.0f;
    timer->isRunning = FALSE;
    timer->formattedTime = g_string_new(NULL);
    format_time(timer->formattedTime, timer->initialTimeMS);
    timer->completionAudioPlayed = FALSE;

    timer->count_update_callback = count_update_callback;
    timer->play_completion_sound = play_completion_sound;
    timer->on_finished = on_finished;
    timer->timerThread = NULL;
    timer->tickIntervalMS = 200;
    timer->user_data = user_data;

    return timer;
}

gpointer running_timer_routine(gpointer timerInstance)
{
    gboolean call_play_sound = FALSE;
    gboolean call_on_finished = FALSE;

    Timer *timer = (Timer*) timerInstance;
    if (!timer) return NULL;

    // Initialize last updated time (in us)
    lock_timer(timer);
    timer->lastUpdatedTimeUS = g_get_monotonic_time(); // get current system time
    timer->isRunning = TRUE;
    unlock_timer(timer);

    while (TRUE) {
        g_usleep((gulong)timer->tickIntervalMS * 1000UL);

        lock_timer(timer);

        if (!timer->isRunning) {
            unlock_timer(timer);
            break;
        }

        gint64 currentTimeUS = g_get_monotonic_time();
        gint64 elapsedTimeUS = currentTimeUS - timer->lastUpdatedTimeUS;
        if (elapsedTimeUS < 0) elapsedTimeUS = 0;
        timer->lastUpdatedTimeUS = currentTimeUS;

        gint64 elapsedTimeMS = elapsedTimeUS / 1000;
        decrement_remaining_time_ms(timer, elapsedTimeMS);

        if (timer->remainingTimeMS < 0) timer->remainingTimeMS = 0;

        if (timer->initialTimeMS > 0)
            timer->timerProgress = (gfloat)timer->remainingTimeMS / (gfloat)timer->initialTimeMS;
        else
            timer->timerProgress = 0.0f;

        // Scheduled a callback to play the timer completion audio.
        if (timer->remainingTimeMS <= 1800 && !timer->completionAudioPlayed) {
            timer->completionAudioPlayed = TRUE;
            call_play_sound = TRUE;
        }

        // Timer Finished.
        if (timer->remainingTimeMS == 0) {
            timer->isRunning = FALSE;
            timer->completionAudioPlayed = FALSE;

            // Scheduled a callback to run the 'on_finished' callback.
            call_on_finished = TRUE;
            unlock_timer(timer);
            break;
        }

        format_time(timer->formattedTime, timer->remainingTimeMS);
        run_count_update_callback(timer, timer->user_data);

        unlock_timer(timer);

        if (call_play_sound) {
            call_play_sound = FALSE;
            if (timer->play_completion_sound) timer->play_completion_sound();
        }
    }

    if (call_on_finished) {
        if (timer->on_finished) timer->on_finished();
        call_on_finished = FALSE;
    }

    return NULL;
}

void timer_start(Timer *timer)
{
    if (!timer) return;

    lock_timer(timer);

    if (timer->isRunning || timer->remainingTimeMS <= 0) {
        unlock_timer(timer);
        return;
    }

    timer->lastUpdatedTimeUS = g_get_monotonic_time();
    timer->isRunning = TRUE;
    timer->completionAudioPlayed = FALSE;

    unlock_timer(timer);

    GThread *timerThread = g_thread_new("timer-thread", running_timer_routine, timer);

    if (!timerThread) {
        lock_timer(timer);
        timer->isRunning = FALSE;
        unlock_timer(timer);
        g_warning("timer_start: failed to create thread");
        return;
    }

    lock_timer(timer);
    timer->timerThread = timerThread;
    unlock_timer(timer);

    g_thread_unref(timerThread);
}

void timer_pause(Timer *timer)
{
    lock_timer(timer);
    timer->isRunning = FALSE;
    unlock_timer(timer);
}

void timer_resume(Timer *timer)
{
    lock_timer(timer);
    timer->isRunning = TRUE;
    unlock_timer(timer);
}

void timer_reset(Timer *timer)
{
    lock_timer(timer);
    timer->isRunning = FALSE;
    // GThread *thread_to_join = timer->timerThread;
    timer->timerThread = NULL;
    unlock_timer(timer);

    // if (thread_to_join) {
    //     g_thread_join(thread_to_join);
    // }

    lock_timer(timer);
    timer->remainingTimeMS = timer->initialTimeMS;
    timer->lastUpdatedTimeUS = 0;
    timer->timerProgress = 1.0f;
    format_time(timer->formattedTime, timer->initialTimeMS);
    timer->completionAudioPlayed = FALSE;

    run_count_update_callback(timer, timer->user_data);

    unlock_timer(timer);
}

void deinit_timer(Timer *timer)
{
    lock_timer(timer);
    timer->isRunning = FALSE;
    GThread *thread_to_join = timer->timerThread;
    timer->timerThread = NULL;
    unlock_timer(timer);

    // if (thread_to_join) {
    //     g_thread_join(thread_to_join);
    // }

    g_string_free(timer->formattedTime, TRUE);
    g_mutex_clear(&timer->timerMutex);

    // if (thread_to_join) {
    //     g_thread_join(thread_to_join);
    // }

    free(timer);
}


void lock_timer(Timer *timer)
{
    g_mutex_lock(&timer->timerMutex);
}

void unlock_timer(Timer *timer)
{
    g_mutex_unlock(&timer->timerMutex);
}

void decrement_remaining_time_ms(Timer *timer, gint64 elapsedTimeMS)
{
    timer->remainingTimeMS -= elapsedTimeMS;
}

gboolean get_is_timer_running(Timer *timer)
{
    lock_timer(timer);
    gboolean is_running = timer->isRunning;
    unlock_timer(timer);
    return is_running;
}

gfloat get_timer_progress(Timer *timer)
{
    lock_timer(timer);
    gfloat timerProgress = timer->timerProgress;
    unlock_timer(timer);

    return timerProgress;
}

gchar* get_time_str(Timer *timer)
{
    lock_timer(timer);
    gchar *time_str = timer->formattedTime->str;
    unlock_timer(timer);
    return time_str;
}

void run_count_update_callback(Timer *timer, gpointer user_data)
{
    if (timer->count_update_callback) {
        timer->count_update_callback(timer->user_data);
    }
}

void format_time (GString *inputString, gint64 timeMS)
{
    gint64 total_seconds = timeMS / 1000;
    gint64 minutes       = total_seconds / 60;
    gint64 seconds       = total_seconds % 60;

    g_string_printf (inputString, "%02" G_GINT64_FORMAT ":%02" G_GINT64_FORMAT,
                     minutes, seconds);
}


