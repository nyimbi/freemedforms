/***************************************************************************
 *  The FreeMedForms project is a set of free, open source medical         *
 *  applications.                                                          *
 *  (C) 2008-2010 by Eric MAEKER, MD (France) <eric.maeker@free.fr>        *
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
 *   Main Developper : Eric MAEKER, <eric.maeker@free.fr>                  *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADRESS>                                                *
 ***************************************************************************/
/**
  * \class Views::ListView
  * \brief This widget shows a QListView with some buttons and a context menu.
  * Actions inside the contextual menu and the toolbar can be setted using
    AvailableActions param and setActions().\n
  * You can reimplement addItem(), removeItem() and on_edit_triggered()
    that are called by buttons and menu (add , remove). \n
  * The getContextMenu() is automatically called when the contextMenu is requiered.\n
    You can reimplement getContextMenu() in order to use your own contextMenu.\n
    Remember that the poped menu will be deleted after execution. \n
  \todo There is a problem when including this widget into a QDataWidgetMapper, when this widget loses focus, datas are not retreived.
*/

#include "listview.h"
#include "listview_p.h"
#include "stringlistmodel.h"

#include <translationutils/constanttranslations.h>
#include <utils/log.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/contextmanager/contextmanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/itheme.h>
#include <coreplugin/icore.h>
#include <coreplugin/constants_menus.h>
#include <coreplugin/constants_icons.h>

#include <QVBoxLayout>
#include <QGridLayout>
#include <QAction>
#include <QMenu>
#include <QWidget>
#include <QListView>
#include <QStringListModel>
#include <QToolButton>
#include <QToolBar>

using namespace Views;
using namespace Views::Internal;
using namespace Trans::ConstantTranslations;

static inline Core::ActionManager *actionManager() { return Core::ICore::instance()->actionManager(); }
static inline Core::ContextManager *contextManager() { return Core::ICore::instance()->contextManager(); }

namespace ListViewConstants
{
    const char* const C_BASIC            = "context.ListView.basic";
    const char* const C_BASIC_ADDREMOVE  = "context.ListView.AddRemove";
    const char* const C_BASIC_MOVE       = "context.ListView.Move";
}

/////////////////////////////////////////////////////////////////////////// List View Manager
ListViewManager *ListViewManager::m_Instance = 0;

ListViewManager *ListViewManager::instance()
{
    if (!m_Instance)
        m_Instance = new ListViewManager(qApp);
    return m_Instance;
}

ListViewManager::ListViewManager(QObject *parent) : ListViewActionHandler(parent)
{
    connect(contextManager(), SIGNAL(contextChanged(Core::IContext*)),
            this, SLOT(updateContext(Core::IContext*)));
}

void ListViewManager::updateContext(Core::IContext *object)
{
//    if (object)
//        qWarning() << "context" << object;
    ListView *view = 0;
    do {
        if (!object) {
            if (!m_CurrentView)
                return;

            m_CurrentView = 0;
            break;
        }
        view = qobject_cast<ListView *>(object->widget());
        if (!view) {
            if (!m_CurrentView)
                return;

            m_CurrentView = 0;
            break;
        }

        if (view == m_CurrentView) {
            return;
        }

    } while (false);
    if (view) {
        ListViewActionHandler::setCurrentView(view);
    }
}



