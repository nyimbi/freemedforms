/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2012 by Eric MAEKER, MD (France) <eric.maeker@gmail.com>      *
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
 *   Main developers : Christian A. Reiter, <christian.a.reiter@gmail.com> *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
#ifndef DATEVALIDATOR_H
#define DATEVALIDATOR_H

#include "utils/global_exporter.h"
#include <QValidator>
#include <QStringList>
#include <QDate>

namespace Utils {

class UTILS_EXPORT DateValidator : public QValidator
{
    Q_OBJECT
public:
    explicit DateValidator(QObject *parent = 0);
    State validate(QString &input, int &pos) const;
    void fixup(QString &input) const;

    void setDate(const QDate &date);
    QDate date() const;

    void addDateFormat(const QString &format);
    QStringList acceptedDateFormat() const {return m_dateFormatList;}

private:
    QStringList m_dateFormatList;
    QString m_lastValidFormat;
    mutable QDate _currentDate;
};
} // end Utils
#endif // DATEVALIDATOR_H