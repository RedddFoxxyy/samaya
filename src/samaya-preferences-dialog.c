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

#include <glib/gi18n.h>
#include "samaya-preferences-dialog.h"
#include "samaya-session.h"

struct _SamayaPreferencesDialog
{
    AdwPreferencesDialog parent_instance;
};

G_DEFINE_FINAL_TYPE(SamayaPreferencesDialog, samaya_preferences_dialog, ADW_TYPE_PREFERENCES_DIALOG)


/* ============================================================================
 * Preferences change Handlers
 * ============================================================================ */

static void on_work_duration_changed(GtkAdjustment *adjustment)
{
    SessionManagerPtr session_manager = sm_get_global();
    if (session_manager) {
        gdouble val = gtk_adjustment_get_value(adjustment);
        sm_set_work_duration(session_manager, val);

        GSettings *settings = g_settings_new("io.github.redddfoxxyy.samaya");
        g_settings_set_double(settings, "work-duration", val);
        g_object_unref(settings);
    }
}

static void on_short_break_changed(GtkAdjustment *adjustment)
{
    SessionManagerPtr session_manager = sm_get_global();
    if (session_manager) {
        gdouble val = gtk_adjustment_get_value(adjustment);
        sm_set_short_break_duration(session_manager, val);

        GSettings *settings = g_settings_new("io.github.redddfoxxyy.samaya");
        g_settings_set_double(settings, "short-break-duration", val);
        g_object_unref(settings);
    }
}

static void on_long_break_changed(GtkAdjustment *adjustment)
{
    SessionManagerPtr session_manager = sm_get_global();
    if (session_manager) {
        gdouble val = gtk_adjustment_get_value(adjustment);
        sm_set_long_break_duration(session_manager, val);

        GSettings *settings = g_settings_new("io.github.redddfoxxyy.samaya");
        g_settings_set_double(settings, "long-break-duration", val);
        g_object_unref(settings);
    }
}

static void on_sessions_count_changed(GtkAdjustment *adjustment)
{
    SessionManagerPtr session_manager = sm_get_global();
    if (session_manager) {
        guint16 val = (guint16) gtk_adjustment_get_value(adjustment);
        sm_set_sessions_to_complete(session_manager, val);

        GSettings *settings = g_settings_new("io.github.redddfoxxyy.samaya");
        GVariant *variant = g_variant_new_uint16(val);
        g_settings_set_value(settings, "sessions-to-complete", variant);
        g_object_unref(settings);
    }
}


/* ============================================================================
 * Samaya Preferences Dialog Group Initializers
 * ============================================================================ */

static AdwPreferencesGroup *create_preferences_group(AdwPreferencesPage *preferences_page,
                                                     char *group_title, char *group_description)
{
    AdwPreferencesGroup *new_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());

    if (group_title != NULL) {
        adw_preferences_group_set_title(new_group, group_title);
    }

    if (group_description != NULL) {
        adw_preferences_group_set_description(new_group, group_description);
    }

    adw_preferences_page_add(preferences_page, new_group);

    return new_group;
}

typedef double (*GetRoutineDuration)(SessionManagerPtr);

static void create_preferences_row(SessionManagerPtr session_manager,
                                   AdwPreferencesGroup *timer_group, char *row_title,
                                   double range_min, double range_max, double range_step,
                                   GetRoutineDuration get_duration, GCallback on_value_changed)
{
    GtkWidget *preferences_row = adw_spin_row_new_with_range(range_min, range_max, range_step);

    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(preferences_row), row_title);

    adw_spin_row_set_value(ADW_SPIN_ROW(preferences_row),
                           session_manager ? get_duration(session_manager) : 0.0);

    g_signal_connect(adw_spin_row_get_adjustment(ADW_SPIN_ROW(preferences_row)), "value-changed",
                     on_value_changed, NULL);

    adw_preferences_group_add(timer_group, preferences_row);
}

static void init_timer_preferences_group(AdwPreferencesPage *preferences_page,
                                         SessionManagerPtr session_manager)
{
    AdwPreferencesGroup *timer_group =
        create_preferences_group(preferences_page, _("Timer Durations"),
                                 _("Set the duration (in minutes) for each routine."));

    create_preferences_row(session_manager, timer_group, _("Work"), 0.5, 999.0, 0.5,
                           sm_get_work_duration, G_CALLBACK(on_work_duration_changed));

    create_preferences_row(session_manager, timer_group, _("Short Break"), 0.5, 999.0, 0.5,
                           sm_get_short_break_duration, G_CALLBACK(on_short_break_changed));

    create_preferences_row(session_manager, timer_group, _("Long Break"), 0.5, 999.0, 0.5,
                           sm_get_long_break_duration, G_CALLBACK(on_long_break_changed));
}

static void init_session_preferences_group(AdwPreferencesPage *preferences_page,
                                           SessionManagerPtr session_manager)
{
    AdwPreferencesGroup *session_group =
        create_preferences_group(preferences_page, _("Session Cycle"), NULL);

    create_preferences_row(session_manager, session_group, _("Sessions before Long Break"), 1.0,
                           100.0, 1.0, sm_get_sessions_to_complete,
                           G_CALLBACK(on_sessions_count_changed));
}


/* ============================================================================
 * Samaya Preferences Dialog Methods
 * ============================================================================ */

static void samaya_preferences_dialog_class_init(SamayaPreferencesDialogClass *klass)
{
}

static void samaya_preferences_dialog_init(SamayaPreferencesDialog *self)
{
    SessionManagerPtr session_manager = sm_get_global();

    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());

    adw_preferences_dialog_add(ADW_PREFERENCES_DIALOG(self), page);

    init_timer_preferences_group(page, session_manager);

    init_session_preferences_group(page, session_manager);
}

SamayaPreferencesDialog *samaya_preferences_dialog_new(void)
{
    return g_object_new(SAMAYA_TYPE_PREFERENCES_DIALOG, NULL);
}
