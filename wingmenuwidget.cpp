#include "wingmenuwidget.h"
#include "applicationsview.h"
#include "itemdescriptiondelegate.h"
#include "wingmenuplugin.h"

#include <QClipboard>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTimer>

#include <lxqtglobalkeys.h>
#include <XmlHelper>

#define BTNMULTIPLIER 1.2
#define DEFAULT_MENU_FILE "/etc/xdg/menus/lxqt-applications.menu"

// Default menu layout cause i couldn't come up with
// sufficiently descriptive names
//  _________________________________________________                                            
// |           WingMenuWidget - RootBox              |         
// |  ______   ____________________________________  |                                       
// | |      | |                MainBox             | |
// | |      | |  ________________________________  | |                                        
// | |      | | |              MenuBox           | | |       
// | |      | | |  ______________   ___________  | | |                        
// | |Leave | | | |              | |           | | | |      
// | |Widget| | | |CategoryWidget| |           | | | |      
// | |  -   | | | |      -       | |StackWidget| | | |           
// | |Leave | | | |  CategoryBox | |           | | | |     
// | | Box  | | | |              | |           | | | |          
// | |      | | | |______________| |___________| | | |                               
// | |      | | |________________________________| | |                             
// | |      | |  ________________________________  | |                            
// | |      | | |            SearchEdit          | | |                                        
// | |      | | |________________________________| | |
// | |______| |____________________________________| |                                     
// |_________________________________________________|                                           
//                                            

WingMenuWidget::WingMenuWidget(WingMenuPlugin* plugin, QWidget* parent)
    : QWidget(parent),
    mPlugin(plugin),
    mIconSize(mPlugin->panel()->iconSize()),
    mRootBox(new QHBoxLayout(this)),
    mMainBox(new QVBoxLayout),
    mMenuBox(new QHBoxLayout),
    mSearchEdit(new QLineEdit(this)),
    mSideWidget(new QWidget),
    mSideBox(new QVBoxLayout(mSideWidget)),
    mCategoryWidget(new QWidget(this)),
    mCategoryBox(new QVBoxLayout(mCategoryWidget)),
    mApplicationsStack(new QStackedWidget(this)),
    mHoverTimer(new QTimer(this)), mHoveredAction(nullptr)
{
    auto logDir = mPlugin->settings()->value(QSL("logDir"), QString()).toString();
    mXdgMenu.setEnvironments(QStringList() << QSL("X-LXQT") << QSL("LXQt"));
    mXdgMenu.setLogDir(logDir);
    mMenuFile = mPlugin->settings()->value(QSL("menuFile"), QSL(DEFAULT_MENU_FILE)).toString();
    bool res = mXdgMenu.read(mMenuFile);
    if (!res)
        QMessageBox::warning(nullptr, QSL("Parse error"), mXdgMenu.errorString());

    auto appLayout = mPlugin->settings()->value(QSL("appLayout"), AppLayout::ListNameAndDescription).value<AppLayout::Layout>();
    mApplicationsView = new ApplicationsView(mPlugin->panel()->iconSize(), appLayout, mApplicationsStack);
    mApplicationsModel = new QStandardItemModel(mApplicationsView);
    mProxyModel = new QSortFilterProxyModel(mApplicationsView);
    mFavoritesView = new ApplicationsView(mPlugin->panel()->iconSize(), appLayout, mApplicationsStack);
    mFavoritesModel = new QStandardItemModel(mFavoritesView);
    setAutoFillBackground(true);
    mProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    mProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    mProxyModel->setSortLocaleAware(true);
    mProxyModel->setSourceModel(mApplicationsModel);
    mApplicationsView->setModel(mProxyModel);
    mApplicationsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mFavoritesView->setModel(mFavoritesModel);

    mSearchEdit->setAutoFillBackground(true);
    mSearchEdit->setObjectName(QSL("MainMenuSearchEdit"));
    mSearchEdit->setClearButtonEnabled(true);

    mMenuBox->addWidget(mCategoryWidget);
    mMenuBox->addWidget(mApplicationsStack);

    mMainBox->addLayout(mMenuBox);
    mMainBox->addWidget(mSearchEdit);

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    mRootBox->addWidget(mSideWidget);
    mRootBox->addLayout(mMainBox);

    mApplicationsStack->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    mApplicationsStack->addWidget(mApplicationsView);
    mApplicationsStack->addWidget(mFavoritesView);

    mSideGroup = new QActionGroup(this);
    mSideGroup->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
    mCategoryGroup = new QActionGroup(this);
    mCategoryGroup->setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
    mCategoryWidget->setLayout(mCategoryBox);

    mMainBox->setMargin(0);
    removeSpaces(mMenuBox);
    removeSpaces(mSideBox);
    removeSpaces(mCategoryBox);

    mHoverTimer->setSingleShot(true);

    settingsChanged();
    buildMenu();
    connectSignals();

    focusLineEdit();
    mSideWidget->installEventFilter(this);
}

