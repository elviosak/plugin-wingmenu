#pragma once

#include <QObject>

#ifndef QSL
#define QSL(text) QStringLiteral(text)
#endif

#ifndef DEFAULT_MENU_FILE
#define DEFAULT_MENU_FILE QSL("/etc/xdg/menus/lxqt-applications-compact.menu")
#endif

#ifndef DEFAULT_SHORTCUT
#define DEFAULT_SHORTCUT QSL("Alt+Shift+F1")
#endif

namespace AppLayout
{
Q_NAMESPACE
enum Layout {
    ListNameAndDescription = 0,
    ListNameOnly = 1,
    Icons = 2
};
Q_ENUM_NS(Layout)
};
