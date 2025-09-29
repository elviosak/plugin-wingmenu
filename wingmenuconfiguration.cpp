#include "wingmenuconfiguration.h"
#include "common.h"
#include "ui_wingmenuconfiguration.h"

#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>

#include <XdgDesktopFile>
#include <XdgMenu>
#include <XmlHelper>
#include <lxqt-globalkeys.h>

WingMenuConfiguration::WingMenuConfiguration(PluginSettings& settings,
    GlobalKeyShortcut::Action* shortcut,
    XdgMenu* xdgMenu,
    QString dBusMessage,
    QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::WingMenuConfiguration)
    , mSettings(settings)
    , mXdgMenu(xdgMenu)
    , mShortcut(shortcut)
    , mLeaveActionsModel(new QStandardItemModel)
    , mDesktopFilesDir(QSL("%1/wingmenu").arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)))
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QSL("WingMenuConfigurationWindow"));
    ui->setupUi(this);
    ui->leaveActionsView->setModel(mLeaveActionsModel);
    ui->locationLB->setText(tr("Files are stored in: %1").arg(mDesktopFilesDir));

    loadSettings();
    connect(ui->shortcutEd, &ShortcutSelector::shortcutGrabbed, this, &WingMenuConfiguration::shortcutChanged);
    connect(ui->shortcutEd->addMenuAction(tr("Reset")), &QAction::triggered, this, &WingMenuConfiguration::shortcutReset);
    connect(mShortcut, &GlobalKeyShortcut::Action::shortcutChanged, this, &WingMenuConfiguration::globalShortcutChanged);
    connect(ui->buttons, &QDialogButtonBox::clicked, this, &WingMenuConfiguration::dialogButtonsAction);

    connect(ui->iconCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QSL("showIcon"), checked); });
    connect(ui->iconLE, &QLineEdit::textChanged, this, [this](QString text) { this->settings().setValue(QSL("icon"), text); });
    connect(ui->textCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QSL("showText"), checked); });
    connect(ui->textLE, &QLineEdit::textChanged, this, [this](QString text) { this->settings().setValue(QSL("text"), text); });
    connect(ui->menuFileLE, &QLineEdit::textChanged, this, [this](QString text) { this->settings().setValue(QSL("menuFile"), text); });
    connect(ui->switchOnHoverCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QSL("switchOnHover"), checked); });
    connect(ui->hoverDelaySB, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) { this->settings().setValue(QSL("hoverDelay"), value); });
    connect(ui->categoryLeftCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QSL("categoryLeft"), checked); });
    connect(ui->searchBottomCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QSL("searchBottom"), checked); });
    connect(ui->sidebarLeftCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QSL("sidebarLeft"), checked); });
    connect(ui->reverseSidebarCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QSL("reverseSidebar"), checked); });
    connect(ui->appLayoutCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) { this->settings().setValue(QSL("appLayout"), index); });
    connect(ui->askFavoriteRemoveCB, &QCheckBox::toggled, this, [this](bool checked) { this->settings().setValue(QSL("askFavoriteRemove"), checked); });
    connect(ui->iconPB, &QPushButton::clicked, this, [this] {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Choose Icon File"),
            QDir::homePath(),
            tr("Images (*.png *.xpm *.svg)"));
        if (!fileName.isEmpty()) {
            ui->iconLE->setText(fileName);
        }
    });
    connect(ui->menuFilePB, &QPushButton::clicked, this, &WingMenuConfiguration::chooseMenuFile);

    connect(ui->customizeLeaveGB, &QGroupBox::toggled, this, &WingMenuConfiguration::customizeLeave);
    connect(ui->loadFromMenuPB, &QPushButton::clicked, this, &WingMenuConfiguration::loadFromMenu);
    connect(ui->addDesktopFilePB, &QPushButton::clicked, this, &WingMenuConfiguration::addDesktopFile);
    connect(ui->newActionPB, &QPushButton::clicked, this, &WingMenuConfiguration::newAction);
    connect(ui->editActionPB, &QPushButton::clicked, this, &WingMenuConfiguration::editAction);
    connect(ui->upActionPB, &QPushButton::clicked, this, &WingMenuConfiguration::upAction);
    connect(ui->downActionPB, &QPushButton::clicked, this, &WingMenuConfiguration::downAction);
    connect(ui->removeActionPB, &QPushButton::clicked, this, &WingMenuConfiguration::removeAction);
    connect(ui->leaveActionsView, &QListView::activated, this, &WingMenuConfiguration::actionActivated);
    connect(mLeaveActionsModel, &QStandardItemModel::rowsRemoved, this, &WingMenuConfiguration::saveLeaveActions);
    
    ui->dBusLE->setText(dBusMessage);
}

void WingMenuConfiguration::actionActivated(const QModelIndex& index)
{
    auto item = mLeaveActionsModel->itemFromIndex(index);
    openEditDialog(item->data().toString());
}
QStandardItem* WingMenuConfiguration::createItem(const QString& fileName) const
{
    XdgDesktopFile df;
    df.load(fileName);
    auto name = df.name();
    auto icon = df.icon();
    auto item = new QStandardItem(icon, name);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    item->setToolTip(df.value(QSL("Exec")).toString());
    item->setData(fileName);
    item->setDropEnabled(false);
    return item;
}