void WingMenuWidget::onShow()
{
    if (mFavoritesList.count() == 0) {
        if (QAction* a = mCategoryGroup->actions().at(1))
            if (!a->isChecked())
                a->trigger();
    }
    else if (QAction* a = mCategoryGroup->actions().at(0)) {
        if (!a->isChecked())
            a->trigger();
    }
}

void WingMenuWidget::onHide()
{
    uncheckSideActions();
    mSearchEdit->setText(QString());
}

void WingMenuWidget::indexActivated(const QModelIndex& index)
{
    if (!index.isValid())
        return;
    XdgDesktopFile df;
    df.load(index.data(DataType::DesktopFile).toString());
    if (df.isValid())
        df.startDetached();
    emit hideMenu();
}

bool WingMenuWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == mSideWidget && event->type() == QEvent::Leave) {
        uncheckSideActions();
        return true;
    }
    return false;
}

void WingMenuWidget::keyPressEvent(QKeyEvent* keyEvent) {

    auto k = (Qt::Key)keyEvent->key();
    if (QKeySequence(keyEvent->key() | keyEvent->modifiers())
        == QKeySequence(mPlugin->shortcut()->shortcut())) {
        mPlugin->hideMenu();
        keyEvent->accept();
        return;
    }
    if (k == Qt::Key_Return || k == Qt::Key_Enter) {
        auto currentWidget = qobject_cast<ApplicationsView*>(mApplicationsStack->currentWidget());
        auto selection = currentWidget->selectionModel();
        if (mSideWidget->hasFocus() && nullptr != mSideGroup->checkedAction()) {
            sideActionTriggered(mSideGroup->checkedAction());
        }
        else if (selection->hasSelection()) {
            auto index = selection->selectedRows().first();
            indexActivated(index);
        }
        keyEvent->accept();
    }
    else if (k == Qt::Key_Escape) {
        mPlugin->hideMenu();
        keyEvent->accept();
    }
    else if (!keyEvent->text().isEmpty()
        && k != Qt::Key_Backspace
        && k != Qt::Key_Delete
        && k != Qt::Key_Tab) {

        mSearchEdit->setFocus();
        mSearchEdit->insert(keyEvent->text());

        keyEvent->accept();
    }
    else if (k == Qt::Key_Left
        || k == Qt::Key_Right
        || k == Qt::Key_Up
        || k == Qt::Key_Down) {

        if (mSearchEdit->hasFocus()) {
            if (k == Qt::Key_Up || k == Qt::Key_Down) {
                sendKeyToApplicationsList(k);
            }
        }
        else if (k == Qt::Key_Left
            || k == Qt::Key_Right) {
            if (mSideWidget->hasFocus()) {
                uncheckSideActions();
                if (mCategoryWidget->isVisible()
                    && ((k == Qt::Key_Left && !mCategoryLeft)
                        || (k == Qt::Key_Right && mCategoryLeft))) {
                    mCategoryWidget->setFocus();
                }
                else {
                    sendKeyToApplicationsList(k);
                }
            }
            else if (mCategoryWidget->hasFocus()) {
                if ((k == Qt::Key_Left && !mCategoryLeft)
                    || (k == Qt::Key_Right && mCategoryLeft)) {
                    sendKeyToApplicationsList(k);
                }
                else {
                    mSideWidget->setFocus();
                    if (mSideGroup->actions().count() > 0) {
                        if (mReverseSidebar)
                            mSideGroup->actions().last()->setChecked(true);
                        else
                            mSideGroup->actions().first()->setChecked(true);
                    }
                }
            }
            else if (mApplicationsStack->currentWidget()->hasFocus()) {
                auto current = qobject_cast<ApplicationsView*>(mApplicationsStack->currentWidget());
                current->setCurrentIndex(QModelIndex());
                if ((k == Qt::Key_Left && !mCategoryLeft)
                    || (k == Qt::Key_Right && mCategoryLeft)) {
                    mSideWidget->setFocus();
                    if (mSideGroup->actions().count() > 0) {
                        if (mReverseSidebar)
                            mSideGroup->actions().last()->setChecked(true);
                        else
                            mSideGroup->actions().first()->setChecked(true);
                    }
                }
                else {
                    mCategoryWidget->setFocus();
                }
            }
            keyEvent->accept();
        }
        // focus is NOT on search and key is UP or DOWN
        // this is to cycle between options on focused widget
        else {
            if (mSideWidget->hasFocus()) {
                QAction* a = mSideGroup->checkedAction();
                auto actions = mSideGroup->actions();

                if (nullptr != a) {
                    int nextIndex = actions.indexOf(a);
                    if (k == Qt::Key_Down) {
                        if (mReverseSidebar)
                            nextIndex--;
                        else
                            nextIndex++;

                    }
                    else if (k == Qt::Key_Up) {
                        if (mReverseSidebar)
                            nextIndex++;
                        else
                            nextIndex--;
                    }

                    if (nextIndex < 0)
                        nextIndex += actions.count();
                    else if (nextIndex >= actions.count())
                        nextIndex -= actions.count();

                    actions.at(nextIndex)->setChecked(true);
                }
                else if (actions.count() > 0) {
                    if (mReverseSidebar)
                        actions.last()->setChecked(true);
                    else
                        actions.first()->setChecked(true);
                }
            }
            else if (mCategoryWidget->hasFocus()) {
                auto actions = mCategoryGroup->actions();
                for (int i = 0; i < actions.count(); ++i) {
                    if (actions[i]->isChecked()) {
                        if ((i > 0) && (k == Qt::Key_Up)) {
                            actions[i - 1]->activate(QAction::ActionEvent::Trigger);
                        }
                        else if ((i < actions.count() - 1) && (k == Qt::Key_Down)) {
                            actions[i + 1]->activate(QAction::ActionEvent::Trigger);
                        }
                        else {
                            mSearchEdit->setFocus();
                        }
                        break;
                    }
                }
            }
            else if (mApplicationsStack->currentWidget()->hasFocus()) {
                sendKeyToApplicationsList(k);
            }
        }

        keyEvent->accept();
    }
    keyEvent->ignore();

}

