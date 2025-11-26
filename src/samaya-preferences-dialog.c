#include "samaya-preferences-dialog.h"
#include "samaya-session.h"

struct _SamayaPreferencesDialog
{
	AdwPreferencesDialog parent_instance;
};

G_DEFINE_FINAL_TYPE(SamayaPreferencesDialog, samaya_preferences_dialog, ADW_TYPE_PREFERENCES_DIALOG)

static void
on_work_duration_changed(GtkAdjustment *adjustment,
                         GValue *value,
                         gpointer user_data)
{
	SessionManager *sm = sm_get_global_ptr();
	if (sm) {
		sm_set_work_duration(sm, gtk_adjustment_get_value(adjustment));
	}
}

static void
on_short_break_changed(GtkAdjustment *adjustment,
                       GValue *value,
                       gpointer user_data)
{
	SessionManager *sm = sm_get_global_ptr();
	if (sm) {
		sm_set_short_break_duration(sm, gtk_adjustment_get_value(adjustment));
	}
}

static void
on_long_break_changed(GtkAdjustment *adjustment,
                      GValue *value,
                      gpointer user_data)
{
	SessionManager *sm = sm_get_global_ptr();
	if (sm) {
		sm_set_long_break_duration(sm, gtk_adjustment_get_value(adjustment));
	}
}

static void
on_sessions_count_changed(GtkAdjustment *adjustment,
                          GValue *value,
                          gpointer user_data)
{
	SessionManager *sm = sm_get_global_ptr();
	if (sm) {
		sm_set_sessions_to_complete(sm, (gint) gtk_adjustment_get_value(adjustment));
	}
}

static void
samaya_preferences_dialog_class_init(SamayaPreferencesDialogClass *klass)
{
}

static void
samaya_preferences_dialog_init(SamayaPreferencesDialog *self)
{
	SessionManager *sm = sm_get_global_ptr();

	AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());

	adw_preferences_dialog_add(ADW_PREFERENCES_DIALOG(self), page);

	/* --- Timer Group --- */
	AdwPreferencesGroup *timer_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());

	adw_preferences_group_set_title(timer_group, "Timer Durations");
	adw_preferences_group_set_description(timer_group, "Set the duration (in minutes) for each routine.");
	adw_preferences_page_add(page, timer_group);

	// Work Duration
	GtkWidget *work_row = adw_spin_row_new_with_range(1.0, 120.0, 1.0);
	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(work_row), "Work");
	adw_spin_row_set_value(ADW_SPIN_ROW(work_row), sm ? sm->work_duration : 25.0);
	g_signal_connect(adw_spin_row_get_adjustment (ADW_SPIN_ROW (work_row)), "value-changed", G_CALLBACK (on_work_duration_changed), NULL);
	adw_preferences_group_add(timer_group, work_row);

	// Short Break Duration
	GtkWidget *short_row = adw_spin_row_new_with_range(1.0, 60.0, 1.0);
	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(short_row), "Short Break");
	adw_spin_row_set_value(ADW_SPIN_ROW(short_row), sm ? sm->short_break_duration : 5.0);
	g_signal_connect(adw_spin_row_get_adjustment (ADW_SPIN_ROW (short_row)), "value-changed", G_CALLBACK (on_short_break_changed), NULL);
	adw_preferences_group_add(timer_group, short_row);

	// Long Break Duration
	GtkWidget *long_row = adw_spin_row_new_with_range(1.0, 120.0, 1.0);
	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(long_row), "Long Break");
	adw_spin_row_set_value(ADW_SPIN_ROW(long_row), sm ? sm->long_break_duration : 20.0);
	g_signal_connect(adw_spin_row_get_adjustment (ADW_SPIN_ROW (long_row)), "value-changed", G_CALLBACK (on_long_break_changed), NULL);
	adw_preferences_group_add(timer_group, long_row);

	/* --- Session Group --- */
	AdwPreferencesGroup *session_group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());

	adw_preferences_group_set_title(session_group, "Session Cycle");
	adw_preferences_page_add(page, session_group);

	// Session Count
	GtkWidget *count_row = adw_spin_row_new_with_range(1.0, 10.0, 1.0);
	adw_preferences_row_set_title(ADW_PREFERENCES_ROW(count_row), "Sessions before Long Break");
	adw_spin_row_set_value(ADW_SPIN_ROW(count_row), sm ? sm->sessions_to_complete : 4.0);
	g_signal_connect(adw_spin_row_get_adjustment (ADW_SPIN_ROW (count_row)), "value-changed", G_CALLBACK (on_sessions_count_changed), NULL);
	adw_preferences_group_add(session_group, count_row);
}

SamayaPreferencesDialog *
samaya_preferences_dialog_new(void)
{
	return g_object_new(SAMAYA_TYPE_PREFERENCES_DIALOG, NULL);
}
