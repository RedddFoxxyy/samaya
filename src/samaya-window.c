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

#include "samaya-window.h"
#include <glib/gi18n.h>
#include <math.h>
#include "samaya-application.h"
#include "samaya-session.h"
#include "samaya-timer.h"

struct _SamayaWindow
{
    AdwApplicationWindow parent_instance;

    GtkBox *routine_switch_box;
    AdwToggleGroup *routine_toggle_group;

    GtkDrawingArea *progress_circle;
    GtkLabel *timer_label;
    GtkLabel *sessions_label;

    GtkButton *start_button;
    GtkButton *reset_button;
};

G_DEFINE_FINAL_TYPE(SamayaWindow, samaya_window, ADW_TYPE_APPLICATION_WINDOW)

/* ============================================================================
 * Function Definitions
 * ============================================================================ */

static void on_routine_toggled(AdwToggleGroup *toggle_group, GParamSpec *pspec,
                               gpointer samaya_window);

static void on_progress_draw(GtkDrawingArea *area, cairo_t *cr, int width, int height,
                             gpointer user_data);

static void sync_button_state(SamayaWindow *self);


/* ============================================================================
 * UI Actions
 * ============================================================================ */


static void sync_progress_style(SamayaWindow *self)
{
    SessionManagerPtr session_manager = sm_get_default();
    GtkWidget *widget = GTK_WIDGET(self->progress_circle);

    gtk_widget_remove_css_class(widget, "routine-working");
    gtk_widget_remove_css_class(widget, "routine-short-break");
    gtk_widget_remove_css_class(widget, "routine-long-break");

    switch (session_manager->current_routine) {
        case Working:
            gtk_widget_add_css_class(widget, "routine-working");
            break;
        case ShortBreak:
            gtk_widget_add_css_class(widget, "routine-short-break");
            break;
        case LongBreak:
            gtk_widget_add_css_class(widget, "routine-long-break");
            break;
        default:
            g_critical("Invalid Routine Type! Defaulting widget class to routine-working.");
            gtk_widget_add_css_class(widget, "routine-working");
            break;
    }

    gtk_widget_queue_draw(widget);
}

static gboolean on_tick_update(gpointer user_data)
{
    SamayaApplication *app = SAMAYA_APPLICATION(user_data);
    GtkWindow *window = gtk_application_get_active_window(GTK_APPLICATION(app));
    SamayaWindow *self = SAMAYA_WINDOW(window);

    SessionManagerPtr session_manager = sm_get_default();
    if (session_manager == NULL) {
        return G_SOURCE_REMOVE;
    }
    TimerPtr timer = session_manager->timer_instance;

    if (timer != NULL) {
        gtk_label_set_text(self->timer_label, sm_get_formatted_time(session_manager));
        gtk_widget_queue_draw(GTK_WIDGET(self->progress_circle));
    }

    char *session_text =
        g_strdup_printf("#%" G_GUINT64_FORMAT, session_manager->total_sessions_counted);

    gtk_label_set_text(self->sessions_label, session_text);
    g_free(session_text);

    if (tm_get_state(timer) != StRunning) {
        sync_button_state(self);
    }

    return G_SOURCE_REMOVE;
}

static gboolean sync_routine_selection(gpointer user_data)
{
    SamayaApplication *app = SAMAYA_APPLICATION(user_data);
    GtkWindow *window = gtk_application_get_active_window(GTK_APPLICATION(app));
    SamayaWindow *self = SAMAYA_WINDOW(window);

    RoutineType current_routine = sm_get_default()->current_routine;

    const char *target_name = NULL;

    switch (current_routine) {
        case Working:
            target_name = "pomodoro";
            break;
        case ShortBreak:
            target_name = "short-break";
            break;
        case LongBreak:
            target_name = "long-break";
            break;
        default:
            g_critical("Invalid Routine Type! Defaulting current routine to pomodoro.");
            target_name = "pomodoro";
            break;
    }

    g_signal_handlers_block_by_func(self->routine_toggle_group, on_routine_toggled, self);
    adw_toggle_group_set_active_name(self->routine_toggle_group, target_name);
    g_signal_handlers_unblock_by_func(self->routine_toggle_group, on_routine_toggled, self);

    sync_progress_style(self);


    return G_SOURCE_REMOVE;
}