/////////////////////////////////////////////////////////////////////////// Action Handler
ListViewActionHandler::ListViewActionHandler(QObject *parent) :
        QObject(parent),
        aAddRow(0),
        aRemoveRow(0),
        aDown(0),
        aUp(0),
        aEdit(0),
        m_CurrentView(0)
{
    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    Core::UniqueIDManager *uid = Core::ICore::instance()->uniqueIDManager();
    Core::ITheme *th = Core::ICore::instance()->theme();
    QList<int> basicContext = QList<int>() << uid->uniqueIdentifier(ListViewConstants::C_BASIC);
    QList<int> addContext = QList<int>() << uid->uniqueIdentifier(ListViewConstants::C_BASIC_ADDREMOVE);
    QList<int> moveContext = QList<int>() << uid->uniqueIdentifier(ListViewConstants::C_BASIC_MOVE);

//    QList<int> allContexts;
//    allContexts << basicContext << addContext << moveContext;
//    // register already existing menu actions
//    aUndo = registerAction(Core::Constants::A_EDIT_UNDO,  allContexts, this);
//    aRedo = registerAction(Core::Constants::A_EDIT_REDO,  allContexts, this);
//    aCut = registerAction(Core::Constants::A_EDIT_CUT,   allContexts, this);
//    aCopy = registerAction(Core::Constants::A_EDIT_COPY,  allContexts, this);
//    aPaste = registerAction(Core::Constants::A_EDIT_PASTE, allContexts, this);

    // Editor's Contextual Menu
    Core::ActionContainer *editMenu = am->actionContainer(Core::Constants::M_EDIT);
    Core::ActionContainer *cmenu = am->actionContainer(Core::Constants::M_EDIT_LIST);
    if (!cmenu) {
        cmenu = am->createMenu(Core::Constants::M_EDIT_LIST);
        cmenu->appendGroup(Core::Constants::G_EDIT_LIST);
        cmenu->setTranslations(Trans::Constants::M_EDIT_LIST_TEXT);
        if (editMenu)
            editMenu->addMenu(cmenu, Core::Constants::G_EDIT_LIST);
    }

    QAction *a = aAddRow = new QAction(this);
    a->setObjectName("ListView.aAddRow");
    a->setIcon(th->icon(Core::Constants::ICONADD));
    Core::Command *cmd = am->registerAction(a, Core::Constants::A_LIST_ADD, addContext);
    cmd->setTranslations(Trans::Constants::LISTADD_TEXT);
    cmenu->addAction(cmd, Core::Constants::G_EDIT_LIST);
    connect(a, SIGNAL(triggered()), this, SLOT(addItem()));

    a = aRemoveRow = new QAction(this);
    a->setObjectName("ListView.aRemoveRow");
    a->setIcon(th->icon(Core::Constants::ICONREMOVE));
    cmd = am->registerAction(a, Core::Constants::A_LIST_REMOVE, addContext);
    cmd->setTranslations(Trans::Constants::LISTREMOVE_TEXT);
    cmenu->addAction(cmd, Core::Constants::G_EDIT_LIST);
    connect(a, SIGNAL(triggered()), this, SLOT(removeItem()));

    a = aDown = new QAction(this);
    a->setObjectName("ListView.aDown");
    a->setIcon(th->icon(Core::Constants::ICONMOVEDOWN));
    cmd = am->registerAction(a, Core::Constants::A_LIST_MOVEDOWN, moveContext);
    cmd->setTranslations(Trans::Constants::LISTMOVEDOWN_TEXT);
    cmenu->addAction(cmd, Core::Constants::G_EDIT_LIST);
    connect(a, SIGNAL(triggered()), this, SLOT(moveDown()));

    a = aUp = new QAction(this);
    a->setObjectName("ListView.aUp");
    a->setIcon(th->icon(Core::Constants::ICONMOVEUP));
    cmd = am->registerAction(a, Core::Constants::A_LIST_MOVEUP, moveContext);
    cmd->setTranslations(Trans::Constants::LISTMOVEUP_TEXT);
    cmenu->addAction(cmd, Core::Constants::G_EDIT_LIST);
    connect(a, SIGNAL(triggered()), this, SLOT(moveUp()));

}

void ListViewActionHandler::setCurrentView(ListView *view)
{
//    if (view)
//        qWarning() << "current view " << view;
    // disconnect old view
    if (m_CurrentView) {
        disconnect(m_CurrentView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                   this, SLOT(listViewItemChanged()));
    }
    m_CurrentView = view;
    if (!view) { // this should never be the case
        return;
    }
    // reconnect some actions
    connect(m_CurrentView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(listViewItemChanged()));
    updateActions();
}

