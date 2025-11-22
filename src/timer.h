#pragma once

#ifndef TIMER_H
#define TIMER_H

#include <stdbool.h>
#include <glib.h>

typedef struct {
    GMutex timerMutex;

    bool isRunning;

    // MS suffix means Milli Seconds and US means micro Seconds.
    gint64 initialTimeMS;
    gint64 remainingTimeMS;
    gint64 lastUpdatedTimeUS;
    gfloat timerProgress;
    GString *formattedTime;
    gboolean completionAudioPlayed;

    GThread *timerThread;
    guint tickIntervalMS;

    // Pointer to the
    gpointer user_data;

    // CallBack API to react when timer is updated.
    void (*count_update_callback) (gpointer user_data);

    // Callback to function that plays completion sound.
    void (*play_completion_sound)(void);

    // Callback to function that will be executed on completion of the set timer.
    void (*on_finished)(void);

} Timer;

Timer* init_timer(float duration_minutes,
                  void (*play_completion_sound)(void),
                  void (*on_finished)(void));

void timer_start(Timer *timer);
void timer_pause(Timer *timer);
void timer_resume(Timer *timer);
void timer_reset(Timer *timer);

void deinit_timer(Timer *timer);

void lock_timer(Timer *timer);
void unlock_timer(Timer *timer);

void decrement_remaining_time_ms(Timer *timer, gint64 elapsedTimeMS);
gboolean get_is_timer_running(Timer *timer);
gfloat get_timerProgress(Timer *timer);
gchar* get_time_str(Timer *timer);

void run_count_update_callback(Timer *timer, gpointer user_data);

void format_time (GString *inputString, gint64 timeMS);

#endif