/* samaya-preferences-dialog.h
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

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define SAMAYA_TYPE_PREFERENCES_DIALOG (samaya_preferences_dialog_get_type())

G_DECLARE_FINAL_TYPE(SamayaPreferencesDialog, samaya_preferences_dialog, SAMAYA, PREFERENCES_DIALOG,
                     AdwPreferencesDialog)

SamayaPreferencesDialog *samaya_preferences_dialog_new(void);

G_END_DECLS
