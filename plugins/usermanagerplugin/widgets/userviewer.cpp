/***************************************************************************
 *   FreeMedicalForms                                                      *
 *   (C) 2008-2010 by Eric MAEKER, MD                                     **
 *   eric.maeker@free.fr                                                   *
 *   All rights reserved.                                                  *
 *                                                                         *
 *   This program is a free and open source software.                      *
 *   It is released under the terms of the new BSD License.                *
 *                                                                         *
 *   Redistribution and use in source and binary forms, with or without    *
 *   modification, are permitted provided that the following conditions    *
 *   are met:                                                              *
 *   - Redistributions of source code must retain the above copyright      *
 *   notice, this list of conditions and the following disclaimer.         *
 *   - Redistributions in binary form must reproduce the above copyright   *
 *   notice, this list of conditions and the following disclaimer in the   *
 *   documentation and/or other materials provided with the distribution.  *
 *   - Neither the name of the FreeMedForms' organization nor the names of *
 *   its contributors may be used to endorse or promote products derived   *
 *   from this software without specific prior written permission.         *
 *                                                                         *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   *
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     *
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS     *
 *   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE        *
 *   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,  *
 *   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,  *
 *   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;      *
 *   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER      *
 *   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT    *
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN     *
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
 *   POSSIBILITY OF SUCH DAMAGE.                                           *
 ***************************************************************************/
/**
  \class UserViewer
  \brief This class is a data wrapper for users.
  When you instanciate this widget, it retreive and show the tkUserModel::currentUserIndex().\n
  Rights are managed via the tkUserModel.\n
  Changes are automaticaly saved into the tkUserModel.
  \todo limit memory usage.
  \ingroup usertoolkit widget_usertoolkit usermanager
*/

#include "userviewer.h"
#include "userviewer_p.h"

#include <utils/global.h>
#include <utils/serializer.h>

#include <coreplugin/translators.h>

#include <listviewplugin/stringlistview.h>

#include <printerplugin/printer.h>

#include <usermanagerplugin/usermodel.h>
#include <usermanagerplugin/widgets/userpassworddialog.h>

#include <QByteArray>
#include <QSqlTableModel>
#include <QHeaderView>
#include <QStringListModel>
#include <QTextEdit>

using namespace UserPlugin;
using namespace UserPlugin::Internal;

//--------------------------------------------------------------------------------------------------------
//-------------------------------------- Constructors / Destructors --------------------------------------
//--------------------------------------------------------------------------------------------------------
UserViewer::UserViewer(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("UserViewer");
    d = new UserViewerPrivate(this);
    d->initialize();
    if (!parent)
        Utils::centerWidget(this);
}

UserViewerPrivate::UserViewerPrivate(QObject *parent)
    : QObject(parent), m_Parent(0), m_Mapper(0),
      genericPreview(0), adminPreview(0), prescriptionPreview(0)
{
    Q_ASSERT_X(static_cast<QDialog *>(parent), "UserViewerPrivate", "*parent is not a QDialog");
    setObjectName("UserViewerPrivate");
    m_Parent = static_cast<QWidget *>(parent);
    m_Row = UserModel::instance()->currentUserIndex().row();
}

/** \brief Change current viewing user to \e modelRow from tkUserModel */
void UserViewer::changeUserTo(const int modelRow)
{
    d->changeUserIndex(modelRow);
}

/** \brief Change current viewing user to \e modelRow from tkUserModel */
void UserViewerPrivate::changeUserIndex(const int modelRow)
{
    int oldRow = m_Row;
    m_Row = modelRow;
    checkUserRights();
    if (m_CanRead)
        m_Mapper->setCurrentModelIndex(UserModel::instance()->index(modelRow, 0));
    else {
        m_Row = oldRow;
        Utils::informativeMessageBox(tr("You can not access to these datas."), tr("You don't have these rights."), "");
    }
}