void WingMenuConfiguration::copyDesktopFile(const QString& fileName)
{
    XdgDesktopFile df;
    if (df.load(fileName)) {
        auto newFile = newFileName();
        df.save(newFile);
        auto item = createItem(newFile);
        mLeaveActionsModel->appendRow(item);
    }
}

void WingMenuConfiguration::loadLeaveActions() const
{
    mLeaveActionsModel->clear();
    auto leaveActions = settings().value(QSL("leaveActions"), QStringList()).toStringList();
    for (const QString& fileName : leaveActions) {
        mLeaveActionsModel->appendRow(createItem(fileName));
    }
}

void WingMenuConfiguration::saveLeaveActions()
{
    QStringList leaveActions;
    for (int i = 0; i < mLeaveActionsModel->rowCount(); ++i) {
        leaveActions << mLeaveActionsModel->item(i)->data().toString();
    }
    settings().setValue(QSL("leaveActions"), leaveActions);
}

void WingMenuConfiguration::customizeLeave(bool customize)
{
    settings().setValue(QSL("customizeLeave"), customize);
}

void WingMenuConfiguration::loadFromMenu()
{
    auto mXml = mXdgMenu->xml().documentElement();
    DomElementIterator it(mXml, QString());
    while (it.hasNext()) {
        QDomElement xml = it.next();
        if (xml.tagName() == QSL("Menu") && xml.attribute(QSL("name")) == QSL("X-Leave")) {
            DomElementIterator it(xml, QString());
            while (it.hasNext()) {
                QDomElement xmlItem = it.next();
                copyDesktopFile(xmlItem.attribute(QSL("desktopFile")));
            }
        }
    }
    saveLeaveActions();
}

void WingMenuConfiguration::addDesktopFile()
{
    // When setting filter to Desktop file("*.desktop"), the desktop files aren't shown,
    // probably because they are displayed with the "Name" field from the file
    // instead of the filename, so unless i find a workaround, it has to show all files.
    QString fileName = QFileDialog::getOpenFileName(this, tr("Choose Desktop File"),
        QDir::homePath(),
        tr("All files (*)"));

    if (!fileName.isEmpty()) {
        XdgDesktopFile df;
        if (df.load(fileName)) {
            copyDesktopFile(fileName);
            saveLeaveActions();
        } else {
            QMessageBox::warning(this, tr("Invalid desktop file"), tr("Selected file: %1 is invalid.").arg(fileName));
        }
    }
}

void WingMenuConfiguration::newAction()
{
    openEditDialog();
}

void WingMenuConfiguration::editAction()
{
    auto index = ui->leaveActionsView->currentIndex();
    if (index.isValid()) {
        auto item = mLeaveActionsModel->itemFromIndex(index);
        openEditDialog(item->data().toString());
    } else {
        QMessageBox::warning(this, tr("No item selected"), tr("Please select an item to edit."));
    }
}

void WingMenuConfiguration::upAction()
{
    auto current = ui->leaveActionsView->currentIndex();
    if (current.isValid() && current.row() > 0) {
        int row = current.row();
        mLeaveActionsModel->insertRow(row - 1, mLeaveActionsModel->takeItem(row));
        mLeaveActionsModel->removeRow(row + 1);
        ui->leaveActionsView->setCurrentIndex(mLeaveActionsModel->index(row - 1, 0));
    }
}

void WingMenuConfiguration::downAction()
{
    auto current = ui->leaveActionsView->currentIndex();
    if (current.isValid() && current.row() < mLeaveActionsModel->rowCount() - 1) {
        int row = current.row();
        mLeaveActionsModel->insertRow(row + 2, mLeaveActionsModel->takeItem(row));
        mLeaveActionsModel->removeRow(row);
        ui->leaveActionsView->setCurrentIndex(mLeaveActionsModel->index(row + 1, 0));
    }
}

void WingMenuConfiguration::removeAction()
{
    auto index = ui->leaveActionsView->currentIndex();
    if (index.isValid()) {
        auto item = mLeaveActionsModel->itemFromIndex(index);
        QFile file(item->data().toString());
        file.remove();
        mLeaveActionsModel->removeRow(index.row());
    } else {
        QMessageBox::warning(this, tr("No item selected"), tr("Please select an item to remove."));
    }
}

