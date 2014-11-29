/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2014 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
 *  All rights reserved.                                                   *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program (COPYING.FREEMEDFORMS file).                   *
 *  If not, see <http://www.gnu.org/licenses/>.                            *
 ***************************************************************************/
/***************************************************************************
 *  Main Developers: Eric MAEKER, <eric.maeker@gmail.com>                  *
 *  Contributors:                                                          *
 *      NAME <MAIL@ADDRESS.COM>                                            *
 ***************************************************************************/
#ifndef GUARD_GUARDMODE_H
#define GUARD_GUARDMODE_H

#include <coreplugin/modemanager/imode.h>

/**
 * \file quargmode.h
 * \author Eric Maeker
 * \version 0.10.0
 * \date 12 Oct 2014
*/

namespace Guard {
namespace Internal {

class GuardianMode : public Core::IMode
{
    Q_OBJECT
public:
    explicit GuardianMode(QObject *parent = 0);
    ~GuardianMode();

    void postCoreInitialization();

#ifdef WITH_TESTS
    void test_runWidgetTests();
#endif

private:
//    __Widget *_widget;

};

}  // End namespace Internal
}  // End namespace Guard

#endif // GUARD_GUARDMODE_H