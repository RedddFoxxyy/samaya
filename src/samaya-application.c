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

#include "config.h"
#include <glib/gi18n.h>

#include "samaya-application.h"
#include "samaya-window.h"
#include "samaya-preferences-dialog.h"
#include "samaya-timer.h"
#include "samaya-session.h"

struct _SamayaApplication
{
	AdwApplication parent_instance;

	// PomoDoro Session Manager Instance:
	SessionManager *samayaSessionManager;
};

G_DEFINE_FINAL_TYPE(SamayaApplication, samaya_application, ADW_TYPE_APPLICATION)

/* ============================================================================
 * Samaya Application GObject Methods
 * ============================================================================ */

SamayaApplication *
samaya_application_new(const char *application_id,
                       GApplicationFlags flags)
{
	g_return_val_if_fail(application_id != NULL, NULL);

	return g_object_new(SAMAYA_TYPE_APPLICATION,
	                    "application-id", application_id,
	                    "flags", flags,
	                    "resource-base-path", "/io/github/redddfoxxyy/samaya",
	                    NULL);
}

static void
samaya_application_startup(GApplication *app)
{
	G_APPLICATION_CLASS(samaya_application_parent_class)->startup(app);

	GtkCssProvider *provider = gtk_css_provider_new();
	gtk_css_provider_load_from_resource(provider, "/io/github/redddfoxxyy/samaya/samaya-style.css");

	gtk_style_context_add_provider_for_display(gdk_display_get_default(),
	                                           GTK_STYLE_PROVIDER(provider),
	                                           GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	g_object_unref(provider);
}

static void
samaya_application_activate(GApplication *app)
{
	GtkWindow *window;

	g_assert(SAMAYA_IS_APPLICATION (app));

	window = gtk_application_get_active_window(GTK_APPLICATION(app));

	if (window == NULL)
		window = g_object_new(SAMAYA_TYPE_WINDOW,
		                      "application", app,
		                      NULL);

	gtk_window_present(window);
}

static void
samaya_application_dispose(GObject *object)
{
	SamayaApplication *self = SAMAYA_APPLICATION(object);

	if (self->samayaSessionManager) {
		sm_deinit(self->samayaSessionManager);
		self->samayaSessionManager = NULL;
	}

	G_OBJECT_CLASS(samaya_application_parent_class)->dispose(object);
}

static void
samaya_application_class_init(SamayaApplicationClass *klass)
{
	GApplicationClass *app_class = G_APPLICATION_CLASS(klass);
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	app_class->startup = samaya_application_startup;
	app_class->activate = samaya_application_activate;
	object_class->dispose = samaya_application_dispose;
}

static void
samaya_application_preferences_action(GSimpleAction *action,
                                      GVariant *parameter,
                                      gpointer user_data)
{
	SamayaApplication *self = SAMAYA_APPLICATION(user_data);
	GtkWindow *window = gtk_application_get_active_window(GTK_APPLICATION(self));

	SamayaPreferencesDialog *dialog = samaya_preferences_dialog_new();

	adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window));
}

static void
samaya_application_about_action(GSimpleAction *action,
                                GVariant *parameter,
                                gpointer user_data)
{
	static const char *developers[] = {"Suyog Tandel", NULL};
	SamayaApplication *self = user_data;
	GtkWindow *window = NULL;

	g_assert(SAMAYA_IS_APPLICATION (self));

	window = gtk_application_get_active_window(GTK_APPLICATION(self));

	adw_show_about_dialog(GTK_WIDGET(window),
	                      "application-name", "samaya",
	                      "application-icon", "io.github.redddfoxxyy.samaya",
	                      "developer-name", "Suyog Tandel",
	                      "translator-credits", _("translator-credits"),
	                      "version", "0.1.0",
	                      "developers", developers,
	                      "copyright", "© 2025 Suyog Tandel",
	                      NULL);
}

static void
samaya_application_quit_action(GSimpleAction *action,
                               GVariant *parameter,
                               gpointer user_data)
{
	SamayaApplication *self = user_data;

	g_assert(SAMAYA_IS_APPLICATION (self));

	g_application_quit(G_APPLICATION(self));
}

static const GActionEntry app_actions[] = {
	{"quit", samaya_application_quit_action},
	{"about", samaya_application_about_action},
	{"preferences", samaya_application_preferences_action},
};

static void
samaya_application_init(SamayaApplication *self)
{
	g_action_map_add_action_entries(G_ACTION_MAP(self),
	                                app_actions,
	                                G_N_ELEMENTS(app_actions),
	                                self);
	gtk_application_set_accels_for_action(GTK_APPLICATION(self),
	                                      "app.quit",
	                                      (const char *[]){"<control>q", NULL});

	self->samayaSessionManager = sm_init(4, NULL, self);
}

/* ============================================================================
 * Timer Helpers for Samaya Application
 * ============================================================================ */

Timer *samaya_application_get_timer(SamayaApplication *self)
{
	return self->samayaSessionManager->timer_instance;
}

SessionManager *samaya_application_get_session_manager(SamayaApplication *self)
{
	return self->samayaSessionManager;
}