void WingMenuWidget::sendKeyToApplicationsList(Qt::Key key) {
    auto current = qobject_cast<ApplicationsView*>(mApplicationsStack->currentWidget());
    auto model = current->model();
    auto index = current->currentIndex();
    auto rows = model->rowCount();
    // when current has focus, this function won't receive left and right keys,
    // it is handled by keyPressEvent
    if (current->hasFocus()) {
        if (index.isValid()) {
            QModelIndex nextIndex;
            if (key == Qt::Key_Up && index.row() > 0) {
                auto nextIndex = model->index(index.row() - 1, 0);
                current->setCurrentIndex(nextIndex);
            }
            else if (key == Qt::Key_Down && index.row() < rows - 1) {
                auto nextIndex = model->index(index.row() + 1, 0);
                current->setCurrentIndex(nextIndex);
            }
            else {
                mSearchEdit->setFocus();
            }
        }
        else if (rows > 0) {
            if (key == Qt::Key_Up) {
                auto nextIndex = model->index(rows - 1, 0);
                current->setCurrentIndex(nextIndex);
            }
            // key down
            else {
                auto nextIndex = model->index(0, 0);
                current->setCurrentIndex(nextIndex);
            }
        }
    }
    // NOT focused
    else {
        // When not focused and there's something selected,
        // it loops around instead of changing focus
        // when it has focus, it doesn't loop and changes focus to search
        if (index.isValid()) {
            QModelIndex nextIndex = index;
            if (key == Qt::Key_Up) {
                auto nextRow = index.row() - 1;
                if (nextRow < 0)
                    nextRow += rows;
                nextIndex = model->index(nextRow, 0);
            }
            else if (key == Qt::Key_Down) {
                auto nextRow = index.row() + 1;
                if (nextRow > rows - 1)
                    nextRow -= rows;
                nextIndex = model->index(nextRow, 0);
            }
            // left or right requires no change when index is valid
            current->setCurrentIndex(nextIndex);
        }
        else if (rows > 0) {
            QModelIndex nextIndex;
            if (key == Qt::Key_Up) {
                nextIndex = model->index(rows - 1, 0);
            }
            // down, left or right selects the first item when there's nothing selected
            else {
                nextIndex = model->index(0, 0);
            }
            current->setCurrentIndex(nextIndex);
        }
        current->setFocus();
    }
}

