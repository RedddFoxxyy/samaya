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
#include "samaya-timer.h"
#include <math.h>
#include "samaya-application.h"
#include "samaya-session.h"

struct _SamayaWindow
{
	AdwApplicationWindow parent_instance;

	GtkBox *routine_switch_box;
	AdwToggleGroup *routine_toggle_group;

	GtkLabel *timer_label;
	GtkDrawingArea *progress_circle;

	GtkButton *start_button;
	GtkButton *reset_button;
};

G_DEFINE_FINAL_TYPE(SamayaWindow, samaya_window, ADW_TYPE_APPLICATION_WINDOW)

/* ============================================================================
 * Function Definitions
 * ============================================================================ */

static void draw_progress_circle(GtkDrawingArea *area,
                                 cairo_t *cr,
                                 int width,
                                 int height,
                                 gpointer user_data);

static void update_secondary_button_mode(SamayaWindow *self, gboolean is_running);

/* ============================================================================
 * Timer Helpers
 * ============================================================================ */

static Timer *get_timer(SamayaWindow *self)
{
	SamayaApplication *app = SAMAYA_APPLICATION(gtk_window_get_application(GTK_WINDOW(self)));
	return samaya_application_get_timer(app);
}


/* ============================================================================
 * UI Actions
 * ============================================================================ */

static void on_routine_changed(AdwToggleGroup *toggle_group,
                               GParamSpec *pspec,
                               gpointer user_data)
{
	SamayaWindow *self = SAMAYA_WINDOW(user_data);
	const char *active_name = adw_toggle_group_get_active_name(toggle_group);

	SessionManager *session_manager = get_active_session_manager();
	WorkRoutine routine;
	if (g_strcmp0(active_name, "pomodoro") == 0) {
		routine = Working;
	} else if (g_strcmp0(active_name, "short-break") == 0) {
		routine = ShortBreak;
	} else if (g_strcmp0(active_name, "long-break") == 0) {
		routine = LongBreak;
	} else {
		return;
	}

	set_routine(routine, session_manager);

	gtk_button_set_label(self->start_button, "Start");
	gtk_widget_remove_css_class(GTK_WIDGET(self->start_button), "warning");
	gtk_widget_add_css_class(GTK_WIDGET(self->start_button), "suggested-action");
}

static gboolean update_timer_label(gpointer user_data)
{
	SamayaApplication *app = SAMAYA_APPLICATION(user_data);
	GtkWindow *window = gtk_application_get_active_window(GTK_APPLICATION(app));
	SamayaWindow *self = SAMAYA_WINDOW(window);

	Timer *timer = get_timer(self);

	if (timer) {
		gtk_label_set_text(self->timer_label, get_time_str(timer));
		gtk_widget_queue_draw(GTK_WIDGET(self->progress_circle));
	}

	if (!get_is_timer_running(timer)) {
		gtk_button_set_label(self->start_button, "Start");
		gtk_widget_remove_css_class(GTK_WIDGET(self->start_button), "warning");
		gtk_widget_add_css_class(GTK_WIDGET(self->start_button), "suggested-action");

		update_secondary_button_mode(self, FALSE);
	}

	return G_SOURCE_REMOVE;
}

static gboolean update_routine_toggle_switch(gpointer user_data)
{
	SamayaApplication *app = SAMAYA_APPLICATION(user_data);
	GtkWindow *window = gtk_application_get_active_window(GTK_APPLICATION(app));
	SamayaWindow *self = SAMAYA_WINDOW(window);

	WorkRoutine new_routine = samaya_application_get_session_manager(app)->current_routine;

	const char *target_name = NULL;

	switch (new_routine) {
		case Working: target_name = "pomodoro";
			break;
		case ShortBreak: target_name = "short-break";
			break;
		case LongBreak: target_name = "long-break";
			break;
	}

	if (target_name) {
		g_signal_handlers_block_by_func(self->routine_toggle_group, on_routine_changed, self);
		adw_toggle_group_set_active_name(self->routine_toggle_group, target_name);
		g_signal_handlers_unblock_by_func(self->routine_toggle_group, on_routine_changed, self);
	}

	return G_SOURCE_REMOVE;
}

