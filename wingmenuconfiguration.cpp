#include "wingmenuconfiguration.h"
#include "ui_wingmenuconfiguration.h"

#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QSpinBox>
#include <QTimer>

#include <lxqt-globalkeys.h>

WingMenuConfiguration::WingMenuConfiguration(PluginSettings& settings,
    GlobalKeyShortcut::Action* shortcut,
    const QString& defaultShortcut,
    QWidget* parent)
    : QDialog(parent),
    ui(new Ui::WingMenuConfiguration),
    mSettings(settings),
    mDefaultShortcut(defaultShortcut),
    mShortcut(shortcut)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QStringLiteral("WingMenuConfigurationWindow"));
    ui->setupUi(this);
    loadSettings();

    connect(ui->shortcutEd, &ShortcutSelector::shortcutGrabbed, this, &WingMenuConfiguration::shortcutChanged);
    connect(ui->shortcutEd->addMenuAction(tr("Reset")), &QAction::triggered, this, &WingMenuConfiguration::shortcutReset);
    connect(mShortcut, &GlobalKeyShortcut::Action::shortcutChanged, this, &WingMenuConfiguration::globalShortcutChanged);
    connect(ui->buttons, &QDialogButtonBox::clicked, this, &WingMenuConfiguration::dialogButtonsAction);

    connect(ui->iconCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QStringLiteral("showIcon"), checked); });
    connect(ui->iconLE, &QLineEdit::textChanged, this, [this](QString text) { this->settings().setValue(QStringLiteral("icon"), text); });
    connect(ui->textCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QStringLiteral("showText"), checked); });
    connect(ui->textLE, &QLineEdit::textChanged, this, [this](QString text) { this->settings().setValue(QStringLiteral("text"), text); });
    connect(ui->menuFileLE, &QLineEdit::textChanged, this, [this](QString text) { this->settings().setValue(QStringLiteral("menuFile"), text); });
    connect(ui->switchOnHoverCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QStringLiteral("switchOnHover"), checked); });
    connect(ui->hoverDelaySB, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) { this->settings().setValue(QStringLiteral("hoverDelay"), value); });
    connect(ui->categoryLeftCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QStringLiteral("categoryLeft"), checked); });
    connect(ui->searchBottomCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QStringLiteral("searchBottom"), checked); });
    connect(ui->sidebarLeftCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QStringLiteral("sidebarLeft"), checked); });
    connect(ui->reverseSidebarCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QStringLiteral("reverseSidebar"), checked); });
    connect(ui->appLayoutCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) { this->settings().setValue(QStringLiteral("appLayout"), index); });
    connect(ui->askFavoriteRemoveCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QStringLiteral("askFavoriteRemove"), checked); });
    connect(ui->iconPB, &QPushButton::clicked, this, [this] {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Choose Icon File"),
            QDir::homePath(),
            tr("Images (*.png *.xpm *.svg)"));
        if (!fileName.isEmpty())
            ui->iconLE->setText(fileName); });
    connect(ui->menuFilePB, &QPushButton::clicked, this, &WingMenuConfiguration::chooseMenuFile);
}
void WingMenuConfiguration::chooseMenuFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Choose Menu File"),
        settings().value(QStringLiteral("menuFile"), QStringLiteral("/etc/xdg/menus/lxqt-applications.menu")).toString(),
        tr("Menu files (*.menu)"));
    if (!fileName.isEmpty())
        ui->menuFileLE->setText(fileName);
}
void WingMenuConfiguration::globalShortcutChanged(const QString& /*oldShortcut*/, const QString& newShortcut)
{
    ui->shortcutEd->setText(newShortcut);
}

void WingMenuConfiguration::shortcutChanged(const QString& value)
{
    if (mShortcut) {
        mShortcut->changeShortcut(value);
    }
}

void WingMenuConfiguration::shortcutReset()
{
    shortcutChanged(mDefaultShortcut);
}

void WingMenuConfiguration::loadSettings() const
{
    ui->shortcutEd->setText(mShortcut->shortcut());

    auto showIcon = settings().value(QStringLiteral("showIcon"), true).toBool();
    auto icon = settings().value(QStringLiteral("icon"), QStringLiteral("wing-lxqt")).toString();
    auto showText = settings().value(QStringLiteral("showText"), true).toBool();
    auto text = settings().value(QStringLiteral("text"), tr("Menu")).toString();
    auto menuFile = settings().value(QStringLiteral("menuFile"), QStringLiteral("/etc/xdg/menus/lxqt-applications.menu")).toString();
    auto switchOnHover = settings().value(QStringLiteral("switchOnHover"), true).toBool();
    auto hoverDelay = settings().value(QStringLiteral("hoverDelay"), 200).toInt();
    auto categoryLeft = settings().value(QStringLiteral("categoryLeft"), true).toBool();
    auto searchBottom = settings().value(QStringLiteral("searchBottom"), true).toBool();
    auto sidebarLeft = settings().value(QStringLiteral("sidebarLeft"), true).toBool();
    auto reverseSidebar = settings().value(QStringLiteral("reverseSidebar"), false).toBool();
    auto appLayout = settings().value(QStringLiteral("appLayout"), 0).toInt();
    auto askFavoriteRemove = settings().value(QStringLiteral("askFavoriteRemove"), false).toBool();

    ui->iconCB->setChecked(showIcon);
    ui->iconLE->setText(icon);
    ui->textCB->setChecked(showText);
    ui->textLE->setText(text);
    ui->menuFileLE->setText(menuFile);
    ui->switchOnHoverCB->setChecked(switchOnHover);
    ui->hoverDelaySB->setValue(hoverDelay);
    ui->categoryLeftCB->setChecked(categoryLeft);
    ui->searchBottomCB->setChecked(searchBottom);
    ui->sidebarLeftCB->setChecked(sidebarLeft);
    ui->reverseSidebarCB->setChecked(reverseSidebar);
    ui->appLayoutCombo->setCurrentIndex(appLayout);
    ui->askFavoriteRemoveCB->setChecked(askFavoriteRemove);
}

PluginSettings& WingMenuConfiguration::settings() const
{
    return mSettings;
}

void WingMenuConfiguration::dialogButtonsAction(QAbstractButton* btn)
{
    QDialogButtonBox* box = qobject_cast<QDialogButtonBox*>(btn->parent());
    if (box && box->buttonRole(btn) == QDialogButtonBox::ResetRole) {
        mSettings.loadFromCache();
        loadSettings();
    }
    else {
        close();
    }
}