void ListViewActionHandler::listViewItemChanged()
{
    aUp->setEnabled(canMoveUp());
    aDown->setEnabled(canMoveDown());
}

void ListViewActionHandler::updateActions()
{
    listViewItemChanged();
}

bool ListViewActionHandler::canMoveUp()
{
    if (!m_CurrentView)
        return false;
    QModelIndex idx = m_CurrentView->currentIndex();
    if (!idx.isValid())
        return false;
    if (idx.row() >= 1)
        return true;
    return false;
}

bool ListViewActionHandler::canMoveDown()
{
    if (!m_CurrentView)
        return false;
    QModelIndex idx = m_CurrentView->currentIndex();
    if (!idx.isValid())
        return false;
    if (idx.row() < (m_CurrentView->model()->rowCount()-1))
        return true;
    return false;
}

void ListViewActionHandler::moveUp()
{ if (m_CurrentView) m_CurrentView->moveUp(); }
void ListViewActionHandler::moveDown()
{ if (m_CurrentView) m_CurrentView->moveDown(); }
void ListViewActionHandler::addItem()
{ if (m_CurrentView) m_CurrentView->addItem(); }
void ListViewActionHandler::removeItem()
{ if (m_CurrentView) m_CurrentView->removeItem(); }


/////////////////////////////////////////////////////////////////////////// List View
namespace Views {
namespace Internal {

class ListViewPrivate
{
public:
    ListViewPrivate(QWidget * parent, ListView::AvailableActions actions) :
            m_Parent(parent),
            m_Actions(actions),
            m_Context(0)
    {
    }

    ~ListViewPrivate()
    {
    }

    void calculateContext()
    {
        m_Context->clearContext();
        if (m_Actions & ListView::MoveUpDown)
            m_Context->addContext(Core::ICore::instance()->uniqueIDManager()->uniqueIdentifier(ListViewConstants::C_BASIC_MOVE));

        if (m_Actions & ListView::AddRemove)
            m_Context->addContext(Core::ICore::instance()->uniqueIDManager()->uniqueIdentifier(ListViewConstants::C_BASIC_ADDREMOVE));
    }

    void populateToolbar()
    {
        Core::ActionManager *am = Core::ICore::instance()->actionManager();
        if (m_Actions & ListView::AddRemove) {
            Core::Command *cmd = am->command(Core::Constants::A_LIST_ADD);
            m_ToolBar->addAction(cmd->action());
            cmd = am->command(Core::Constants::A_LIST_REMOVE);
            m_ToolBar->addAction(cmd->action());
        }

        if (m_Actions & ListView::MoveUpDown) {
            Core::Command *cmd = am->command(Core::Constants::A_LIST_MOVEUP);
            m_ToolBar->addAction(cmd->action());
            cmd = am->command(Core::Constants::A_LIST_MOVEDOWN);
            m_ToolBar->addAction(cmd->action());
        }
    }

public:
    QWidget *m_Parent;
    ListView::AvailableActions m_Actions;
    ListViewContext *m_Context;
    QToolBar *m_ToolBar;
    QString m_ContextName;
};

}  // End Internal
}  // End Views


/** \brief Constructor */
ListView::ListView(QWidget *parent, AvailableActions actions)
    : QListView(parent),
    d(0)
{
    static int handler = 0;
    ++handler;
    QObject::setObjectName("ListView_"+QString::number(handler));
    d = new Internal::ListViewPrivate(this, actions);

    // Create the Manager instance and context
    ListViewManager::instance();
    d->m_Context = new ListViewContext(this);
    d->calculateContext();
    contextManager()->addContextObject(d->m_Context);

    // Add toolbar to the horizontal scroolbar
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    d->m_ToolBar = new QToolBar(this);
    d->m_ToolBar->setIconSize(QSize(16,16));
    d->m_ToolBar->setFocusPolicy(Qt::ClickFocus);
    d->m_ToolBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    d->populateToolbar();
    addScrollBarWidget(d->m_ToolBar, Qt::AlignLeft);

    // Manage context menu
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(contextMenu(const QPoint &)));
}