void WingMenuConfiguration::openEditDialog(const QString& fileName)
{
    auto d = new QDialog;
    int fh = d->fontMetrics().height();
    d->setWindowTitle(tr("Edit Action"));
    d->resize(20 * fh, 10 * fh);
    auto vbox = new QVBoxLayout(d);
    auto form = new QFormLayout;
    vbox->addLayout(form);
    vbox->addStretch(0);
    auto nameLine = new QLineEdit;
    auto iconLine = new QLineEdit;
    auto execLine = new QLineEdit;
    if (!fileName.isEmpty()) {
        XdgDesktopFile df;
        if (df.load(fileName)) {
            nameLine->setText(df.name());
            iconLine->setText(df.iconName());
            execLine->setText(df.value(QSL("Exec")).toString());
        }
    }
    nameLine->setMinimumWidth(12 * fh);
    form->addRow(tr("Name"), nameLine);
    form->addRow(tr("Icon"), iconLine);
    form->addRow(tr("Command"), execLine);

    auto btnBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, d);
    vbox->addWidget(btnBox);

    // form->addRow(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, this,
        [=, this] {
            if (!nameLine->text().isEmpty()
                && !iconLine->text().isEmpty()
                && !execLine->text().isEmpty()) {
                XdgDesktopFile df(XdgDesktopFile::ApplicationType, nameLine->text(), execLine->text());
                df.setValue(QSL("Icon"), iconLine->text());
                saveDesktopFile(nameLine->text(), iconLine->text(), execLine->text(), fileName);
                d->close();
            } else {
                QStringList textList;
                if (nameLine->text().isEmpty()) {
                    textList << tr("\"Name\" field is empty.");
                }
                if (iconLine->text().isEmpty()) {
                    textList << tr("\"Icon\" field is empty.");
                }
                if (execLine->text().isEmpty()) {
                    textList << tr("\"Command\" field is empty.");
                }
                QMessageBox::warning(d, tr("Please fill all fields."), textList.join(QLatin1Char('\n')));
            }
        });
    connect(btnBox, &QDialogButtonBox::rejected, this,
        [d] {
            d->close();
        });
    d->exec();
}

void WingMenuConfiguration::saveDesktopFile(const QString& name, const QString& icon, const QString& exec, const QString& fileName)
{
    if (fileName.isEmpty()) {
        auto newFile = newFileName();
        XdgDesktopFile df(XdgDesktopFile::ApplicationType, name, exec);
        df.setValue(QSL("Icon"), icon);
        df.save(newFile);
        mLeaveActionsModel->appendRow(createItem(newFile));
    } else {
        for (int i = 0; i < mLeaveActionsModel->rowCount(); ++i) {
            auto item = mLeaveActionsModel->item(i);
            if (item->data().toString() == fileName) {
                XdgDesktopFile df;
                df.load(fileName);
                df.setLocalizedValue(QSL("Name"), name);
                df.setValue(QSL("Icon"), icon);
                df.setValue(QSL("Exec"), exec);
                df.save(fileName);
                item->setText(df.name());
                item->setIcon(df.icon());
                break;
            }
        }
    }
    saveLeaveActions();
}

QString WingMenuConfiguration::newFileName()
{
    QDir dir(mDesktopFilesDir);
    dir.mkpath(QSL("."));
    for (int i = 0;; ++i) {
        QString fileName = QSL("%1.desktop").arg(i);
        if (!dir.exists(fileName)) {
            return dir.absolutePath() + QLatin1Char('/') + fileName;
        }
    }
}

void WingMenuConfiguration::chooseMenuFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Choose Menu File"),
        settings().value(QSL("menuFile"), DEFAULT_MENU_FILE).toString(),
        tr("Menu files (*.menu)"));
    if (!fileName.isEmpty()) {
        ui->menuFileLE->setText(fileName);
    }
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
    shortcutChanged(DEFAULT_SHORTCUT);
}

void WingMenuConfiguration::loadSettings() const
{
    ui->shortcutEd->setText(mShortcut->shortcut());

    auto showIcon = settings().value(QSL("showIcon"), true).toBool();
    auto icon = settings().value(QSL("icon"), QSL("wing-lxqt")).toString();
    auto showText = settings().value(QSL("showText"), true).toBool();
    auto text = settings().value(QSL("text"), tr("Menu")).toString();
    auto menuFile = settings().value(QSL("menuFile"), DEFAULT_MENU_FILE).toString();
    auto switchOnHover = settings().value(QSL("switchOnHover"), true).toBool();
    auto hoverDelay = settings().value(QSL("hoverDelay"), 200).toInt();
    auto categoryLeft = settings().value(QSL("categoryLeft"), true).toBool();
    auto searchBottom = settings().value(QSL("searchBottom"), true).toBool();
    auto sidebarLeft = settings().value(QSL("sidebarLeft"), true).toBool();
    auto reverseSidebar = settings().value(QSL("reverseSidebar"), false).toBool();
    auto appLayout = settings().value(QSL("appLayout"), 0).toInt();
    auto askFavoriteRemove = settings().value(QSL("askFavoriteRemove"), false).toBool();
    auto customizeLeave = settings().value(QSL("customizeLeave"), false).toBool();

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
    ui->customizeLeaveGB->setChecked(customizeLeave);
    loadLeaveActions();
}

PluginSettings& WingMenuConfiguration::settings() const
{
    return mSettings;
}

void WingMenuConfiguration::dialogButtonsAction(QAbstractButton* btn)
{
    QDialogButtonBox* box = qobject_cast<QDialogButtonBox*>(btn->parent());
    if (box && box->buttonRole(btn) == QDialogButtonBox::ResetRole) {
        settings().loadFromCache();
        loadSettings();
    } else {
        close();
    }
}
