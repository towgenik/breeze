# Accent Outline

A separate Plasma 6 KDecoration3 plugin. It provides a titlebar-free
decoration with no window buttons and no plugin-provided shadow. KWin draws a
configurable outline using the accent color resolved by Plasma.

## Defaults

- Outline width: **8 logical pixels**
- Rounded corners: enabled
- Corner radius: **8 logical pixels**
- Custom accent override: disabled
- Outline visibility: focused window only
- Titlebar: disabled
- Window buttons: none
- Shadow: none
- Accent: Plasma's wallpaper-derived accent, with palette/config fallbacks

The outline is an external KWin `BorderOutline`; it does not consume client
area. It is drawn only on the focused window. Resize-only margins are kept
around the client so edge resizing remains available. Maximized windows have no
outline.

Rounded corners are a separate mechanism and apply to every window, focused or
not. KWin reads the decoration's `borderRadius()` in
`Window::updateDecorationBorderRadius()`, forwards it to the window item, and the
scene renderer turns it into a corner mask
(`ShaderTrait::RoundedCorners`) on the window contents. The outline is a
different item (`OutlinedBorderItem`) with its own radius, so clearing the
outline does not clear the corners. Breeze separates the two the same way.
Maximized windows are never rounded.

## Minimum KDecoration3 requirements

A native KDecoration3 plugin needs all of the following:

1. A shared module installed under a Qt plugin path in the
   `org.kde.kdecoration3` namespace.
2. Embedded metadata with `ServiceTypes: ["org.kde.kdecoration3"]` and a
   `K_PLUGIN_FACTORY_WITH_JSON` factory. For C++ plugins, the output filename
   is the plugin ID.
3. A class derived from `KDecoration3::Decoration` that forwards the
   constructor arguments to the base class and implements both `init()` and
   `paint()`.
4. Geometry published through `setBorders()`, `setResizeOnlyBorders()`, and
   `setTitleBar()`. An outline-only decoration may use an empty titlebar and
   resize-only margins.
5. ABI-compatible Qt, KDE Frameworks, and KDecoration3 versions.
6. A user plugin directory that is present in `QT_PLUGIN_PATH` **before KWin
   starts**, or a system-wide installation. Changing the environment after
   KWin starts cannot make the running compositor discover a new plugin.
7. A matching `kwinrc` selection, for example
   `[org.kde.kdecoration2] library=io.github.towgenik.accentoutline`.

For this specific plugin, the visual work is done by
`KDecoration3::BorderOutline`; `paint()` is intentionally empty. A KCM is
optional for loading the decoration, but required for changing its settings.
Applications that negotiate client-side decorations may bypass server-side
decorations entirely.

## Configure

Install the plugin and its KCM, then select **Accent Outline** in **System
Settings → Window Management → Window Decoration**. Its configuration module
exposes:

- outline width: 0–64 px
- rounded-corner toggle
- corner radius: 0–32 px
- custom accent-color override

The plugin asks Plasma for the current wallpaper accent. If Plasma cannot
provide one, it falls back to the current palette accent. Enable the custom
color option to override that choice.

### Opening the configuration module

The decoration tile in **Window Decoration** carries a pencil action that opens
the KCM. Per `kcmutils`' `GridDelegate.qml`, that action is only visible while
the tile is hovered or is the current item, so it is easy to miss.

The KCM can also be opened directly. `kcmshell6` only searches the
`plasma/kcms*` namespaces, so the module has to be addressed by its
namespace-relative plugin id, exactly like `PreviewBridge::configure()` in KWin
does:

```sh
kcmshell6 org.kde.kdecoration3.kcm/kcm_accentoutline
```

The bare id (`kcmshell6 kcm_accentoutline`) does **not** resolve and shows a
"Could not find plugin" error. This is not specific to this KCM; stock Breeze
behaves the same way. The installed desktop entry
(`share/applications/kcm_accentoutline.desktop`) already uses the working form.

## Debug

The read-only diagnostic script reports the installed files, KWin selection,
Qt plugin path, output state, wallpaper accent, and recent decoration errors:

```sh
kdecoration/accentoutline/tools/accent-outline-debug
```

For verbose plugin logging, set these variables before starting the Plasma
session (they cannot be added to an already-running KWin process):

```sh
ACCENT_OUTLINE_DEBUG=1
QT_LOGGING_RULES='org.towgenik.accentoutline*=true'
```

Then inspect KWin's journal:

```sh
journalctl --user -u plasma-kwin_wayland.service -f
```

## Build

From the repository root, a Qt 6-only build can be configured with:

```sh
cmake -S . -B build \
  -DBUILD_QT5=OFF \
  -DBUILD_QT6=ON \
  -DBUILD_WITH_QTQUICK=OFF \
  -DBUILD_CURSOR=OFF \
  -DWITH_WALLPAPERS=OFF \
  -DWITH_DECORATIONS=ON \
  -DKDE_INSTALL_USE_QT_SYS_PATHS=ON \
  -DKDE_INSTALL_PLUGINDIR=lib/qt6/plugins
cmake --build build --target accentoutlinedecoration kcm_accentoutline
```

Install into a user prefix with:

```sh
cmake --install build --prefix "$HOME/.local"
```

The plugin is installed under `~/.local/lib/qt6/plugins`. Ensure that
`QT_PLUGIN_PATH` contains `~/.local/lib/qt6/plugins` before the next Plasma
session starts; do not replace the system Breeze plugin.

`QT_PLUGIN_PATH` must be an absolute path. Qt does not expand `%h` in that
variable, so `QT_PLUGIN_PATH=%h/.local/lib/qt6/plugins` silently disables user
plugin discovery for KWin, plasmashell and anything they launch (including
System Settings, which then cannot find either the decoration or its KCM).
Persist the value with a systemd user environment file:

```ini
# ~/.config/environment.d/90-kde-user-qt-plugins.conf
[Environment]
QT_PLUGIN_PATH=/home/user/.local/lib/qt6/plugins
```

For a running session, apply it without logging out:

```sh
systemctl --user set-environment QT_PLUGIN_PATH="$HOME/.local/lib/qt6/plugins"
dbus-update-activation-environment --systemd QT_PLUGIN_PATH="$HOME/.local/lib/qt6/plugins"
systemctl --user restart plasma-plasmashell.service
```

`systemctl --user show-environment` should report the expanded absolute path.