//--------------------------------------------------------------------------------------------------------
//------------------------------------------- PRIVATE PART -----------------------------------------------
//--------------------------------------------------------------------------------------------------------
/** \brief Ui initializer. */
void UserViewerPrivate::initialize()
{
    setupUi(m_Parent);
    // add previewer
    genericPreview = Print::Printer::previewer(m_Parent);
    adminPreview = Print::Printer::previewer(m_Parent);
    prescriptionPreview = Print::Printer::previewer(m_Parent);
    editorsGenericLayout->addWidget(genericPreview, 0,0);
    editorsAdminLayout->addWidget(adminPreview, 0,0);
    editorsPrescriptionLayout->addWidget(prescriptionPreview, 0,0);

    // populate combos
    titleCombo->addItems(UserModel::instance()->titles());
    genderCombo->addItems(UserModel::instance()->genders());
    languageCombo->addItems(Core::Translators::availableLocales());
    // QListView need to be managed by hand
    QStringListModel * modelspe = new QStringListModel(this);
    specialtyListView->setModel(modelspe);
    specialtyListView->setActions(Views::ListView::AllActions);
    QStringListModel * modelqual = new QStringListModel(this);
    qualificationsListView->setModel(modelqual);
    QStringListModel * modelids = new QStringListModel(this);
    practIdsListView->setModel(modelids);

    checkUserRights();
    prepareMapper();

    // make connections
    connect(languageCombo, SIGNAL(activated(int)), this, SLOT(on_languageCombo_activated(int)));
    connect(but_changePassword, SIGNAL(clicked()), this, SLOT(on_but_changePassword_clicked()));
    connect(but_viewHistory, SIGNAL(clicked()), this, SLOT(on_but_viewHistory_clicked()));

    tabWidget->setCurrentWidget(tabIdentity);
    tabHeadersFooters->setCurrentWidget(genericTab);
}

/**
  \brief Mapper preparer
*/
void UserViewerPrivate::prepareMapper()
{
    m_Mapper = new QDataWidgetMapper(m_Parent);
    m_Mapper->setModel(UserModel::instance());
    m_Mapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);
    m_Mapper->addMapping(uuidLineEdit, User::Uuid);
    m_Mapper->addMapping(titleCombo, User::TitleIndex, "currentIndex");
    m_Mapper->addMapping(genderCombo, User::GenderIndex, "currentIndex");
    m_Mapper->addMapping(nameLineEdit, User::Name);
    m_Mapper->addMapping(loginLineEdit, User::DecryptedLogin);
    m_Mapper->addMapping(secNameLineEdit, User::SecondName);
    m_Mapper->addMapping(surnameLineEdit, User::Surname);
    m_Mapper->addMapping(lastLoginDateTimeEdit, User::LastLogin);
    m_Mapper->addMapping(languageCombo, User::LanguageIndex, "currentIndex");
    m_Mapper->addMapping(specialtyListView, User::Specialities);
    m_Mapper->addMapping(adressTextEdit, User::Adress, "plainText");
    m_Mapper->addMapping(countryLineEdit, User::Country);
    m_Mapper->addMapping(zipcodeLineEdit, User::Zipcode);
    m_Mapper->addMapping(cityLineEdit, User::City);
    m_Mapper->addMapping(tel1LineEdit, User::Tel1);
    m_Mapper->addMapping(tel2LineEdit, User::Tel2);
    m_Mapper->addMapping(tel3LineEdit, User::Tel3);
    m_Mapper->addMapping(faxLineEdit, User::Fax);
    m_Mapper->addMapping(mailLineEdit, User::Mail);
    m_Mapper->addMapping(specialtyListView, User::Specialities, "stringList");
    m_Mapper->addMapping(qualificationsListView, User::Qualifications, "stringList");
    m_Mapper->addMapping(practIdsListView, User::PractitionerId, "stringList");

    m_Mapper->addMapping(genericPreview->headerEditor() , User::GenericHeader, "html");
    m_Mapper->addMapping(genericPreview->footerEditor() , User::GenericFooter, "html");
    m_Mapper->addMapping(genericPreview->watermarkEditor(), User::GenericWatermark, "html");

    m_Mapper->addMapping(adminPreview->headerEditor(), User::AdministrativeHeader, "html");
    m_Mapper->addMapping(adminPreview->footerEditor(), User::AdministrativeFooter, "html");
    m_Mapper->addMapping(adminPreview->watermarkEditor(), User::AdministrativeWatermark, "html");

    m_Mapper->addMapping(prescriptionPreview->headerEditor(), User::PrescriptionHeader, "html");
    m_Mapper->addMapping(prescriptionPreview->footerEditor(), User::PrescriptionFooter, "html");
    m_Mapper->addMapping(prescriptionPreview->watermarkEditor(), User::PrescriptionWatermark, "html");

    m_Mapper->addMapping(userManagerRightsListWidget, User::ManagerRights, "rights");
    m_Mapper->addMapping(drugsRightsListWidget, User::DrugsRights, "rights");
    m_Mapper->addMapping(medicalRightsListWidget, User::MedicalRights, "rights");
    m_Mapper->addMapping(paramedicalRightsWidget, User::ParamedicalRights, "rights");
    m_Mapper->addMapping(administrativeRightsWidget, User::AdministrativeRights, "rights");

    m_Mapper->setCurrentModelIndex(UserModel::instance()->currentUserIndex());
    // make connections
    connect (m_Mapper->model(), SIGNAL(modelReset()), this, SLOT(on_modelReseted()));
}

