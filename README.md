# Wing Menu is an alternative menu plugin for [lxqt-panel](https://github.com/lxqt/lxqt-panel)

It has two columns instead of the "classic menu" and was inspired by Xfce's [Whisker Menu](https://docs.xfce.org/panel-plugins/xfce4-whiskermenu-plugin/start)

## Screenshots

### Settings:

![Settings](screenshots/settings.png "Settings")

### Name and Description View:

![Name and Description](screenshots/name-and-description.png "Name and Description")

### Name Only View:

![Name Only](screenshots/name-only.png "Name Only")

### Icons View:

![Icons](screenshots/icons.png "Icons")

## Install:

```bash
# clone repo
git clone https://github.com/slidinghotdog/plugin-wingmenu.git

# cd into folder
cd plugin-wingmenu

# run install script
bash install.sh

# run lxqt-config session to stop/start panel
# to reload the list of plugins.
lxqt-config-session

```

#### Note: it's not well integrated with LXQt Themes, use it with "System" theme.

If everything was successful you should have a new plugin ready to be added to your panel, named `Wing Menu (wingmenu)`.