static void sync_button_state(SamayaWindow *self)
{
    TimerPtr timer = sm_get_default()->timer_instance;
    GtkWidget *start_btn_widget = GTK_WIDGET(self->start_button);
    GtkWidget *reset_btn_widget = GTK_WIDGET(self->reset_button);
    TmState timer_state = tm_get_state(timer);

    switch (timer_state) {
        case StRunning:
            gtk_button_set_label(GTK_BUTTON(start_btn_widget), _("Stop"));
            gtk_widget_remove_css_class(start_btn_widget, "suggested-action");
            gtk_widget_add_css_class(start_btn_widget, "warning");

            gtk_widget_set_visible(reset_btn_widget, TRUE);
            gtk_button_set_icon_name(GTK_BUTTON(reset_btn_widget), "media-skip-forward-symbolic");
            gtk_widget_set_tooltip_text(reset_btn_widget, _("Skip Session"));
            gtk_actionable_set_action_name(GTK_ACTIONABLE(self->reset_button), "win.skip-session");
            gtk_widget_remove_css_class(reset_btn_widget, "destructive-action");
            break;

        case StPaused:
            gtk_button_set_label(GTK_BUTTON(start_btn_widget), _("Resume"));
            gtk_widget_remove_css_class(start_btn_widget, "warning");
            gtk_widget_add_css_class(start_btn_widget, "suggested-action");

            gtk_widget_set_visible(reset_btn_widget, TRUE);
            gtk_button_set_icon_name(GTK_BUTTON(reset_btn_widget), "view-refresh-symbolic");
            gtk_widget_set_tooltip_text(reset_btn_widget, _("Reset Timer"));
            gtk_actionable_set_action_name(GTK_ACTIONABLE(self->reset_button), "win.reset-timer");
            gtk_widget_add_css_class(reset_btn_widget, "destructive-action");
            break;

        case StIdle:
            gtk_button_set_label(GTK_BUTTON(start_btn_widget), _("Start"));
            gtk_widget_remove_css_class(start_btn_widget, "warning");
            gtk_widget_add_css_class(start_btn_widget, "suggested-action");

            gtk_widget_set_visible(reset_btn_widget, FALSE);
            break;

        case StExited:
        default:
            gtk_widget_set_sensitive(start_btn_widget, FALSE);
            gtk_widget_set_sensitive(reset_btn_widget, FALSE);
            break;
    }
}

static void on_routine_toggled(AdwToggleGroup *toggle_group, GParamSpec *pspec,
                               gpointer samaya_window)
{
    SamayaWindow *self = SAMAYA_WINDOW(samaya_window);
    const char *active_name = adw_toggle_group_get_active_name(toggle_group);

    SessionManager *session_manager = sm_get_default();

    RoutineType routine;
    if (g_strcmp0(active_name, "pomodoro") == 0) {
        routine = Working;
    } else if (g_strcmp0(active_name, "short-break") == 0) {
        routine = ShortBreak;
    } else if (g_strcmp0(active_name, "long-break") == 0) {
        routine = LongBreak;
    } else {
        return;
    }

    sm_set_routine(routine, session_manager);

    sync_progress_style(self);
    sync_button_state(self);
}

static void on_action_start_stop(GtkWidget *widget, const char *action_name, GVariant *param)
{
    SamayaWindow *self = SAMAYA_WINDOW(widget);
    TimerPtr timer = sm_get_default()->timer_instance;
    TmState timer_state = tm_get_state(timer);

    if (timer_state == StRunning) {
        tm_trigger_event(timer, EvStop);
    } else {
        tm_trigger_event(timer, EvStart);
    }

    sync_button_state(self);
}

