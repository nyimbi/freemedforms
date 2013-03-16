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
 *   Main Developpers:                                                     *
 *       Eric MAEKER, <eric.maeker@gmail.com>,                             *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
#include "accountitems.h"

#include <QStringList>
#include <QDebug>

namespace Account2 {

QDateTime VariableDatesItem::date(DateType type) const
{
    return _dates.value(type, QDateTime());
}

void VariableDatesItem::setDate(int type, const QDateTime &datetime)
{
    _dates.insert(type, datetime);
}

void VariableDatesItem::setDate(int type, const QDate &date)
{
    _dates.insert(type, QDateTime(date, QTime(0,0,0)));
}

QString VariableDatesItem::dateTypeUid(DateType type)
{
    switch (type) {
    case Date_MedicalRealisation: return "med_real";
    case Date_Invocing: return "inv";
    case Date_Payment: return "pay";
    case Date_Banking: return "bkg";
    case Date_Accountancy: return "acc";
    case Date_Creation: return "crea";
    case Date_Update: return "upd";
    case Date_Validation: return "val";
    case Date_Annulation: return "ann";
    case Date_ValidityPeriodStart: return "validitystart";
    case Date_ValidityPeriodEnd: return "validityend";
    default: return QString::null;
    }
    return QString::null;
}

bool Banking::canComputeTotalAmount()
{
    return _payments.count() == _paymentsId.count();
}

bool Banking::computeTotalAmount()
{
    if (!canComputeTotalAmount())
        return false;
    _total = 0.0;
    foreach(const Payment &pay, _payments)
        _total += pay.amount();
    return true;
}

void Banking::addPayment(const Payment &payment)
{
    _payments << payment;
    _paymentsId << payment.id();
}

} // namespace Account2


QDebug operator<<(QDebug dbg, const Account2::Fee &c)
{
    QStringList s;
    s << "Account2::Fee(" + c.id();
    if (c.isValid()) {
        if (c.isModified())
            s << "valid*";
        else
            s << "valid";
    } else {
        if (c.isModified())
            s << "notValid*";
        else
            s << "notValid";
    }
    s << "amount: " + QString::number(c.amount(), 'f', 6);
    s << "user: " + c.userUid();
    s << "patient: " + c.patientUid();
    s << "type: " + c.type();
    s << "comment: " + c.comment();
    for(int i = 0; i < Account2::DatesOfItem::Date_MaxParam; ++i) {
        if (c.date(Account2::Fee::DateType(i)).isValid())
            s << QString("date: %1 - %2").arg(i).arg(c.date(Account2::Fee::DateType(i)).isValid());
    }
    dbg.nospace() << s.join(",\n           ")
                  << ")";
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const Account2::Payment &c)
{
    QStringList s;
    s << "Account2::Payment(" + c.id();
    if (c.isValid()) {
        if (c.isModified())
            s << "valid*";
        else
            s << "valid";
    } else {
        if (c.isModified())
            s << "notValid*";
        else
            s << "notValid";
    }
    s << "amount: " + QString::number(c.amount(), 'f', 6);
    s << "type: " + c.type();
    s << "comment: " + c.comment();
    foreach(const Account2::Fee &fee, c.fees()) {
        s << "Fee: " + QString::number(fee.id()) + "; amount: " + QString::number(fee.amount(), 'f', 6);;
    }

    for(int i = 0; i < Account2::DatesOfItem::Date_MaxParam; ++i) {
        if (c.date(Account2::Fee::DateType(i)).isValid())
            s << QString("date: %1 - %2").arg(i).arg(c.date(Account2::Fee::DateType(i)).isValid());
    }
    dbg.nospace() << s.join(",\n           ")
                  << ")";
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const Account2::Banking &c)
{
    QStringList s;
    s << "Account2::Banking(" + c.id();
    if (c.isValid()) {
        if (c.isModified())
            s << "valid*";
        else
            s << "valid";
    } else {
        if (c.isModified())
            s << "notValid*";
        else
            s << "notValid";
    }
    s << "bkAccUid: " + c.bankAccountUuid();
    // s << "bkAccId: " + c.bankAccountId();
    s << "total: " + QString::number(c.totalAmount(), 'f', 6);
    foreach(const Account2::Payment &pay, c.payments()) {
        s << "Payment: " + QString::number(pay.id()) + "; amount: " + QString::number(pay.amount(), 'f', 6);
    }

    for(int i = 0; i < Account2::DatesOfItem::Date_MaxParam; ++i) {
        if (c.date(Account2::Fee::DateType(i)).isValid())
            s << QString("date: %1 - %2").arg(i).arg(c.date(Account2::Fee::DateType(i)).isValid());
    }
    dbg.nospace() << s.join(",\n           ")
                  << ")";
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const Account2::Quotation &c)
{
    return dbg.space();
}