void WingMenuWidget::showCustomMenu(const QPoint& pos)
{
    QModelIndex index;
    bool isFavorites = mApplicationsStack->currentWidget() == mFavoritesView;
    if (isFavorites)
        index = mFavoritesView->indexAt(mFavoritesView->mapFrom(this, pos));
    else
        index = mApplicationsView->indexAt(mApplicationsView->mapFrom(this, pos));

    if (!index.isValid())
        return;

    XdgDesktopFile df;
    df.load(index.data(DataType::DesktopFile).toString());
    if (!df.isValid())
        return;

    auto menu = new QMenu;
    QIcon icon;
    QString text;
    QAction* a;
    if (isFavorites) {
        icon = XdgIcon::fromTheme(QSL("favorites"));
        text = tr("Remove from Favorites");
        a = menu->addAction(icon, text);
        connect(a, &QAction::triggered, this, [this, df, index] {
            if (mAskFavoriteRemove) {
                QMessageBox::StandardButton btn = QMessageBox::question(nullptr, tr("Confirm removal"), tr("Are you sure you want to remove\n%1:%2\nfrom Favorites?\n").arg(df.name(), df.fileName()));
                if (btn != QMessageBox::Yes)
                    return;
            }
            removeFromFavorites(index); });
    }
    else {
        icon = XdgIcon::fromTheme(QSL("favorites"));
        text = tr("Add to Favorites");
        a = menu->addAction(icon, text);
        if (mFavoritesList.contains(df.fileName())) {
            a->setEnabled(false);
        }
        else {
            connect(a, &QAction::triggered, this, [this, df] {
                addItemToFavorites(df);
                saveFavoritesList();
                });
        }
    }

    menu->addSeparator();
    if (df.actions().count() > 0 && df.type() == XdgDesktopFile::Type::ApplicationType) {
        for (int i = 0; i < df.actions().count(); ++i) {
            QString actionString(df.actions().at(i));
            a = menu->addAction(df.actionIcon(actionString), df.actionName(actionString));
            connect(a, &QAction::triggered, this, [this, df, actionString] {
                df.actionActivate(actionString, QStringList());
                emit hideMenu(); });
        }
        menu->addSeparator();
    }
    a = menu->addAction(XdgIcon::fromTheme(QLatin1String("desktop")), tr("Add to desktop"));
    connect(a, &QAction::triggered, [df] {
        QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        QString desktopFile = desktop + QStringLiteral("/") + df.fileName().section(QStringLiteral("/"), -1);
        if (QFile::exists(desktopFile)) {
            QMessageBox::StandardButton btn =
                QMessageBox::question(nullptr, tr("Question"), tr("A file with the same name already exists.\nDo you want to overwrite it?"));
            if (btn == QMessageBox::No)
                return;
            if (!QFile::remove(desktopFile)) {
                QMessageBox::warning(nullptr, tr("Warning"), tr("The file cannot be overwritten."));
                return;
            }
        }
        QFile::copy(df.fileName(), desktopFile); });
    a = menu->addAction(XdgIcon::fromTheme(QLatin1String("edit-copy")), tr("Copy"));
    connect(a, &QAction::triggered, this, [df] {
        QClipboard* clipboard = QApplication::clipboard();
        QMimeData* data = new QMimeData();
        data->setData(QStringLiteral("text/uri-list"), QUrl::fromLocalFile(df.fileName()).toEncoded() + QByteArray("\r\n"));
        clipboard->setMimeData(data); });
    menu->exec(mapToGlobal(pos));
}

