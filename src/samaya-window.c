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
#include <math.h>
#include "samaya-application.h"

struct _SamayaWindow
{
	AdwApplicationWindow parent_instance;

	GtkLabel *timer_label;
	GtkButton *start_button;
	GtkButton *reset_button;
	GtkDrawingArea *progress_circle;
};

G_DEFINE_FINAL_TYPE(SamayaWindow, samaya_window, ADW_TYPE_APPLICATION_WINDOW)

/* ============================================================================
 * Function Definitions
 * ============================================================================ */

static void
draw_progress_circle(GtkDrawingArea *area,
                     cairo_t *cr,
                     int width,
                     int height,
                     gpointer user_data);

/* ============================================================================
 * Timer Helpers
 * ============================================================================ */

static Timer *
get_timer(SamayaWindow *self)
{
	SamayaApplication *app = SAMAYA_APPLICATION(gtk_window_get_application(GTK_WINDOW(self)));
	return samaya_application_get_timer(app);
}


/* ============================================================================
 * UI Actions
 * ============================================================================ */

static gboolean
update_timer_label(gpointer user_data)
{
	SamayaApplication *app = SAMAYA_APPLICATION(user_data);
	GtkWindow *window = gtk_application_get_active_window(GTK_APPLICATION(app));
	SamayaWindow *self = SAMAYA_WINDOW(window);

	Timer *timer = get_timer(self);

	if (timer) {
		gtk_label_set_text(self->timer_label, get_time_str(timer));
		gtk_widget_queue_draw(GTK_WIDGET(self->progress_circle));
	}

	return G_SOURCE_REMOVE;
}

static void
schedule_timer_label_update(gpointer user_data)
{
	// As we cannot touch GtkWidgets here. We schedule it for the main thread...
	g_idle_add(update_timer_label, user_data);
}

static void
on_press_start(GtkWidget *widget,
               const char *action_name,
               GVariant *param)
{
	SamayaWindow *self = SAMAYA_WINDOW(widget);

	Timer *timer = get_timer(self);

	gboolean is_running = get_is_timer_running(timer);

	if (is_running) {
		timer_pause(timer);
		gtk_button_set_label(self->start_button, "Resume");
		gtk_widget_remove_css_class(GTK_WIDGET(self->start_button), "warning");
		gtk_widget_add_css_class(GTK_WIDGET(self->start_button), "suggested-action");
	} else {
		timer_start(timer);
		gtk_button_set_label(self->start_button, "Pause");
		gtk_widget_remove_css_class(GTK_WIDGET(self->start_button), "suggested-action");
		gtk_widget_remove_css_class(GTK_WIDGET(self->start_button), "warning");
	}
}

static void
on_press_reset(GtkWidget *widget,
               const char *action_name,
               GVariant *param)
{
	SamayaWindow *self = SAMAYA_WINDOW(widget);

	timer_reset(get_timer(self));

	gtk_button_set_label(self->start_button, "Start");
	gtk_widget_remove_css_class(GTK_WIDGET(self->start_button), "warning");
	gtk_widget_add_css_class(GTK_WIDGET(self->start_button), "suggested-action");
}

/* ============================================================================
 * Rendering Functions
 * ============================================================================ */

static void
draw_progress_circle(GtkDrawingArea *area,
                     cairo_t *cr,
                     int width,
                     int height,
                     gpointer user_data)
{
	SamayaWindow *self = SAMAYA_WINDOW(user_data);

	double center_x = width / 2.0;
	double center_y = height / 2.0;
	double radius = MIN(width, height) / 2.0 - 5;

	gfloat progress = get_timer_progress(get_timer(self));

	cairo_set_line_width(cr, 10.0);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

	cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 0.3);
	cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
	cairo_stroke(cr);

	cairo_set_source_rgb(cr, 0.2, 0.6, 1.0);
	double start_angle = -M_PI / 2;
	double end_angle = start_angle + (2 * M_PI * progress);
	cairo_arc(cr, center_x, center_y, radius, start_angle, end_angle);
	cairo_stroke(cr);
}

/* ============================================================================
 * Samaya Window GObject Methods
 * ============================================================================ */

static void
samaya_window_realize(GtkWidget *widget)
{
	SamayaWindow *self = SAMAYA_WINDOW(widget);

	GTK_WIDGET_CLASS(samaya_window_parent_class)->realize(widget);

	Timer *timer = get_timer(self);
	set_count_update_callback(timer, schedule_timer_label_update);
	gtk_label_set_text(self->timer_label, timer->formattedTime->str);
}

static void
samaya_window_class_init(SamayaWindowClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	widget_class->realize = samaya_window_realize;

	gtk_widget_class_set_template_from_resource(widget_class, "/io/github/redddfoxxyy/samaya/samaya-window.ui");
	gtk_widget_class_bind_template_child(widget_class, SamayaWindow, timer_label);
	gtk_widget_class_bind_template_child(widget_class, SamayaWindow, start_button);
	gtk_widget_class_bind_template_child(widget_class, SamayaWindow, reset_button);
	gtk_widget_class_bind_template_child(widget_class, SamayaWindow, progress_circle);

	gtk_widget_class_install_action(widget_class, "win.start-timer", NULL, on_press_start);
	gtk_widget_class_install_action(widget_class, "win.reset-timer", NULL, on_press_reset);
}

static void
samaya_window_init(SamayaWindow *self)
{
	gtk_widget_init_template(GTK_WIDGET(self));

	gtk_drawing_area_set_draw_func(self->progress_circle, draw_progress_circle, self, NULL);
}
