/*
 *  Copyright (C) 2011  Alexandre Courbot
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CORE_LANG_H
#define __CORE_LANG_H

#include "core/Preferences.h"
#include <QStringList>

class Lang {
  public:
    static PreferenceItem<QString> preferredDictLanguage;
    static PreferenceItem<QString> preferredGUILanguage;
    static PreferenceItem<bool> alwaysShowEnglish;

    /**
     * Returns the list of languages supported for the user interface and
     * database searches.
     */
    static const QStringList &supportedDictLanguages();
    static const QStringList &supportedGUILanguages();
    static QStringList preferredDictLanguages();
};

#endif