void WingMenuWidget::buildMenu()
{
    mApplicationsModel->clear();
    mFavoritesModel->clear();
    mOtherActions.clear();
    mLeaveActions.clear();
    QLayoutItem* child;

    while ((child = mSideBox->takeAt(0)) != nullptr) {
        mCategoryBox->removeItem(child);
        delete child->widget();
        delete child;
    }
    while ((child = mCategoryBox->takeAt(0)) != nullptr) {
        mCategoryBox->removeItem(child);
        delete child->widget();
        delete child;
    }

    // Favorites
    QIcon favIcon = XdgIcon::fromTheme(QSL("favorites"), (XdgIcon::fromTheme(QSL("applications-other")), XdgIcon::defaultApplicationIcon()));
    addCategoryButton(favIcon, tr("Favorites"), QSL("Favorites"));
    for (const QString& fav : qAsConst(mFavoritesList)) {
        XdgDesktopFile df;
        df.load(fav);
        if (df.isValid())
            addItemToFavorites(df);
    }

    // All Applications
    QIcon allIcon = XdgIcon::fromTheme(QSL("applications-all"), (XdgIcon::fromTheme(QSL("applications-other")), XdgIcon::defaultApplicationIcon()));
    addCategoryButton(allIcon, tr("All Applications"), QSL("AllApplications"));

    // Categories and Sidebar
    mSideSpacer = new QSpacerItem(1, 1, QSizePolicy::Preferred, QSizePolicy::Expanding);
    mSideBox->addSpacerItem(mSideSpacer);
    mXml = mXdgMenu.xml().documentElement();
    DomElementIterator it(mXml, QString());
    while (it.hasNext()) {
        QDomElement xmlItem = it.next();
        if (xmlItem.tagName() == QSL("Menu")) {
            if (xmlItem.attribute(QSL("name")) == QSL("X-Leave"))
                parseSideActions(xmlItem);
            else
                addMenu(xmlItem);
        }
        else if (xmlItem.tagName() == QSL("AppLink"))
            parseSideActions(xmlItem);
    }
    mProxyModel->sort(0);
    buildSideButtons();
}

void WingMenuWidget::connectSignals()
{
    connect(&mXdgMenu, &XdgMenu::changed, this, [=] {
        mPlugin->buildMenu();
        });
    connect(mSearchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        filterApplications(DataType::SearchString, text);
        });
    connect(mSearchEdit, &QLineEdit::returnPressed, this, [this] {
        QModelIndex index;
        if (mApplicationsStack->currentWidget() == mApplicationsView)
            index = mApplicationsView->currentIndex();
        else
            index = mFavoritesView->currentIndex();
        indexActivated(index);
        });
    connect(mHoverTimer, &QTimer::timeout, this, &WingMenuWidget::hoverTimeout);
    connect(mSideGroup, &QActionGroup::triggered, this, &WingMenuWidget::sideActionTriggered);
    connect(mSideGroup, &QActionGroup::hovered, this, &WingMenuWidget::sideActionHovered);
    connect(mCategoryGroup, &QActionGroup::triggered, this, &WingMenuWidget::categoryActionTriggered);
    connect(mCategoryGroup, &QActionGroup::hovered, this, &WingMenuWidget::categoryActionHovered);
    connect(mApplicationsView, &QListView::customContextMenuRequested, this, &WingMenuWidget::showCustomMenu);
    connect(mFavoritesView, &QListView::customContextMenuRequested, this, &WingMenuWidget::showCustomMenu);
    connect(mApplicationsView, &ApplicationsView::indexClicked, this, &WingMenuWidget::indexActivated);
    connect(mFavoritesView, &ApplicationsView::indexClicked, this, &WingMenuWidget::indexActivated);
    connect(mFavoritesModel, &QStandardItemModel::rowsRemoved, this, &WingMenuWidget::favoritesRowRemoved);
}

