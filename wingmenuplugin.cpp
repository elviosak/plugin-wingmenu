#include "wingmenuplugin.h"
#include "wingmenuconfiguration.h"
#include "wingmenuwidget.h"

#include <QDBusConnectionInterface>
#include <QIcon>
#include <QMessageBox>
#include <QTimer>
#include <QWidgetAction>

#include <lxqtglobalkeys.h>

#define SERVICE_NAME QSL("org.lxqt.panel.wingmenu")

int WingMenuPlugin::sCounter = 0;
QDBusConnection WingMenuPlugin::sConnection = QDBusConnection::connectToBus(QDBusConnection::SessionBus, QSL("plugin-wingmenu"));

WingMenuPlugin::WingMenuPlugin(const ILXQtPanelPluginStartupInfo& startupInfo)
    : QObject()
    , ILXQtPanelPlugin(startupInfo)
    , mShortcut(nullptr)
    , mMenu(nullptr)
    , mMenuWidget(nullptr)
    , mMenuAction(nullptr)
    , mMenuFile(QString())
    , mXdgMenu(new XdgMenu())
{
    auto logDir = settings()->value(QSL("logDir"), QString()).toString();
    mXdgMenu->setEnvironments(QStringList() << QSL("X-LXQT") << QSL("LXQt"));
    mXdgMenu->setLogDir(logDir);
    mWidget = new QToolButton;
    mWidget->setObjectName(QSL("MainMenu"));
    mWidget->setAutoRaise(true);
    mWidget->setMinimumWidth(1);
    mWidget->setMinimumHeight(1);

    connect(mWidget, &QToolButton::clicked, this, &WingMenuPlugin::showHideMenu);
    settingsChanged();
    // buildMenu();
    setupShortcut();
    connect(mXdgMenu, &XdgMenu::changed, this, &WingMenuPlugin::buildMenu);

    sCounter++;
    if (sCounter == 1) {
        registerService();
    }
    registerObject();
}

WingMenuPlugin::~WingMenuPlugin()
{
    sCounter--;
    qDebug() << "DESTRUCTOR" << sCounter << settings()->group();
    if (sCounter <= 0) {
        sConnection.unregisterService(SERVICE_NAME);
    }
}

void WingMenuPlugin::buildMenu()
{
    if (nullptr != mMenu) {
        mMenu->deleteLater();
    }

    if (nullptr != mMenuWidget) {
        mMenuWidget->deleteLater();
    }

    mMenu = new QMenu(mWidget);
    mMenu->setAttribute(Qt::WA_AlwaysShowToolTips);
    mMenu->setAttribute(Qt::WA_TranslucentBackground);

    mMenu->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Popup);

    mMenuWidget = new WingMenuWidget(this, mXdgMenu);
    mMenuWidget->ensurePolished();

    mMenu->setFocusProxy(mMenuWidget);

    mMenuAction = new QWidgetAction(mMenu);
    mMenuAction->setDefaultWidget(mMenuWidget);
    mMenu->addAction(mMenuAction);
    mMenu->ensurePolished();
    connect(mMenuWidget, &WingMenuWidget::hideMenu, this, &WingMenuPlugin::hideMenu);
    connect(mMenu, &QMenu::aboutToHide, mMenuWidget, &WingMenuWidget::onHide);
    connect(mMenu, &QMenu::aboutToShow, mMenuWidget, &WingMenuWidget::onShow);
}

