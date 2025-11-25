#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define SAMAYA_TYPE_PREFERENCES_DIALOG (samaya_preferences_dialog_get_type())

G_DECLARE_FINAL_TYPE(SamayaPreferencesDialog, samaya_preferences_dialog, SAMAYA, PREFERENCES_DIALOG, AdwPreferencesDialog)

SamayaPreferencesDialog *samaya_preferences_dialog_new(void);

G_END_DECLS
