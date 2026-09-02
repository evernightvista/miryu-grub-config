# Miryu GRUB2 Boot Config

`miryu-grub-config` is a Qt-based GRUB2 settings tool rewritten from the uploaded Fedora GRUB boot configuration prototype.

## What changed

- Writes the drop-in file to `/etc/default/grub.d/99_miryu_boot.cfg`.
- Saves GRUB2 settings asynchronously through KAuth, so the UI never freezes while waiting for administrator authentication or `grub2-mkconfig`.
- The KAuth helper receives configuration content via `QVariantMap` arguments instead of through a temporary file.
- Removes shell command concatenation from the GUI save path.
- Regenerates GRUB2 configuration after saving with `grub2-mkconfig -o /boot/grub2/grub.cfg`.
- Uses the active KDE Plasma 6 application style instead of overriding controls with custom QSS.
- Adds KF6 I18n support and `po` translation files.
- Adds a KDE System Settings (KCM) module that embeds the full GRUB2 configuration GUI directly, so users edit boot-menu and kernel parameters in place with the standard "Reset" (left) and "Apply" (right) buttons instead of opening a separate window.
- Adds translated polkit policy text (auto-generated from the KAuth `.actions` file) for Simplified Chinese, Traditional Chinese, Japanese, German, Korean, and French.

## Dependencies

On Fedora KDE / Plasma 6, install the Qt 6, KF6 (I18n, CoreAddons, KCMUtils, WidgetsAddons, Auth), and ECM development packages before building. Package names may vary by distribution, but Fedora systems usually need:

```bash
sudo dnf install cmake extra-cmake-modules qt6-qtbase-devel kf6-ki18n-devel kf6-kcoreaddons-devel kf6-kcmutils-devel kf6-kwidgetsaddons-devel kf6-kauth-devel
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Install

Install with administrator privileges so the KAuth helper and polkit action are placed in system locations:

```bash
sudo cmake --install build
```

This installs:

- `miryu-grub-config` to the system binary directory.
- `miryu-grub-config-kauth-helper` to the KAuth helper directory (`libexec/kauth/`).
- `org.miryugaming.grubconfig.policy` (auto-generated from the `.actions` file) to the polkit actions directory.
- `org.miryugaming.grubconfig.desktop` to the applications directory.
- `kcm_miryu_grubconfig.so` to the KDE System Settings plugin directory (`plasma/kcms/systemsettings_qwidgets`).
- Compiled translations generated from `po/*/miryu-grub-config.po`.

## KDE System Settings integration

The KCM module registers under the "System Administration" section of KDE Plasma 6 System Settings. Instead of launching a separate window, it embeds the full Miryu GRUB2 Boot Config GUI (the boot-menu card and the kernel-parameter card) directly in the page.

Only the standard **Reset** (bottom-left) and **Apply** (bottom-right) buttons are shown:

- **Apply** starts the asynchronous KAuth save flow (writes `/etc/default/grub`, updates BLS entries via `grubby`, and regenerates `grub.cfg`), then reloads the page so it reflects the new on-disk state. The UI stays responsive throughout.
- **Reset** reverts any unsaved edits by re-reading the current configuration.

The editable surface and the save logic are shared with the standalone `miryu-grub-config` window through `GrubConfigWidget`, so both stay identical.

## Save flow

1. The GUI generates the GRUB2 configuration content as a string.
2. The GUI creates a `KAuth::Action` with the content passed as a `QVariantMap` argument.
3. KAuth asks for administrator authentication (polkit under the hood).
4. The helper verifies that the input is a safe Miryu GRUB2 configuration.
5. The helper merges the settings into `/etc/default/grub` in place.
6. The helper removes the old drop-in file if present.
7. The helper updates BLS entries via `grubby` (non-fatal on failure).
8. The helper runs `grub2-mkconfig -o /boot/grub2/grub.cfg`.
9. The helper returns an `ActionReply`; the GUI reloads on success or shows an error on failure.

The entire flow is asynchronous: `KAuth::ExecuteJob` runs in the background and emits `KJob::result` when done, so the interface never blocks.

## Notes

- The KAuth helper must be installed before the Save button can apply changes.
- Kernel parameters are intentionally restricted to one safe token at a time.
- Reboot is still a separate explicit action.
- The interface intentionally does not force Breeze or any custom stylesheet; KDE Plasma 6 controls, colors, fonts, spacing, and icons come from the current system appearance settings.
