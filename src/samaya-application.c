/* samaya-application.c
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
#include "samaya-application.h"
#include "samaya-preferences-dialog.h"
#include "samaya-session.h"
#include "samaya-window.h"

struct _SamayaApplication
{
    AdwApplication parent_instance;

    SessionManagerPtr samayaSessionManager;
};

G_DEFINE_FINAL_TYPE(SamayaApplication, samaya_application, ADW_TYPE_APPLICATION)

/* ============================================================================
 * Samaya Application Methods
 * ============================================================================ */

static void samaya_application_preferences_action(GSimpleAction *action, GVariant *parameter,
                                                  gpointer user_data)
{
    SamayaApplication *self = SAMAYA_APPLICATION(user_data);
    GtkWindow *window = gtk_application_get_active_window(GTK_APPLICATION(self));

    SamayaPreferencesDialog *dialog = samaya_preferences_dialog_new();

    adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window));
}

static void samaya_application_about_action(GSimpleAction *action, GVariant *parameter,
                                            gpointer user_data)
{
    static const char *developers[] = {N_("Suyog Tandel"), NULL};
    static const char *artists[] = {
        N_("Suyog Tandel"), N_("Completion Sound:\nDr. Richard Boulanger et al (CC-BY 3.0)"), NULL};
    SamayaApplication *self = user_data;
    GtkWindow *window = NULL;

    g_assert(SAMAYA_IS_APPLICATION(self));

    window = gtk_application_get_active_window(GTK_APPLICATION(self));

    // clang-format off
    adw_show_about_dialog(
        GTK_WIDGET(window),
        "application-name", _("Samaya"),
        "application-icon", "io.github.redddfoxxyy.samaya",
        "developer-name", _("Suyog Tandel"),
        "translator-credits", _("translator-credits"),
        "version", "0.1.6",
        "developers", developers,
        "artists", artists,
        "website", "https://samaya.suyogtandel.in/",
        "issue-url", "https://codeberg.org/lockedmutex/samaya/issues",
        "copyright", "© 2025 Suyog Tandel",
        "license-type", GTK_LICENSE_AGPL_3_0,
        NULL
    );
    // clang-format on
}

static void samaya_application_quit_action(GSimpleAction *action, GVariant *parameter,
                                           gpointer user_data)
{
    SamayaApplication *self = user_data;

    g_assert(SAMAYA_IS_APPLICATION(self));

    g_application_quit(G_APPLICATION(self));
}

static const GActionEntry appActions[] = {
    {"quit", samaya_application_quit_action},
    {"about", samaya_application_about_action},
    {"preferences", samaya_application_preferences_action},
};

SamayaApplication *samaya_application_new(const char *application_id, GApplicationFlags flags)
{
    g_return_val_if_fail(application_id != NULL, NULL);

    return g_object_new(SAMAYA_TYPE_APPLICATION, "application-id", application_id, "flags", flags,
                        "resource-base-path", "/io/github/redddfoxxyy/samaya", NULL);
}

static void samaya_application_startup(GApplication *app)
{
    G_APPLICATION_CLASS(samaya_application_parent_class)->startup(app);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_resource(provider, "/io/github/redddfoxxyy/samaya/samaya-style.css");

    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                               GTK_STYLE_PROVIDER(provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref(provider);
}

static void samaya_application_activate(GApplication *app)
{
    GtkWindow *window;

    g_assert(SAMAYA_IS_APPLICATION(app));

    window = gtk_application_get_active_window(GTK_APPLICATION(app));

    if (window == NULL) {
        window = g_object_new(SAMAYA_TYPE_WINDOW, "application", app, NULL);
    }

    gtk_window_present(window);
}

static void samaya_application_dispose(GObject *object)
{
    SamayaApplication *self = SAMAYA_APPLICATION(object);

    if (self->samayaSessionManager) {
        sm_deinit(self->samayaSessionManager);
        self->samayaSessionManager = NULL;
    }

    G_OBJECT_CLASS(samaya_application_parent_class)->dispose(object);
}

static void samaya_application_class_init(SamayaApplicationClass *klass)
{
    GApplicationClass *app_class = G_APPLICATION_CLASS(klass);
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    app_class->startup = samaya_application_startup;
    app_class->activate = samaya_application_activate;
    object_class->dispose = samaya_application_dispose;
}

static void samaya_application_init(SamayaApplication *self)
{
    g_action_map_add_action_entries(G_ACTION_MAP(self), appActions, G_N_ELEMENTS(appActions), self);
    gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.quit",
                                          (const char *[]) {"<control>q", NULL});
    gtk_application_set_accels_for_action(GTK_APPLICATION(self), "app.preferences",
                                          (const char *[]) {"<control>comma", NULL});

    // TODO: Convert the given block of code till line 163 into a function.
    GSettings *settings = g_settings_new("io.github.redddfoxxyy.samaya");

    GVariant *sessions_variant = g_settings_get_value(settings, "sessions-to-complete");
    guint16 sessions = g_variant_get_uint16(sessions_variant);
    g_variant_unref(sessions_variant);

    gdouble work_duration = g_settings_get_double(settings, "work-duration");
    gdouble short_break_duration = g_settings_get_double(settings, "short-break-duration");
    gdouble long_break_duration = g_settings_get_double(settings, "long-break-duration");
    gboolean auto_breaks = g_settings_get_boolean(settings, "auto-start-breaks");
    gboolean auto_work = g_settings_get_boolean(settings, "auto-start-work");

    self->samayaSessionManager = sm_init(sessions, work_duration, short_break_duration,
                                         long_break_duration, auto_breaks, auto_work, NULL, self);

    g_object_unref(settings);
}
