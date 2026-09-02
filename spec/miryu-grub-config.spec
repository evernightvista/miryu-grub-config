Name:           miryu-grub-config
Version:        45.0.0
Release:        3%{?dist}
Summary:        KDE Plasma 6 GRUB2 configuration tool for Miryu

License:        GPL-3.0-or-later
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  extra-cmake-modules
BuildRequires:  gcc-c++
BuildRequires:  gettext
BuildRequires:  qt6-qtbase-devel
BuildRequires:  kf6-ki18n-devel
BuildRequires:  kf6-kcoreaddons-devel
BuildRequires:  kf6-kcmutils-devel
BuildRequires:  kf6-kwidgetsaddons-devel
BuildRequires:  kf6-kauth-devel
BuildRequires:  desktop-file-utils
BuildRequires:  libappstream-glib

Requires:       polkit
Requires:       kf6-kauth
Requires:       grub2-tools
Requires:       grubby
Obsoletes:      evernight-vista-grub-config
Provides:       evernight-vista-grub-config

%description
Miryu GRUB2 Boot Config is a KDE Plasma 6 friendly Qt/KF6 utility for
configuring GRUB2 boot menu behavior and custom kernel parameters.

The graphical interface follows the active KDE Plasma 6 application appearance
style. Saving GRUB2 settings is performed through a KAuth helper that runs
asynchronously, so the UI never freezes while waiting for administrator
authentication or grub2-mkconfig. After saving, the bootloader configuration
is regenerated with grub2-mkconfig.

The same GUI is embedded directly into KDE System Settings as a KCM, so users
configure GRUB2 in place with the standard "Reset" (left) and "Apply" (right)
buttons instead of launching a separate window.


%prep
%autosetup -n %{name}-%{version}


%build
%cmake
%cmake_build


%install
%cmake_install
%find_lang %{name}


%check
desktop-file-validate %{buildroot}%{_datadir}/applications/org.miryugaming.grubconfig.desktop


%files -f %{name}.lang
%doc README.md
%{_bindir}/%{name}
%{_libexecdir}/kauth/%{name}-kauth-helper
%{_datadir}/applications/org.miryugaming.grubconfig.desktop
%{_datadir}/dbus-1/system-services/org.miryugaming.grubconfig.service
%{_datadir}/dbus-1/system.d/org.miryugaming.grubconfig.conf
%{_datadir}/polkit-1/actions/org.miryugaming.grubconfig.policy
%{_qt6_plugindir}/plasma/kcms/systemsettings_qwidgets/kcm_miryu_grubconfig.so


%changelog
* Tue Sep 01 2026 KairikiFedora <13278297951@sina.cn> - 45.0.0-3
- Renamed the polkit action ID, D-Bus service name, and KAuth helper ID
  prefix from org.miryu.grubconfig to org.miryugaming.grubconfig.
- Renamed all affected files (.actions, .conf, .service.in, .policy.in,
  .desktop) and updated all CMake, source, and spec references accordingly.
- Fixed the KCM Apply button icon override so the dialog-password (key)
  icon is reliably shown in both the dirty and disabled (greyed-out) states.
- The icon search now covers QWindow instances (QQuickWindow / QQuickView)
  in addition to QWidget top-level windows, fixing discovery on KDE Plasma 6
  where System Settings may use a QQuickWindow that does not appear in
  topLevelWidgets().
- Added QDialogButtonBox::button(Apply) as a locale-independent discovery path
  for QWidget-based button bars.
- Added multi-language text matching (en, de, fr, ja, ko, zh_CN, zh_TW) for
  QML-based button bars.
- Fixed the timer to use a two-phase approach: fast polling (300 ms) for
  initial discovery, then slow maintenance polling (2 s) that re-applies the
  icon when the framework resets it on enabled/disabled state changes.
- The icon is also re-applied on changed(), load(), and save() callbacks.
- Replaced the synchronous pkexec save flow with an asynchronous KAuth
  helper so the UI no longer freezes during authentication or grub2-mkconfig.
- The KAuth helper receives configuration content via QVariantMap arguments
  instead of through a temporary file.
- Removed the old polkit .policy.in and pkexec helper; the polkit policy is
  now auto-generated from the KAuth .actions file.

* Tue Sep 01 2026 Evernight Vista <13278297951@sina.cn> - 45.0.0-2
- Integrated the full GRUB2 configuration GUI directly into the KDE System
  Settings KCM instead of launching the standalone application.
- The KCM now exposes only the standard "Reset" (left) and "Apply" (right)
  buttons; Apply runs the polkit-protected save flow, Reset reloads the
  current configuration.
- Extracted the shared settings panel into GrubConfigWidget so the KCM and
  the standalone miryu-grub-config window stay in sync.
- Updated the KCM metadata description to reflect direct configuration.

* Tue Aug 25 2026 Evernight Vista <13278297951@sina.cn> - 45.0.0-1
- Renamed from evernight-vista-grub-config to miryu-grub-config.
- Added a KDE System Settings (KCM) launcher module so clicking the
  "Miryu GRUB2 Boot Config" entry in System Settings opens the standalone
  application, mirroring the openSUSE YaST integration.

* Wed Aug 12 2026 Evernight Vista <13278297951@sina.cn> - 45.0.0-1
- Initial RPM package.
