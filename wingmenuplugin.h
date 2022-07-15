#pragma once

#include "common.h"
#include <ilxqtpanelplugin.h>
#include <pluginsettings.h>

#include <QAction>
#include <QDialog>
#include <QMenu>
#include <QObject>
#include <QString>
#include <QToolButton>
#include <QWidget>
#include <QWidgetAction>
#include <QKeyEvent>
#include <XdgMenu>

namespace GlobalKeyShortcut
{
    class Action;
}
class WingMenuWidget;

class WingMenuPlugin : public QObject, public ILXQtPanelPlugin
{
    Q_OBJECT
public:
    WingMenuPlugin(const ILXQtPanelPluginStartupInfo& startupInfo);
    ~WingMenuPlugin();

    virtual QWidget* widget() override { return mWidget; }
    virtual QString themeId() const override
    {
        return QStringLiteral("WingMenuPlugin");
    }

    virtual Flags flags() const override { return HaveConfigDialog; }
    QDialog* configureDialog() override;

    bool isSeparate() const override { return true; }
    bool isExpandable() const override { return false; }

    void realign() override;
    void buildMenu();
    void showMenu();
    void hideMenu();
    const GlobalKeyShortcut::Action* shortcut() { return mShortcut; };

private:
    QToolButton* mWidget;
    GlobalKeyShortcut::Action* mShortcut;
    QMenu* mMenu;
    WingMenuWidget* mMenuWidget;
    QWidgetAction* mMenuAction;
    AppLayout::Layout mAppLayout;
    QString mMenuFile;
    XdgMenu* mXdgMenu;

    bool mShowIcon;
    QString mIcon;
    bool mShowText;
    QString mText;

    void settingsChanged() override;
    void setupShortcut();

    void showHideMenu();
    void showHideMenuDelayed();
};

// ***************************************************

class WingMenuLibrary : public QObject, public ILXQtPanelPluginLibrary
{
    Q_OBJECT
        Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
        Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin* instance(const ILXQtPanelPluginStartupInfo& startupInfo) const
    {
        return new WingMenuPlugin(startupInfo);
    }
};
