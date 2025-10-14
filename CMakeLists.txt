cmake_minimum_required(VERSION 3.18.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.

# Set CMAKE_INSTALL_PREFIX to /usr by default
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX /usr CACHE PATH "PREFIX=/usr" FORCE)
endif()

project(lxqt-panel)

set(PLUGIN "wingmenu")

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" ON)

# additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


include(GNUInstallDirs)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(REQUIRED_QT_VERSION "6.6.0")
set(KF6_MINIMUM_VERSION "6.0.0")
set(LXQT_GLOBALKEYS_MINIMUM_VERSION "2.0.0")
set(LXQT_MINIMUM_VERSION "2.0.0")

find_package(Qt6Widgets ${REQUIRED_QT_VERSION} REQUIRED)
find_package(Qt6Xml ${REQUIRED_QT_VERSION} REQUIRED)
find_package(Qt6DBus ${REQUIRED_QT_VERSION} REQUIRED)
find_package(lxqt ${LXQT_MINIMUM_VERSION} REQUIRED)
find_package(lxqt-globalkeys-ui ${LXQT_GLOBALKEYS_MINIMUM_VERSION} REQUIRED)
find_package(Qt6Xdg REQUIRED)
# find_package(lxqt-panel ${LXQT_MINIMUM_VERSION} REQUIRED)

if (Qt6Gui_VERSION VERSION_GREATER_EQUAL "6.10.0")
    find_package(Qt6GuiPrivate REQUIRED)
endif()
# Patch Version
# set(LXQT_PANEL_PATCH_VERSION 0)
# set(LXQT_PANEL_VERSION ${LXQT_MAJOR_VERSION}.${LXQT_MINOR_VERSION}.${LXQT_PANEL_PATCH_VERSION})
# add_definitions("-DLXQT_PANEL_VERSION=\"${LXQT_PANEL_VERSION}\"")

include(LXQtPreventInSourceBuilds)
include(LXQtTranslate)

# All LXQtCompilerSettings except CMAKE_MODULE_LINKER_FLAGS work just fine
# So we reset only these Flags after loading LXQtCompilerSettings
# lxqt-build-tools:
#  set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--no-undefined ${SYMBOLIC_FLAGS} ${CMAKE_MODULE_LINKER_FLAGS}")
message(STATUS "==OLD== CMAKE_MODULE_LINKER_FLAGS: ${CMAKE_MODULE_LINKER_FLAGS}")
set( OLD_CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS}")
include(LXQtCompilerSettings NO_POLICY_SCOPE)
set(CMAKE_MODULE_LINKER_FLAGS "${OLD_CMAKE_MODULE_LINKER_FLAGS} ${SYMBOLIC_FLAGS}")

# Warning: This must be before add_subdirectory(panel). Move with caution.
set(PLUGIN_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/lxqt-panel")
add_definitions(
    -DPLUGIN_DIR=\"${PLUGIN_DIR}\"
)

message(STATUS "CMAKE Module linker flags: ${CMAKE_MODULE_LINKER_FLAGS}")
message(STATUS "Panel plugins location: ${PLUGIN_DIR}")

#########################################################################

include_directories("${CMAKE_INSTALL_PREFIX}/include/lxqt/")
set(HEADERS
  "${PLUGIN}plugin.h"
  "${PLUGIN}configuration.h"
  "${PLUGIN}widget.h"
  itemdescriptiondelegate.h
  applicationsview.h
  common.h
)

set(SOURCES
  "${PLUGIN}plugin.cpp"
  "${PLUGIN}configuration.cpp"
  "${PLUGIN}widget.cpp"
  itemdescriptiondelegate.cpp
  applicationsview.cpp
)

set(UIS
  "${PLUGIN}configuration.ui"
)

set(LIBRARIES
    ${LIBRARIES}
    lxqt-globalkeys-ui
    Qt6Xdg
    Qt6Xml
    Qt6DBus
)
set(ICON_SHARE_DIR ${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/scalable/apps)
set(ICON_FILE "resources/wing-lxqt.svg")
install(FILES "${ICON_FILE}" DESTINATION "${ICON_SHARE_DIR}")
include ("cmake/BuildPlugin.cmake")
BUILD_LXQT_PLUGIN(${PLUGIN})