static void update_secondary_button_mode(SamayaWindow *self, gboolean is_running)
{
	if (is_running) {
		gtk_button_set_icon_name(self->reset_button, "media-skip-forward-symbolic");

		gtk_widget_set_size_request(GTK_WIDGET(self->reset_button), 44, 44);

		gtk_widget_add_css_class(GTK_WIDGET(self->reset_button), "circular");
		gtk_widget_remove_css_class(GTK_WIDGET(self->reset_button), "pill");
		gtk_widget_set_tooltip_text(GTK_WIDGET(self->reset_button), "Skip Session");
		gtk_actionable_set_action_name(GTK_ACTIONABLE(self->reset_button), "win.skip-session");
		gtk_widget_remove_css_class(GTK_WIDGET(self->reset_button), "destructive-action");
	} else {
		gtk_button_set_label(self->reset_button, "Reset");

		gtk_widget_set_size_request(GTK_WIDGET(self->reset_button), -1, -1);

		gtk_widget_remove_css_class(GTK_WIDGET(self->reset_button), "circular");
		gtk_widget_add_css_class(GTK_WIDGET(self->reset_button), "pill");

		gtk_widget_set_tooltip_text(GTK_WIDGET(self->reset_button), "Reset Timer");
		gtk_actionable_set_action_name(GTK_ACTIONABLE(self->reset_button), "win.reset-timer");
		gtk_widget_add_css_class(GTK_WIDGET(self->reset_button), "destructive-action");
	}
}

static void on_press_start(GtkWidget *widget,
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

		update_secondary_button_mode(self, FALSE);
	} else {
		timer_start(timer);
		gtk_button_set_label(self->start_button, "Pause");
		gtk_widget_remove_css_class(GTK_WIDGET(self->start_button), "suggested-action");
		gtk_widget_remove_css_class(GTK_WIDGET(self->start_button), "warning");

		update_secondary_button_mode(self, TRUE);
	}
}

static void on_press_reset(GtkWidget *widget,
                           const char *action_name,
                           GVariant *param)
{
	SamayaWindow *self = SAMAYA_WINDOW(widget);

	timer_reset(get_timer(self));

	gtk_button_set_label(self->start_button, "Start");
	gtk_widget_remove_css_class(GTK_WIDGET(self->start_button), "warning");
	gtk_widget_add_css_class(GTK_WIDGET(self->start_button), "suggested-action");
}

static void on_press_skip(GtkWidget *widget,
                          const char *action_name,
                          GVariant *param)
{
	SamayaWindow *self = SAMAYA_WINDOW(widget);

	// This will increment the count (if working) and switch to the next routine
	skip_current_session();

	// Ensure button state is correct (Start/Pause button reset)
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

	double line_width = 10.0;

	double center_x = width / 2.0;
	double center_y = height / 2.0;
	double radius = MIN(width, height) / 2.0 - line_width;

	gfloat progress = get_timer_progress(get_timer(self));

	cairo_set_line_width(cr, line_width);
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

	set_timer_instance_tick_callback(update_timer_label);
	set_routine_update_callback(update_routine_toggle_switch);

	Timer *timer = get_timer(self);
	gtk_label_set_text(self->timer_label, get_time_str(timer));
}

static void
samaya_window_class_init(SamayaWindowClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

	widget_class->realize = samaya_window_realize;

	gtk_widget_class_set_template_from_resource(widget_class, "/io/github/redddfoxxyy/samaya/samaya-window.ui");

	gtk_widget_class_bind_template_child(widget_class, SamayaWindow, routine_switch_box);
	gtk_widget_class_bind_template_child(widget_class, SamayaWindow, routine_toggle_group);

	gtk_widget_class_bind_template_child(widget_class, SamayaWindow, progress_circle);
	gtk_widget_class_bind_template_child(widget_class, SamayaWindow, timer_label);

	gtk_widget_class_bind_template_child(widget_class, SamayaWindow, start_button);
	gtk_widget_class_bind_template_child(widget_class, SamayaWindow, reset_button);

	gtk_widget_class_install_action(widget_class, "win.start-timer", NULL, on_press_start);
	gtk_widget_class_install_action(widget_class, "win.reset-timer", NULL, on_press_reset);
	gtk_widget_class_install_action(widget_class, "win.skip-session", NULL, on_press_skip);
}

static void
samaya_window_init(SamayaWindow *self)
{
	gtk_widget_init_template(GTK_WIDGET(self));

	gtk_drawing_area_set_draw_func(self->progress_circle, draw_progress_circle, self, NULL);

	g_signal_connect(self->routine_toggle_group, "notify::active-name", G_CALLBACK(on_routine_changed), self);
}