void WingMenuPlugin::settingsChanged()
{
    mShowIcon = settings()->value(QSL("showIcon"), true).toBool();
    mIcon = settings()->value(QSL("icon"), QSL("wing-lxqt")).toString();
    mShowText = settings()->value(QSL("showText"), true).toBool();
    mText = settings()->value(QSL("text"), tr("Menu")).toString();
    auto appLayout = settings()->value(QSL("appLayout"), AppLayout::ListNameAndDescription).value<AppLayout::Layout>();
    auto menuFile = settings()->value(QSL("menuFile"), DEFAULT_MENU_FILE).toString();

    if (!mShowIcon && !mShowText) {
        mWidget->setIcon(QIcon());
        mWidget->setText(QString());
        mWidget->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
    }
    else {
        mWidget->show();
        if (mShowIcon && mShowText) {
            mWidget->setIcon(XdgIcon::fromTheme(mIcon, QIcon(mIcon)));
            mWidget->setText(mText);
            mWidget->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
        }
        else if (mShowIcon) {
            mWidget->setIcon(XdgIcon::fromTheme(mIcon, QIcon(mIcon)));
            mWidget->setText(QString());

            mWidget->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
        }
        else if (mShowText) {
            mWidget->setIcon(QIcon());
            mWidget->setText(mText);
            mWidget->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
        }
    }

    if ((mAppLayout != appLayout) || (mMenuFile != menuFile)) {
        mAppLayout = appLayout;
        mMenuFile = menuFile;
        bool res = mXdgMenu->read(mMenuFile);
        if (!res) {
            QMessageBox::warning(nullptr, QSL("Parse error"), mXdgMenu->errorString());
        }
        buildMenu();
    }
    mMenuWidget->settingsChanged();
}

void WingMenuPlugin::setupShortcut()
{
    mShortcut = GlobalKeyShortcut::Client::instance()->addAction(QString {}, QSL("/panel/%1/show_hide").arg(settings()->group()), tr("Show/hide main menu"), this);
    if (mShortcut) {
        connect(mShortcut, &GlobalKeyShortcut::Action::registrationFinished, this, [this] {
            if (mShortcut->shortcut().isEmpty()){
                mShortcut->changeShortcut(DEFAULT_SHORTCUT);
} });
        connect(mShortcut, &GlobalKeyShortcut::Action::activated, this, [this] { showHideMenu(); });
    }
}

void WingMenuPlugin::registerService()
{
    auto reply = sConnection.interface()->registerService(SERVICE_NAME, QDBusConnectionInterface::DontQueueService, QDBusConnectionInterface::AllowReplacement);
    if (reply.value() == QDBusConnectionInterface::ServiceRegistered) {
        qInfo() << "D-Bus Service" << SERVICE_NAME << "registered succesfully.";
    }
    else {
        qInfo() << "Service" << SERVICE_NAME << "exists, will wait for it to unregister.";
        mDBusMessage = tr("Failed to Register D-Bus Service");
    }
}
void WingMenuPlugin::registerObject()
{
    auto objectPath = QSL("/%1").arg(settings()->group());
    if (sConnection.registerObject(objectPath, this, QDBusConnection::QDBusConnection::ExportAllSlots)) {
        qInfo() << "D-Bus Object" << objectPath << "registered succesfully.";
        mDBusMessage = QSL("qdbus %1 %2 toggle").arg(SERVICE_NAME, objectPath);
    }
    else {
        qInfo() << "D-Bus Object" << objectPath << "could not be registered.";
        mDBusMessage = tr("Failed to Register D-Bus Object");
    }
}

void WingMenuPlugin::showMenu()
{
    if (!mMenu) {
        buildMenu();
    }
    willShowWindow(mMenu);
    mMenu->popup(calculatePopupWindowPos(mMenu->sizeHint()).topLeft());
    mMenuWidget->focusLineEdit();
}

void WingMenuPlugin::hideMenu()
{
    mMenu->hide();
}

void WingMenuPlugin::showHideMenu()
{
    if (!mMenu) {
        return;
    }

    if (mMenu->isVisible()) {
        hideMenu();
    }
    else {
        showMenu();
    }
}

void WingMenuPlugin::showHideMenuDelayed()
{
    if (!mMenu) {
        return;
    }

    if (mMenu->isVisible()) {
        hideMenu();
    }
    else {
        QTimer::singleShot(200, this, &WingMenuPlugin::showMenu);
    }
}

void WingMenuPlugin::realign()
{
    mWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QDialog* WingMenuPlugin::configureDialog()
{
    return new WingMenuConfiguration(settings(), mShortcut, mXdgMenu, mDBusMessage);
}
