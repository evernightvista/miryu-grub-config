/*
    SPDX-FileCopyrightText: 2026 Evernight Vista Team <13278297951@sina.cn>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

// KDE System Settings module that auto-launches the external Miryu GRUB2
// Boot Config application. Registered under the "System Administration"
// section via X-KDE-System-Settings-Parent-Category in
// kcm_miryu_grubconfig.json. Clicking the entry in System Settings opens
// the standalone GUI, mirroring how openSUSE exposes YaST from System
// Settings. The KCM itself holds no configurable options; it is a launcher.

#include <KCModule>
#include <KPluginFactory>
#include <KPluginMetaData>
#include <KLocalizedString>

#include <QFont>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QProcess>
#include <QTimer>
#include <QVBoxLayout>

class MiryuGrubConfigKcm : public KCModule
{
    Q_OBJECT
public:
    MiryuGrubConfigKcm(QObject *parent, const KPluginMetaData &data)
        : KCModule(parent, data)
    {
        // 该 KCM 仅作为启动器，无可配置项，隐藏默认/重置/应用按钮
        setButtons(KCModule::NoAdditionalButton);

        auto *layout = new QVBoxLayout(widget());
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(12);

        auto *title = new QLabel(i18nd("miryu-grub-config", "Miryu GRUB2 Boot Config"), widget());
        QFont titleFont = title->font();
        titleFont.setPointSize(titleFont.pointSize() + 6);
        titleFont.setBold(true);
        title->setFont(titleFont);

        auto *description = new QLabel(
            i18nd("miryu-grub-config",
                  "Miryu GRUB2 Boot Config tunes the GRUB2 boot menu, kernel "
                  "parameters, and the saved default entry. Use the button "
                  "below to open the application."),
            widget());
        description->setWordWrap(true);

        m_status = new QLabel(widget());
        m_status->setWordWrap(true);
        m_status->setStyleSheet(QStringLiteral("color: palette(text)"));

        m_launchButton = new QPushButton(
            QIcon::fromTheme(QStringLiteral("system-boot-manager")),
            i18nd("miryu-grub-config", "Open Miryu GRUB2 Boot Config"),
            widget());
        m_launchButton->setObjectName(QStringLiteral("launchMiryuGrubConfig"));

        connect(m_launchButton, &QPushButton::clicked, this, &MiryuGrubConfigKcm::launchGrubConfig);

        layout->addWidget(title);
        layout->addWidget(description);
        layout->addStretch(1);
        layout->addWidget(m_launchButton, 0, Qt::AlignLeft);
        layout->addSpacing(8);
        layout->addWidget(m_status);
        layout->addStretch(10);

        // Auto-launch on entry: System Settings only instantiates a KCM when
        // the user navigates into it, so firing the launch here matches the
        // openSUSE YaST "click in System Settings -> opens the app" pattern.
        // Deferred to the next event-loop tick so the KCM widget paints first
        // and a transient failure can still be retried via the button.
        QTimer::singleShot(0, this, [this]() { launchGrubConfig(); });
    }

private Q_SLOTS:
    void launchGrubConfig()
    {
        const QString program = QStringLiteral("miryu-grub-config");
        qint64 pid = 0;
        if (QProcess::startDetached(program, {}, QString(), &pid)) {
            m_status->setText(i18nd("miryu-grub-config", "Miryu GRUB2 Boot Config has been launched."));
        } else {
            m_status->setText(
                i18nd("miryu-grub-config",
                      "Could not launch Miryu GRUB2 Boot Config. Make sure it is installed."));
        }
    }

private:
    QPushButton *m_launchButton = nullptr;
    QLabel *m_status = nullptr;
};

K_PLUGIN_CLASS_WITH_JSON(MiryuGrubConfigKcm, "kcm_miryu_grubconfig.json")

#include "kcm_miryu_grubconfig.moc"
