Name:           miryu-grub-config
Version:        45.0.0
Release:        1%{?dist}
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
BuildRequires:  desktop-file-utils
BuildRequires:  libappstream-glib

Requires:       polkit
Requires:       grub2-tools
Requires:       grubby
Obsoletes:      evernight-vista-grub-config
Provides:       evernight-vista-grub-config

%description
Miryu GRUB2 Boot Config is a KDE Plasma 6 friendly Qt/KF6 utility for
configuring GRUB2 boot menu behavior and custom kernel parameters.

The graphical interface follows the active KDE Plasma 6 application appearance
style. Saving GRUB2 settings is performed through a dedicated polkit-protected
helper, then the bootloader configuration is regenerated with grub2-mkconfig.


%prep
%autosetup -n %{name}-%{version}


%build
%cmake
%cmake_build


%install
%cmake_install
%find_lang %{name}


%check
desktop-file-validate %{buildroot}%{_datadir}/applications/org.miryu.grubconfig.desktop


%files -f %{name}.lang
%doc README.md
%{_bindir}/%{name}
%{_libexecdir}/%{name}-helper
%{_datadir}/applications/org.miryu.grubconfig.desktop
%{_datadir}/polkit-1/actions/org.miryu.grubconfig.policy
%{_qt6_plugindir}/plasma/kcms/systemsettings_qwidgets/kcm_miryu_grubconfig.so


%changelog
* Tue Aug 25 2026 Evernight Vista <13278297951@sina.cn> - 45.0.0-1
- Renamed from evernight-vista-grub-config to miryu-grub-config.
- Added a KDE System Settings (KCM) launcher module so clicking the
  "Miryu GRUB2 Boot Config" entry in System Settings opens the standalone
  application, mirroring the openSUSE YaST integration.

* Wed Aug 12 2026 Evernight Vista <13278297951@sina.cn> - 45.0.0-1
- Initial RPM package.