static void on_action_reset(GtkWidget *widget, const char *action_name, GVariant *param)
{
    SamayaWindow *self = SAMAYA_WINDOW(widget);
    TimerPtr timer = sm_get_default()->timer_instance;

    tm_trigger_event(timer, EvReset);

    sync_button_state(self);
}

static void on_action_skip(GtkWidget *widget, const char *action_name, GVariant *param)
{
    SamayaWindow *self = SAMAYA_WINDOW(widget);

    sm_skip_session();
    sync_button_state(self);
}

/* ============================================================================
 * Rendering Functions
 * ============================================================================ */

static void on_progress_draw(GtkDrawingArea *area, cairo_t *cr, int width, int height,
                             gpointer user_data)
{
    double line_width = 10.0;

    double center_x = width / 2.0;
    double center_y = height / 2.0;
    double radius = MIN(width, height) / 2.0 - line_width;

    gfloat progress = tm_get_progress(sm_get_default()->timer_instance);

    GdkRGBA color;
    gtk_widget_get_color(GTK_WIDGET(area), &color);

    cairo_set_line_width(cr, line_width);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    cairo_set_source_rgba(cr, color.red, color.green, color.blue, 0.2);
    cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
    cairo_stroke(cr);


    gdk_cairo_set_source_rgba(cr, &color);

    double start_angle = -M_PI / 2;
    double end_angle = start_angle + (2 * M_PI * progress);
    cairo_arc(cr, center_x, center_y, radius, start_angle, end_angle);
    cairo_stroke(cr);
}


/* ============================================================================
 * Samaya Window Methods
 * ============================================================================ */

static void samaya_window_realize(GtkWidget *widget)
{
    SamayaWindow *self = SAMAYA_WINDOW(widget);

    GTK_WIDGET_CLASS(samaya_window_parent_class)->realize(widget);

    sm_set_timer_tick_callback(on_tick_update);
    sm_set_routine_update_callback(sync_routine_selection);

    gtk_label_set_text(self->timer_label, sm_get_formatted_time(sm_get_default()));

    sync_progress_style(self);
    sync_button_state(self);
}

static void samaya_window_class_init(SamayaWindowClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    widget_class->realize = samaya_window_realize;

    gtk_widget_class_set_template_from_resource(widget_class,
                                                "/io/github/redddfoxxyy/samaya/samaya-window.ui");

    gtk_widget_class_bind_template_child(widget_class, SamayaWindow, routine_switch_box);
    gtk_widget_class_bind_template_child(widget_class, SamayaWindow, routine_toggle_group);

    gtk_widget_class_bind_template_child(widget_class, SamayaWindow, progress_circle);
    gtk_widget_class_bind_template_child(widget_class, SamayaWindow, timer_label);
    gtk_widget_class_bind_template_child(widget_class, SamayaWindow, sessions_label);

    gtk_widget_class_bind_template_child(widget_class, SamayaWindow, start_button);
    gtk_widget_class_bind_template_child(widget_class, SamayaWindow, reset_button);

    gtk_widget_class_install_action(widget_class, "win.start-timer", NULL, on_action_start_stop);
    gtk_widget_class_install_action(widget_class, "win.reset-timer", NULL, on_action_reset);
    gtk_widget_class_install_action(widget_class, "win.skip-session", NULL, on_action_skip);
}

static void samaya_window_init(SamayaWindow *self)
{
    gtk_widget_init_template(GTK_WIDGET(self));

    gtk_drawing_area_set_draw_func(self->progress_circle, on_progress_draw, self, NULL);

    g_signal_connect(self->routine_toggle_group, "notify::active-name",
                     G_CALLBACK(on_routine_toggled), self);
}
