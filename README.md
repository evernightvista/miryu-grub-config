# Miryu GRUB2 Boot Config

`miryu-grub-config` is a Qt-based GRUB2 settings tool rewritten from the uploaded Fedora GRUB boot configuration prototype.

## What changed

- Writes the drop-in file to `/etc/default/grub.d/99_miryu_boot.cfg`.
- Saves GRUB2 settings through `pkexec` and a dedicated polkit-protected helper.
- Removes shell command concatenation from the GUI save path.
- Regenerates GRUB2 configuration after saving with `grub2-mkconfig -o /boot/grub2/grub.cfg`.
- Uses the active KDE Plasma 6 application style instead of overriding controls with custom QSS.
- Adds KF6 I18n support and `po` translation files.
- Adds a KDE System Settings (KCM) launcher module so clicking "Miryu GRUB2 Boot Config" in System Settings opens the standalone application, mirroring the openSUSE YaST integration.
- Adds translated polkit policy text for Simplified Chinese, Traditional Chinese, Japanese, German, Korean, and French.

## Dependencies

On Fedora KDE / Plasma 6, install the Qt 6, KF6 (I18n, CoreAddons, KCMUtils, WidgetsAddons), and ECM development packages before building. Package names may vary by distribution, but Fedora systems usually need:

```bash
sudo dnf install cmake extra-cmake-modules qt6-qtbase-devel kf6-ki18n-devel kf6-kcoreaddons-devel kf6-kcmutils-devel kf6-kwidgetsaddons-devel
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Install

Install with administrator privileges so the helper and polkit action are placed in system locations:

```bash
sudo cmake --install build
```

This installs:

- `miryu-grub-config` to the system binary directory.
- `miryu-grub-config-helper` to the system libexec directory.
- `org.miryugaming.grubconfig.policy` to the polkit actions directory.
- `org.miryugaming.grubconfig.desktop` to the applications directory.
- `kcm_miryu_grubconfig.so` to the KDE System Settings plugin directory (`plasma/kcms/systemsettings_qwidgets`).
- Compiled translations generated from `po/*/miryu-grub-config.po`.

## KDE System Settings integration

The KCM module registers under the "System Administration" section of KDE Plasma 6 System Settings. Clicking the "Miryu GRUB2 Boot Config" entry opens the standalone application automatically, the same way openSUSE exposes YaST from System Settings.

## Save flow

1. The GUI generates a temporary GRUB drop-in configuration.
2. The GUI launches `pkexec miryu-grub-config-helper --apply <temporary-file>`.
3. polkit asks for administrator authentication.
4. The helper verifies that the input is a safe Miryu GRUB2 configuration.
5. The helper writes `/etc/default/grub.d/99_miryu_boot.cfg`.
6. The helper runs `grub2-mkconfig -o /boot/grub2/grub.cfg`.

## Notes

- The helper must be installed before the Save button can apply changes.
- Kernel parameters are intentionally restricted to one safe token at a time.
- Reboot is still a separate explicit action.
- The interface intentionally does not force Breeze or any custom stylesheet; KDE Plasma 6 controls, colors, fonts, spacing, and icons come from the current system appearance settings.
