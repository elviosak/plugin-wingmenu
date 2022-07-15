#pragma once

#include <QObject>

#ifndef QSL
#define QSL(text) QStringLiteral(text)
#endif

#ifndef DEFAULT_MENU_FILE
#define DEFAULT_MENU_FILE QStringLiteral("/etc/xdg/menus/lxqt-applications.menu")
#endif

namespace AppLayout {
    Q_NAMESPACE
        enum Layout {
        ListNameAndDescription = 0,
        ListNameOnly = 1,
        Icons = 2
    };
    Q_ENUM_NS(Layout)
}