/** \brief Change current user view. No save are done into the database from the model. */
void UserViewerPrivate::on_modelReseted()
{
    changeUserIndex(UserModel::instance()->currentUserIndex().row());
}

/** \brief Verify rights of user */
void UserViewerPrivate::checkUserRights()
{
    UserModel *m = UserModel::instance();
    int currentUserRow = m->currentUserIndex().row();
    if (currentUserRow == m_Row){
        // showing currentuser
        User::UserRights r = User::UserRights(m->index(currentUserRow, User::ManagerRights).data().toInt());
        m_CanModify = (r ^ User::WriteOwn);
        m_CanRead = (r ^ User::ReadOwn);
    } else {
        // not showing currentuser
        User::UserRights r = User::UserRights(m->index(currentUserRow, User::ManagerRights).data().toInt());
        m_CanModify = (r & User::WriteAll);
        m_CanRead = (r & User::ReadAll);
    }
    // enable editors
    foreach(QLineEdit *l, findChildren<QLineEdit *>())
        l->setEnabled(m_CanModify);
    foreach(QComboBox *c, findChildren<QComboBox *>())
        c->setEnabled(m_CanModify);
    foreach(QPushButton *c, findChildren<QPushButton *>())
        c->setEnabled(m_CanModify);
}

/** \brief Lange change requested by user from the ui */
void UserViewerPrivate::on_languageCombo_activated(int)
{
    Core::Translators::instance()->changeLanguage(languageCombo->currentText());
    retranslateUi(m_Parent);
}

void UserViewerPrivate::on_but_changePassword_clicked()
{
    UserModel *m = UserModel::instance();
    UserPasswordDialog d(m->index(m_Row, User::Password).data().toString(), m_Parent);
    if (d.exec() == QDialog::Accepted){
        if (! d.canGetNewPassword())
            return;
        QModelIndex idx = m->index(m_Row, User::Password);
        m->setData(idx, d.cryptedPassword());
    }
}

void UserViewerPrivate::on_but_viewHistory_clicked()
{
    UserModel *m = UserModel::instance();
    Utils::informativeMessageBox(tr("Login history."),
                                     tr("User %1\nLast connection : %2")
                           .arg(m->index(m_Row,User::Name).data().toString())
                           .arg(m->index(m_Row,User::LastLogin).data().toDateTime().toString()),
                            m->index(m_Row,User::LoginHistory).data().toString(),
                            qApp->applicationName());
}
