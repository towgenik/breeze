# Accent Outline

A separate Plasma 6 KDecoration3 plugin based on the Breeze window-decoration
framework. It provides a titlebar-free decoration with no window buttons and no
plugin-provided shadow. KWin draws a configurable outline using the accent
color resolved by Plasma.

## Defaults

- Outline width: **8 logical pixels**
- Titlebar: disabled
- Window buttons: none
- Shadow: none
- Accent: Plasma's wallpaper-derived accent when wallpaper accent mode is
  enabled, otherwise the current palette highlight color

The outline is an external KWin `BorderOutline`; it does not consume client
area. Resize-only margins are kept around the client so edge resizing remains
available. Maximized windows have no outline, matching normal Plasma behavior.

## Configure

Install the plugin and its KCM, restart KWin if necessary, then select
**Accent Outline** in **System Settings → Window Management → Window
Decoration**. Its configuration module exposes an outline-width control from
0–64 px.

The color is shared with Plasma's Colors settings. To use the wallpaper accent,
select **Accent color from wallpaper** there.

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

The plugin is installed under the KDecoration3 Qt plugin directory. Restart
KWin or the Plasma session after installing a new plugin.
