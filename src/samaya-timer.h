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

typedef enum
{
    StIdle,
    StRunning,
    StPaused,
    StExited
} TmState;

typedef enum
{
    EvStart,
    EvStop,
    EvReset,
} TmEvent;

typedef struct Timer Timer;
typedef Timer *TimerPtr;

typedef void (*TmCallback)(gpointer callback_data);

struct Timer
{
    GMutex tm_mutex;

    TmState tm_state;
    GCond tm_running_cond;

    guint64 initial_time_ms;
    guint64 remaining_time_ms;
    guint64 last_updated_time_us;

    gfloat timer_progress;

    GThread *tm_machine_thread;
    guint32 tm_sleep_time_ms;

    TmCallback tm_time_update;
    TmCallback tm_time_complete;
    TmCallback tm_event_update;
};

/*  Constructs a new instance of the timer on the heap and returns a pointer to it.

    Timer instance constructed using this function should be de-initialised using tm_free, or else
    will leak memory.
*/
TimerPtr tm_new(float duration_minutes, TmCallback time_complete, TmCallback time_update,
                TmCallback event_update);

// De-initialises the timer and frees the allocated memory.
void tm_free(TimerPtr self);

// Handles external timer state events.
void tm_trigger_event(TimerPtr timer, TmEvent event);

// Get the current running state of the Timer.
TmState tm_get_state(TimerPtr timer);

// Get the progress of the timer, ( 0 means timer has finished, 1 means timer has not started).
gfloat tm_get_progress(TimerPtr self);

// Get the remaining time for the timer to complete.
gint64 tm_get_remaining_time_ms(TimerPtr self);

// Sets the duration the timer will tick.
void tm_set_duration(TimerPtr self, gfloat initial_time_minutes);