ListView::~ListView()
{
    contextManager()->removeContextObject(d->m_Context);
}

/** \brief Defines the objectName */
void ListView::setObjectName(const QString &name)
{
    setObjectName(name+"ListView");
    QListView::setObjectName(name);
}

void ListView::setActions(AvailableActions actions)
{
    d->m_Actions = actions;
    d->calculateContext();
}

void ListView::hideButtons() const
{
    d->m_ToolBar->hide();
}

void ListView::showButtons()
{
    d->m_ToolBar->show();
}

void ListView::useContextMenu(bool state)
{
    if (state)
        setContextMenuPolicy(Qt::CustomContextMenu);
    else
        setContextMenuPolicy(Qt::NoContextMenu);
}

QMenu *ListView::getContextMenu()
{
    QMenu *pop = new QMenu(d->m_Parent);
    pop->addActions(d->m_ToolBar->actions());
    return pop;
}

void ListView::addItem()
{
    if (!model())
        return;
    
    // insert a row into model
    int row = 0;
    if (currentIndex().isValid()) {
        row = currentIndex().row() + 1;
    } else {
        row = model()->rowCount();
        if (row<0)
            row = 0;
    }
    if (!model()->insertRows(row, 1))
        Utils::Log::addError(this, QString("ListView can not add a row to the model %1").arg(model()->objectName()),
                             __FILE__, __LINE__);

    // select inserted row and edit it
    QModelIndex index = model()->index(row, modelColumn());
    setCurrentIndex(index);
    if (editTriggers() != QAbstractItemView::NoEditTriggers) {
        edit(index);
    }
    Q_EMIT addRequested();
}

void ListView::removeItem()
{
    if (!model())
        return;
    const QModelIndex &idx = currentIndex();
    if (idx.isValid()) {
        // Do this to keep QDataWidgetMapper informed of the modification
        edit(idx);
        closePersistentEditor(idx);
        // Now delete row
        int row = idx.row();
        if (!model()->removeRow(row)) {
            Utils::Log::addError(this, QString("ListView can not remove row %1 to the model %2")
                             .arg(row)
                             .arg(model()->objectName()),
                             __FILE__, __LINE__);
        }
    }
    Q_EMIT removeRequested();
}

void ListView::moveDown()
{
//    if (!d->canMoveDown())
//        return;

    QModelIndex idx = currentIndex();
    bool moved = false;

    StringListModel *m = qobject_cast<StringListModel*>(model());
    if (m) {
        m->moveDown(idx);
        moved = true;
    } else {
        QStringListModel *strModel = qobject_cast<QStringListModel*>(model());
        if (strModel) {
            QStringList list = strModel->stringList();
            list.move(idx.row(), idx.row()+1);
            strModel->setStringList(list);
            moved=true;
        }
    }
    // TODO : else swap the two rows.

    if (moved)
        setCurrentIndex(model()->index(idx.row()+1,0));
    Q_EMIT moveDownRequested();
}

void ListView::moveUp()
{
    QModelIndex idx = currentIndex();
//    closePersistentEditor(idx);
    bool moved = false;

    StringListModel *m = qobject_cast<StringListModel*>(model());
    if (m) {
        m->moveUp(idx);
        moved = true;
    } else {
        QStringListModel *strModel = qobject_cast<QStringListModel*>(model());
        if (strModel) {
            QStringList list = strModel->stringList();
            list.move(idx.row(), idx.row()-1);
            strModel->setStringList(list);
            moved=true;
        }
    }
    // TODO : else swap the two rows.

    if (moved)
        setCurrentIndex(model()->index(idx.row()-1,0));
    Q_EMIT moveUpRequested();
}

void ListView::on_edit_triggered()
{}

void ListView::contextMenu(const QPoint & p)
{
    QMenu *pop = getContextMenu();
    pop->popup(this->mapToGlobal(p));
    pop->exec();
    delete pop;
    pop = 0;
}
