/* samaya-preferences-dialog.c
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

#include "samaya-preferences-dialog.h"
#include <glib/gi18n.h>
#include "samaya-session.h"

struct _SamayaPreferencesDialog
{
    AdwPreferencesDialog parent_instance;

    AdwSpinRow *work_duration_row;
    AdwSpinRow *short_break_row;
    AdwSpinRow *long_break_row;

    AdwSpinRow *sessions_count_row;

    AdwSwitchRow *auto_start_breaks_row;
    AdwSwitchRow *auto_start_work_row;
};

G_DEFINE_FINAL_TYPE(SamayaPreferencesDialog, samaya_preferences_dialog, ADW_TYPE_PREFERENCES_DIALOG)


/* ============================================================================
 * Preferences change Handlers
 * ============================================================================ */

static void on_work_duration_changed(AdwSpinRow *row, GParamSpec *pspec, gpointer user_data)
{
    SessionManagerPtr session_manager = sm_get_default();
    if (session_manager) {
        gdouble val = adw_spin_row_get_value(row);
        sm_set_work_duration(session_manager, val);

        GSettings *settings = g_settings_new("io.github.redddfoxxyy.samaya");
        g_settings_set_double(settings, "work-duration", val);
        g_object_unref(settings);
    }
}

static void on_short_break_changed(AdwSpinRow *row, GParamSpec *pspec, gpointer user_data)
{
    SessionManagerPtr session_manager = sm_get_default();
    if (session_manager) {
        gdouble val = adw_spin_row_get_value(row);
        sm_set_short_break_duration(session_manager, val);

        GSettings *settings = g_settings_new("io.github.redddfoxxyy.samaya");
        g_settings_set_double(settings, "short-break-duration", val);
        g_object_unref(settings);
    }
}

static void on_long_break_changed(AdwSpinRow *row, GParamSpec *pspec, gpointer user_data)
{
    SessionManagerPtr session_manager = sm_get_default();
    if (session_manager) {
        gdouble val = adw_spin_row_get_value(row);
        sm_set_long_break_duration(session_manager, val);

        GSettings *settings = g_settings_new("io.github.redddfoxxyy.samaya");
        g_settings_set_double(settings, "long-break-duration", val);
        g_object_unref(settings);
    }
}

static void on_sessions_count_changed(AdwSpinRow *row, GParamSpec *pspec, gpointer user_data)
{
    SessionManagerPtr session_manager = sm_get_default();
    if (session_manager) {
        guint16 val = (guint16) adw_spin_row_get_value(row);
        sm_set_sessions_to_complete(session_manager, val);

        GSettings *settings = g_settings_new("io.github.redddfoxxyy.samaya");
        GVariant *variant = g_variant_new_uint16(val);
        g_settings_set_value(settings, "sessions-to-complete", variant);
        g_object_unref(settings);
    }
}

static void on_auto_start_breaks_changed(AdwSwitchRow *row, GParamSpec *pspec, gpointer user_data)
{
    SessionManagerPtr session_manager = sm_get_default();
    if (session_manager) {
        gboolean val = adw_switch_row_get_active(row);
        sm_set_auto_start_breaks(session_manager, val);

        GSettings *settings = g_settings_new("io.github.redddfoxxyy.samaya");
        g_settings_set_boolean(settings, "auto-start-breaks", val);
        g_object_unref(settings);
    }
}

static void on_auto_start_work_changed(AdwSwitchRow *row, GParamSpec *pspec, gpointer user_data)
{
    SessionManagerPtr session_manager = sm_get_default();
    if (session_manager) {
        gboolean val = adw_switch_row_get_active(row);
        sm_set_auto_start_work(session_manager, val);

        GSettings *settings = g_settings_new("io.github.redddfoxxyy.samaya");
        g_settings_set_boolean(settings, "auto-start-work", val);
        g_object_unref(settings);
    }
}

