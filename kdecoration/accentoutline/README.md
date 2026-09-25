# Accent Outline

A separate Plasma 6 KDecoration3 plugin. It provides a titlebar-free
decoration with no window buttons and no plugin-provided shadow. KWin draws a
configurable outline using the accent color resolved by Plasma.

## Defaults

- Outline width: **8 logical pixels**
- Titlebar: disabled
- Window buttons: none
- Shadow: none
- Accent: Plasma's wallpaper-derived accent, with palette/config fallbacks

The outline is an external KWin `BorderOutline`; it does not consume client
area. Resize-only margins are kept around the client so edge resizing remains
available. Maximized windows have no outline.

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
exposes an outline-width control from 0–64 px.

The color is shared with Plasma's Colors settings. To use the wallpaper accent,
select **Accent color from wallpaper** there.

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
