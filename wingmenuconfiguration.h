#pragma once

#include <QAbstractButton>
#include <QDialog>
#include <QStandardItemModel>

#include <lxqtpanelglobals.h>
#include <pluginsettings.h>

class XdgDesktopFile;
class XdgMenu;
// class QComboBox;
namespace Ui
{
class WingMenuConfiguration;
};

namespace GlobalKeyShortcut
{
class Action;
}

class WingMenuConfiguration : public QDialog
{
    Q_OBJECT
public:
    explicit WingMenuConfiguration(
        PluginSettings& settings,
        GlobalKeyShortcut::Action* shortcut,
        XdgMenu* xdgMenu,
        QString dBusMessage,
        QWidget* parent = nullptr);

    explicit WingMenuConfiguration(
        PluginSettings* settings,
        GlobalKeyShortcut::Action* shortcut,
        XdgMenu* xdgMenu,
        QString dBusMessage,
        QWidget* parent = nullptr)
        : WingMenuConfiguration(
              *settings,
              shortcut,
              xdgMenu,
              dBusMessage,
              parent) {};

    ~WingMenuConfiguration() = default;

    PluginSettings& settings() const;

protected slots:
    void loadSettings() const;
    void dialogButtonsAction(QAbstractButton* btn);

private:
    Ui::WingMenuConfiguration* ui;
    PluginSettings& mSettings;
    XdgMenu* mXdgMenu;
    GlobalKeyShortcut::Action* mShortcut;
    QStandardItemModel* mLeaveActionsModel;
    QString mDesktopFilesDir;

    void globalShortcutChanged(const QString& oldShortcut, const QString& newShortcut);
    void shortcutChanged(const QString& value);
    void shortcutReset();
    void chooseMenuFile();

    void actionActivated(const QModelIndex& index);
    QStandardItem* createItem(const QString& fileName) const;
    void copyDesktopFile(const QString& fileName);
    void loadLeaveActions() const;
    void saveLeaveActions();
    void customizeLeave(bool customize);
    void loadFromMenu();
    void addDesktopFile();
    void newAction();
    void editAction();
    void upAction();
    void downAction();
    void removeAction();
    void openEditDialog(const QString& fileName = QString());
    void saveDesktopFile(const QString& name, const QString& icon, const QString& exec, const QString& fileName);
    QString newFileName();
};