QToolButton* WingMenuWidget::createSideButton(const XdgDesktopFile& df)
{
    auto icon = df.icon();
    if (icon.isNull())
        icon = XdgIcon::defaultApplicationIcon();
    auto a = new QAction(icon, df.name());
    a->setToolTip(df.name());
    a->setData(df.fileName());
    a->setCheckable(true);
    mSideGroup->addAction(a);
    auto tb = new QToolButton;
    tb->setToolButtonStyle(Qt::ToolButtonIconOnly);
    tb->setAutoRaise(true);
    tb->setIconSize(QSize(mIconSize, mIconSize));
    tb->setToolTip(df.name());
    // tb->setCheckable(true);
    tb->setDefaultAction(a);
    tb->setMinimumWidth(tb->sizeHint().width() * BTNMULTIPLIER);
    tb->setMinimumHeight(tb->sizeHint().height() * BTNMULTIPLIER);
    return tb;
}

void WingMenuWidget::buildSideButtons()
{
    mSideBox->removeItem(mSideSpacer);
    QLayoutItem* child;
    while ((child = mSideBox->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    mSideBox->addSpacerItem(mSideSpacer);

    for (const auto& option : qAsConst(mOtherActions)) {
        XdgDesktopFile df;
        df.load(option);
        auto tb = createSideButton(df);
        int index = mSideBox->indexOf(mSideSpacer);
        mSideBox->insertWidget(index, tb);
    }
    for (const auto& option : qAsConst(mLeaveActions)) {
        XdgDesktopFile df;
        df.load(option);
        auto tb = createSideButton(df);
        mSideBox->addWidget(tb);
    }
}

void WingMenuWidget::parseSideActions(const QDomElement& xml)
{
    if (xml.tagName() == QSL("Menu")) {
        DomElementIterator it(xml, QString());
        while (it.hasNext()) {
            QDomElement xmlItem = it.next();
            mLeaveActions.append(xmlItem.attribute(QSL("desktopFile")));
            addItem(xmlItem, QSL("Sidebar"));
        }
    }
    else if (xml.tagName() == QSL("AppLink")) {
        mOtherActions.append(xml.attribute(QSL("desktopFile")));
        addItem(xml, QSL("Sidebar"));
    }
}

QString WingMenuWidget::escape(QString string)
{
    return string.replace(QLatin1Char('&'), QSL("&&"));
}

void WingMenuWidget::addMenu(const QDomElement& xml)
{
    QString title = xml.attribute(QSL("title"));
    QString category = xml.attribute(QSL("name"));
    if (title.isEmpty())
        title = category;
    title = escape(title);
    QString iconName = xml.attribute(QSL("icon"));
    QIcon icon = XdgIcon::fromTheme(iconName, XdgIcon::fromTheme(QSL("applications-other"), XdgIcon::defaultApplicationIcon()));

    addCategoryButton(icon, title, category);
    DomElementIterator it(xml, QString());
    while (it.hasNext()) {
        QDomElement xmlItem = it.next();
        if (xmlItem.tagName() == QSL("Menu"))
            addMenu(xmlItem);
        else if (xmlItem.tagName() == QSL("AppLink"))
            addItem(xmlItem, category);
    }
}

void WingMenuWidget::addCategoryButton(const QIcon& icon, const QString& title, const QString& category)
{
    auto tb = new QToolButton;
    auto action = new QAction(icon, title, tb);
    action->setData(category);
    action->setCheckable(true);
    mCategoryGroup->addAction(action);
    tb->setDefaultAction(action);
    tb->setObjectName(QSL("CategoryButton"));
    // tb->setCheckable(true);
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tb->setAutoRaise(true);
    tb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tb->setIconSize(QSize(mIconSize, mIconSize));
    tb->setMinimumHeight(tb->sizeHint().height() * BTNMULTIPLIER);
    tb->setMinimumWidth(tb->sizeHint().width() * BTNMULTIPLIER);
    mCategoryBox->addWidget(tb);
}

void WingMenuWidget::addItem(const QDomElement& xml, const QString& category)
{
    XdgDesktopFile df;
    df.load(xml.attribute(QSL("desktopFile")));
    addItem(df, category);
}

void WingMenuWidget::addItem(const XdgDesktopFile& df, const QString& category)
{
    auto name = df.name();
    auto list = mApplicationsModel->findItems(name);
    for (auto i = list.cbegin(); i != list.cend(); ++i) {
        if ((*i)->data(DataType::DesktopFile) == df.fileName()) {
            auto cat = (*i)->data(DataType::Category).toString();
            (*i)->setData(QSL("%1;%2").arg(cat, category), DataType::Category);
            setItemToolTip(*i);
            return;
        }
    }

    auto item = new QStandardItem(df.icon(), name);
    auto comment = df.comment();
    if (comment.isEmpty())
        comment = df.localizedValue(QSL("genericName")).toString();

    auto exec = df.value(QSL("Exec")).toString();
    auto execBin = exec.split(QLatin1Char(' ')).first().split(QLatin1Char('/')).last();
    auto searchString = QSL("%1;%2;%3").arg(name, comment, execBin);
    item->setDropEnabled(false);
    item->setData(comment, DataType::Comment);
    item->setData(category, DataType::Category);
    item->setData(df.fileName(), DataType::DesktopFile);
    item->setData(exec, DataType::Exec);
    item->setData(searchString, DataType::SearchString);

    setItemToolTip(item);
    mApplicationsModel->appendRow(item);
}

void WingMenuWidget::setItemToolTip(QStandardItem* item)
{
    QString tooltip = QSL("%1 : %2\n%3 : %4\n%5 : %6\n%7 : %8\n%9 : %10")
        .arg(tr("Name"),
            item->data(Qt::DisplayRole).toString(),
            tr("Comment"),
            item->data(DataType::Comment).toString(),
            tr("File"), item->data(DataType::DesktopFile).toString(),
            tr("Category"), item->data(DataType::Category).toString(),
            tr("Exec"), item->data(DataType::Exec).toString());
    item->setToolTip(tooltip);
}

void WingMenuWidget::settingsChanged()
{
    if (mFavoritesList.isEmpty())
        mFavoritesList = (mPlugin->settings()->value(QSL("favoritesList"), {})).toStringList();

    auto menuFile = mPlugin->settings()->value(QSL("menuFile"), QSL(DEFAULT_MENU_FILE)).toString();
    mCategoryLeft = mPlugin->settings()->value(QSL("categoryLeft"), true).toBool();
    mSearchBottom = mPlugin->settings()->value(QSL("searchBottom"), true).toBool();
    mSidebarLeft = mPlugin->settings()->value(QSL("sidebarLeft"), true).toBool();
    mReverseSidebar = mPlugin->settings()->value(QSL("reverseSidebar"), false).toBool();
    mSwitchOnHover = mPlugin->settings()->value(QSL("switchOnHover"), true).toBool();
    auto hoverDelay = mPlugin->settings()->value(QSL("hoverDelay"), 200).toInt();
    mAskFavoriteRemove = mPlugin->settings()->value(QSL("askFavoriteRemove"), false).toBool();

    hoverDelay = qBound(50, hoverDelay, 1000);
    if (mSwitchOnHover) {
        if (hoverDelay != mHoverDelay) {
            mHoverTimer->setInterval(hoverDelay);
            mHoverDelay = hoverDelay;
        }
    }
    else {
        mHoverTimer->stop();
    }

    if (mCategoryLeft)
        mMenuBox->setDirection(QBoxLayout::LeftToRight);
    else
        mMenuBox->setDirection(QBoxLayout::RightToLeft);

    if (mSearchBottom)
        mMainBox->setDirection(QBoxLayout::TopToBottom);
    else
        mMainBox->setDirection(QBoxLayout::BottomToTop);

    if (mSidebarLeft)
        mRootBox->setDirection(QBoxLayout::LeftToRight);
    else
        mRootBox->setDirection(QBoxLayout::RightToLeft);

    if (mReverseSidebar)
        mSideBox->setDirection(QBoxLayout::BottomToTop);
    else
        mSideBox->setDirection(QBoxLayout::TopToBottom);


    if (mMenuFile != menuFile) {
        mMenuFile = menuFile;
        bool res = mXdgMenu.read(mMenuFile);

        if (!res)
            QMessageBox::warning(nullptr, QSL("Parse error"), mXdgMenu.errorString());
        QTimer::singleShot(100, mPlugin, &WingMenuPlugin::buildMenu);
    }
}

void WingMenuWidget::focusLineEdit()
{
    mSearchEdit->setReadOnly(false);
    mSearchEdit->setFocus();
}

void WingMenuWidget::removeSpaces(QLayout* layout)
{
    layout->setSpacing(0);
    layout->setMargin(0);
}

void WingMenuWidget::removeSpaces(QWidget* widget)
{
    widget->setContentsMargins(0, 0, 0, 0);
}

void WingMenuWidget::filterApplications(DataType type, const QString& filterString)
{
    if (type == DataType::Category) {
        mLastCategory = filterString;
        mCategoryWidget->show();
        if (filterString == QSL("Favorites")) {
            mApplicationsStack->setCurrentWidget(mFavoritesView);
        }
        else if (filterString == QSL("AllApplications")) {
            mApplicationsStack->setCurrentWidget(mApplicationsView);
            mProxyModel->setFilterRole(type);
            mProxyModel->setFilterWildcard(QSL("*"));
        }
        else {
            mApplicationsStack->setCurrentWidget(mApplicationsView);
            mProxyModel->setFilterRole(type);
            mProxyModel->setFilterWildcard(filterString);
        }

    } // Search
    else if (type == DataType::SearchString) {
        if (filterString.isEmpty()) {
            filterApplications(DataType::Category, mLastCategory);
        }
        else {
            mApplicationsStack->setCurrentWidget(mApplicationsView);
            mCategoryWidget->hide();
            mProxyModel->setFilterRole(type);
            mProxyModel->setFilterWildcard(filterString);
            // select first result every time search term changes
            if (mProxyModel->rowCount() > 0)
                mApplicationsView->setCurrentIndex(mProxyModel->index(0, 0));
        }
    }
}

void WingMenuWidget::sideActionTriggered(QAction* action)
{
    XdgDesktopFile df;
    df.load(action->data().toString());
    df.startDetached();
    mPlugin->hideMenu();
}

void WingMenuWidget::sideActionHovered(QAction* action)
{
    action->setChecked(true);
}

void WingMenuWidget::uncheckSideActions()
{
    for (const auto a : mSideGroup->actions()) {
        a->setChecked(false);
    }
}

void WingMenuWidget::categoryActionTriggered(QAction* action)
{
    auto category = action->data().toString();
    filterApplications(DataType::Category, category);
}

void WingMenuWidget::categoryActionHovered(QAction* action)
{
    if (mSwitchOnHover) {
        mHoverTimer->start();
        mHoveredAction = action;
    }
}

void WingMenuWidget::hoverTimeout()
{
    auto tb = qobject_cast<QToolButton*>(mHoveredAction->parentWidget());
    if (nullptr != tb && mCategoryGroup->checkedAction() != mHoveredAction && tb->underMouse())
        mHoveredAction->activate(QAction::ActionEvent::Trigger);
}

void WingMenuWidget::addItemToFavorites(const XdgDesktopFile& df)
{
    auto name = df.name();
    auto item = new QStandardItem(df.icon(), name);
    auto comment = df.comment();
    if (comment.isEmpty())
        comment = df.localizedValue(QSL("genericName")).toString();
    auto category = QSL("Favorites");
    auto exec = df.value(QSL("Exec")).toString();
    item->setDropEnabled(false);
    item->setData(comment, DataType::Comment);
    item->setData(category, DataType::Category);
    item->setData(df.fileName(), DataType::DesktopFile);
    item->setData(exec, DataType::Exec);

    setItemToolTip(item);
    mFavoritesModel->appendRow(item);
    mFavoritesList.append(df.fileName());
}

void WingMenuWidget::saveFavoritesList()
{
    mPlugin->settings()->setValue(QSL("favoritesList"), QVariant::fromValue(mFavoritesList));
}

void WingMenuWidget::removeFromFavorites(const QModelIndex& index)
{
    auto row = index.row();
    mFavoritesModel->removeRow(row);
}

void WingMenuWidget::favoritesRowRemoved()
{
    auto count = mFavoritesModel->rowCount();
    QStringList newList;
    for (int i = 0; i < count; ++i) {
        auto item = mFavoritesModel->item(i);
        auto fileName = item->data(DataType::DesktopFile).toString();
        if (!fileName.isEmpty())
            newList << fileName;
    }
    mFavoritesList = newList;
    saveFavoritesList();
}
// Accepting click release to stop propagating to QMenu,
// so it doesn't close when clicking on empty space
void WingMenuWidget::mouseReleaseEvent(QMouseEvent* event)
{
    event->accept();
}
