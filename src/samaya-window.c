/* samaya-window.c
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

#include "config.h"

#include "samaya-window.h"
#include "timer.h"

struct _SamayaWindow
{
	AdwApplicationWindow  parent_instance;

    GtkLabel   *timer_label;
    GtkButton  *start_button;
    GtkButton  *reset_button;

    Timer      *timer;
};

G_DEFINE_FINAL_TYPE (SamayaWindow, samaya_window, ADW_TYPE_APPLICATION_WINDOW)

static gboolean
update_timer_label(gpointer user_data)
{
    SamayaWindow *self = SAMAYA_WINDOW (user_data);

    gtk_label_set_text(self->timer_label, get_time_str(self->timer));

    return G_SOURCE_REMOVE;
}

static void
schedule_timer_label_update(gpointer user_data)
{
    // As we cannot touch GtkWidgets here. We schedule it for the main thread...
    g_idle_add(update_timer_label, user_data);
}

static void
on_press_start (GtkWidget *widget,
                const char *action_name,
                GVariant *param)
{
    SamayaWindow *self = SAMAYA_WINDOW (widget);

    gboolean is_running = get_is_timer_running(self->timer);

    if (is_running) {
        timer_pause(self->timer);
        gtk_button_set_label(self->start_button, "Resume");
        gtk_widget_remove_css_class(GTK_WIDGET(self->start_button), "destructive-action");
        gtk_widget_add_css_class(GTK_WIDGET(self->start_button), "suggested-action");
    } else {
        timer_start(self->timer);
        gtk_button_set_label(self->start_button, "Pause");
        gtk_widget_remove_css_class(GTK_WIDGET(self->start_button), "suggested-action");
        gtk_widget_add_css_class(GTK_WIDGET(self->start_button), "destructive-action");
    }
}

static void
on_press_reset (GtkWidget *widget,
                const char *action_name,
                GVariant *param)
{
    SamayaWindow *self = SAMAYA_WINDOW (widget);

    timer_reset(self->timer);

    // Reset UI state
    gtk_button_set_label(self->start_button, "Start");
    gtk_widget_remove_css_class(GTK_WIDGET(self->start_button), "destructive-action");
    gtk_widget_add_css_class(GTK_WIDGET(self->start_button), "suggested-action");
}

static void
samaya_window_dispose (GObject *object)
{
    SamayaWindow *self = SAMAYA_WINDOW (object);

    if (self->timer) {
        deinit_timer(self->timer);
        self->timer = NULL;
    }

    G_OBJECT_CLASS (samaya_window_parent_class)->dispose (object);
}

static void
samaya_window_class_init (SamayaWindowClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
    GObjectClass   *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = samaya_window_dispose;

    gtk_widget_class_set_template_from_resource (widget_class, "/io/github/redddfoxxyy/samaya/samaya-window.ui");
    gtk_widget_class_bind_template_child (widget_class, SamayaWindow, timer_label);
    gtk_widget_class_bind_template_child (widget_class, SamayaWindow, start_button);
    gtk_widget_class_bind_template_child (widget_class, SamayaWindow, reset_button);

    gtk_widget_class_install_action (widget_class, "win.start-timer", NULL, on_press_start);
    gtk_widget_class_install_action (widget_class, "win.reset-timer", NULL, on_press_reset);
}

static void
samaya_window_init (SamayaWindow *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    self->timer = init_timer(25.0f, NULL, NULL);
    self->timer->count_update_callback = schedule_timer_label_update;
    self->timer->user_data = self;

    gtk_label_set_text(self->timer_label, self->timer->formattedTime->str);
}
