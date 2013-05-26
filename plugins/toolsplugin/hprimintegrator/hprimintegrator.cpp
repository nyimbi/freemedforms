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
 *   Main developers: Eric MAEKER, <eric.maeker@gmail.com>                 *
 *   Contributors:                                                         *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 *       NAME <MAIL@ADDRESS.COM>                                           *
 ***************************************************************************/
/*!
 * \class Tools::Internal::HprimIntegrator
 */

#include "hprimintegrator.h"
#include "hprimintegratordialog.h"
#include "constants.h"

#include "ui_hprimintegratorwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/itheme.h>
#include <coreplugin/ipatient.h>
#include <coreplugin/isettings.h>
#include <coreplugin/constants_icons.h>
#include <coreplugin/constants_menus.h>

#include <patientbaseplugin/patientmodel.h>

#include <utils/log.h>
#include <utils/global.h>
#include <utils/hprimparser.h>

#include <QTimer>
#include <QFileSystemModel>
#include <QFileInfo>

#include <QDebug>

using namespace Tools;
using namespace Internal;
//using namespace Trans::ConstantTranslations;

static inline Core::ITheme *theme() {return Core::ICore::instance()->theme();}
static inline Core::ISettings *settings() {return Core::ICore::instance()->settings();}
static inline Core::IPatient *patient() {return Core::ICore::instance()->patient();}

namespace Tools {
namespace Internal {
class HprimIntegratorWidgetPrivate
{
public:
    HprimIntegratorWidgetPrivate(HprimIntegratorWidget *parent) :
        ui(0),
        _fileModel(0),
        q(parent)
    {
    }
    
    ~HprimIntegratorWidgetPrivate()
    {
        delete ui;
    }

    int refreshFileModelInterval() // in ms
    {
        // TODO: use a setting
        return 6000; // 60000 == 1 minute
    }
    
    // Read the content of the currently selected file
    QString readFileContent()
    {
        QModelIndex index = ui->dirContentTableView->currentIndex();
        const QString &encoding = settings()->value(Constants::S_DEFAULT_FILE_ENCODING, "MacRoman").toString();
        return Utils::readTextFile(_fileModel->fileInfo(index).absoluteFilePath(), encoding);
    }

public:
    Ui::HprimIntegratorWidget *ui;
    QFileSystemModel *_fileModel;

private:
    HprimIntegratorWidget *q;
};
} // namespace Internal
} // namespace Tools


/*! Constructor of the Tools::Internal::HprimIntegrator class */
HprimIntegratorWidget::HprimIntegratorWidget(QWidget *parent) :
    QWidget(parent),
    d(new HprimIntegratorWidgetPrivate(this))
{
    d->ui = new Ui::HprimIntegratorWidget;
    d->ui->setupUi(this);

    // Set courrier as font of the viewer
    QFont font = d->ui->contentViewer->font();
    font.setFamily("Courier");
    d->ui->contentViewer->setFont(font);

    d->ui->patientSelector->setFieldsToShow(Patients::PatientSelector::FullName | Patients::PatientSelector::DateOfBirth | Patients::PatientSelector::FullAddress);
    d->ui->patientSelector->setOnPatientActivatedSetAsCurrent(false);
    d->ui->patientSelector->initialize();

    d->_fileModel = new QFileSystemModel(this);
    d->_fileModel->setReadOnly(true);
    d->_fileModel->setResolveSymlinks(false);
    // TODO: should manage multiple path...
    QModelIndex root = d->_fileModel->setRootPath(settings()->value(Constants::S_PATH_TO_SCAN).toString());

    d->ui->dirContentTableView->setModel(d->_fileModel);
    d->ui->dirContentTableView->setRootIndex(root);
#if QT_VERSION < 0x050000
    d->ui->dirContentTableView->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    d->ui->dirContentTableView->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
    d->ui->dirContentTableView->horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);
#else
    // Qt5
    d->ui->dirContentTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    d->ui->dirContentTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    d->ui->dirContentTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
