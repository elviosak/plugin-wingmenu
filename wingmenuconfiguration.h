#pragma once

#include <QAbstractButton>
#include <QDialog>

#include <lxqtpanelglobals.h>
#include <pluginsettings.h>

class QComboBox;
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
        const QString& defaultShortcut,
        QWidget* parent = nullptr);

    explicit WingMenuConfiguration(
        PluginSettings* settings,
        GlobalKeyShortcut::Action* shortcut,
        const QString& defaultShortcut,
        QWidget* parent = nullptr) : WingMenuConfiguration(*settings,
            shortcut,
            defaultShortcut,
            parent) {};

    ~WingMenuConfiguration() = default;

    PluginSettings& settings() const;

protected slots:
    void loadSettings() const;
    void dialogButtonsAction(QAbstractButton* btn);

private:
    Ui::WingMenuConfiguration* ui;
    PluginSettings& mSettings;
    QString mDefaultShortcut;
    GlobalKeyShortcut::Action* mShortcut;

    void globalShortcutChanged(const QString& oldShortcut, const QString& newShortcut);
    void shortcutChanged(const QString& value);
    void shortcutReset();
    void chooseMenuFile();
};