static void set_initial_preference_values(SessionManagerPtr session_manager,
                                          SamayaPreferencesDialog *self)
{
    if (session_manager != NULL) {
        g_signal_handlers_block_by_func(self->work_duration_row, on_work_duration_changed, self);
        adw_spin_row_set_value(self->work_duration_row, sm_get_work_duration(session_manager));
        g_signal_handlers_unblock_by_func(self->work_duration_row, on_work_duration_changed, self);

        g_signal_handlers_block_by_func(self->short_break_row, on_short_break_changed, self);
        adw_spin_row_set_value(self->short_break_row, sm_get_short_break_duration(session_manager));
        g_signal_handlers_unblock_by_func(self->short_break_row, on_short_break_changed, self);

        g_signal_handlers_block_by_func(self->long_break_row, on_long_break_changed, self);
        adw_spin_row_set_value(self->long_break_row, sm_get_long_break_duration(session_manager));
        g_signal_handlers_unblock_by_func(self->long_break_row, on_long_break_changed, self);

        g_signal_handlers_block_by_func(self->sessions_count_row, on_sessions_count_changed, self);
        adw_spin_row_set_value(self->sessions_count_row,
                               sm_get_sessions_to_complete(session_manager));
        g_signal_handlers_unblock_by_func(self->sessions_count_row, on_sessions_count_changed,
                                          self);

        g_signal_handlers_block_by_func(self->auto_start_breaks_row, on_auto_start_breaks_changed,
                                        self);
        adw_switch_row_set_active(self->auto_start_breaks_row,
                                  sm_get_auto_start_breaks(session_manager));
        g_signal_handlers_unblock_by_func(self->auto_start_breaks_row, on_auto_start_breaks_changed,
                                          self);

        g_signal_handlers_block_by_func(self->auto_start_work_row, on_auto_start_work_changed,
                                        self);
        adw_switch_row_set_active(self->auto_start_work_row,
                                  sm_get_auto_start_work(session_manager));
        g_signal_handlers_unblock_by_func(self->auto_start_work_row, on_auto_start_work_changed,
                                          self);
    }
}


/* ============================================================================
 * Samaya Preferences Dialog Methods
 * ============================================================================ */

static void samaya_preferences_dialog_class_init(SamayaPreferencesDialogClass *klass)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    gtk_widget_class_set_template_from_resource(
        widget_class, "/io/github/redddfoxxyy/samaya/preferences-dialog.ui");

    gtk_widget_class_bind_template_child(widget_class, SamayaPreferencesDialog, work_duration_row);
    gtk_widget_class_bind_template_child(widget_class, SamayaPreferencesDialog, short_break_row);
    gtk_widget_class_bind_template_child(widget_class, SamayaPreferencesDialog, long_break_row);
    gtk_widget_class_bind_template_child(widget_class, SamayaPreferencesDialog, sessions_count_row);
    gtk_widget_class_bind_template_child(widget_class, SamayaPreferencesDialog,
                                         auto_start_breaks_row);
    gtk_widget_class_bind_template_child(widget_class, SamayaPreferencesDialog,
                                         auto_start_work_row);

    gtk_widget_class_bind_template_callback(widget_class, on_work_duration_changed);
    gtk_widget_class_bind_template_callback(widget_class, on_short_break_changed);
    gtk_widget_class_bind_template_callback(widget_class, on_long_break_changed);
    gtk_widget_class_bind_template_callback(widget_class, on_sessions_count_changed);
    gtk_widget_class_bind_template_callback(widget_class, on_auto_start_breaks_changed);
    gtk_widget_class_bind_template_callback(widget_class, on_auto_start_work_changed);
}

static void samaya_preferences_dialog_init(SamayaPreferencesDialog *self)
{
    gtk_widget_init_template(GTK_WIDGET(self));

    SessionManagerPtr session_manager = sm_get_default();

    set_initial_preference_values(session_manager, self);
}

SamayaPreferencesDialog *samaya_preferences_dialog_new(void)
{
    return g_object_new(SAMAYA_TYPE_PREFERENCES_DIALOG, NULL);
}