#endif
    connect(d->ui->dirContentTableView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(onFileSelected(QModelIndex, QModelIndex)));
    connect(d->ui->integrate, SIGNAL(clicked()), this, SLOT(onDataIntegrationRequested()));
}

/*! Destructor of the Tools::Internal::HprimIntegrator class */
HprimIntegratorWidget::~HprimIntegratorWidget()
{
    if (d)
        delete d;
    d = 0;
}

/*! Initializes the object with the default values. Return true if initialization was completed. */
bool HprimIntegratorWidget::initialize()
{
    return true;
}

/**
 * When a file is selected, read its content and show it in the viewer.
 * Tries also to find the related patient.
 */
void HprimIntegratorWidget::onFileSelected(const QModelIndex &, const QModelIndex &)
{
    // Read the content of the file
    QString content = d->readFileContent();
    // CR only
    if (content.contains("\r") && !content.contains("\n"))
        content = content.replace("\r", "\n");

    // Try to read HPRIM header
    Utils::HPRIM::HprimMessage msg = Utils::HPRIM::parseHprimRawSource(content);

    if (msg.isValid() && !msg.isNull()) {
        // Populate the viewer with the HPRIM content
        d->ui->contentViewer->clear();
        d->ui->contentViewer->appendHtml(msg.toBasicHtml());
        QTextCursor cursor = d->ui->contentViewer->textCursor();
        cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
        d->ui->contentViewer->setTextCursor(cursor);

        // Try to find the correct patient uuid
        Patients::PatientModel *model = new Patients::PatientModel(this);
        model->setFilter(msg.header().patientName(), msg.header().patientFirstName());
        if (model->rowCount() == 1) {
            d->ui->patientInfo->setText(tr("Patient correctly detected: %1; %2")
                                        .arg(model->data(model->index(0, Core::IPatient::FullName)).toString())
                                        .arg(model->data(model->index(0, Core::IPatient::DateOfBirth)).toString()));
        } else if (model->rowCount() == 0) {
            d->ui->patientInfo->setText(tr("No patient found with the following names: %1; %2")
                                        .arg(msg.header().patientName())
                                        .arg(msg.header().patientFirstName()));
        } else {
            d->ui->patientInfo->setText(tr("There are multiple patient with the same names: %1 %2. You will have to select the correct one.")
                                        .arg(msg.header().patientName())
                                        .arg(msg.header().patientFirstName()));
        }
        d->ui->patientSelector->setFilterPatientModel(msg.header().patientName(), msg.header().patientFirstName(), msg.header().patientDateOfBirth());
        d->ui->patientSelector->setSelectedPatient(0);
    } else {
        // Populate the viewer with the raw file content
        d->ui->contentViewer->setPlainText(content);

        d->ui->patientInfo->setText(tr("No patient found with the following names: %1; %2")
                                    .arg(msg.header().patientName())
                                    .arg(msg.header().patientFirstName()));
    }
}

void HprimIntegratorWidget::onDataIntegrationRequested()
{
    HprimIntegratorDialog dlg(this);
    dlg.initialize(d->readFileContent());
    Utils::resizeAndCenter(&dlg, this);
    if (dlg.exec() == QDialog::Rejected) {
        // Show a widget with an error
    } else {
        // Show a widget with the importation success message
        // Manage the imported hprim file
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////     HprimIntegratorMode     ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
HprimIntegratorMode::HprimIntegratorMode(QObject *parent) :
    Core::IMode(parent)
{
    setEnabled(true);
    setDisplayName(tr("HPRIM"));
    setIcon(theme()->icon(Core::Constants::ICONDOCTOR));
    setPriority(Core::Constants::P_MODE_TOOLS);
    setId("HprimIntegratorMode");
    setType("HprimIntegratorMode");
    setPatientBarVisibility(false);
    setWidget(new HprimIntegratorWidget);
}

HprimIntegratorMode::~HprimIntegratorMode()
{
    // Has did not parent the widget -> delete it by hand
    delete widget();
    setWidget(0);
}
