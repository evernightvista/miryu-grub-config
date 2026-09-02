#include "mainwindow.h"
#include "grubconfigwidget.h"

#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QVBoxLayout>
#include <KLocalizedString>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_widget(new GrubConfigWidget(this))
{
    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("central"));

    auto *outer = new QVBoxLayout(central);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *title = new QLabel(i18n("Miryu GRUB2 Boot Config"), this);
    title->setObjectName(QStringLiteral("windowTitle"));
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setBold(true);
    title->setFont(titleFont);

    auto *header = new QVBoxLayout();
    header->setContentsMargins(26, 24, 26, 0);
    header->setSpacing(0);
    header->addWidget(title);
    outer->addLayout(header);

    // The shared panel owns the subtitle, the boot-menu/kernel-parameter cards
    // and the status line. Embed it directly so the standalone window shows the
    // exact same controls as the KCM.
    outer->addWidget(m_widget, 1);

    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(26, 0, 26, 20);
    bottomLayout->setSpacing(10);
    bottomLayout->addStretch();

    m_saveButton = new QPushButton(i18n("Save GRUB2 Settings"), this);
    m_saveButton->setObjectName(QStringLiteral("primaryButton"));
    m_saveButton->setIcon(QIcon::fromTheme(QStringLiteral("dialog-password")));
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::onSaveClicked);

    m_rebootButton = new QPushButton(i18n("Reboot"), this);
    m_rebootButton->setIcon(QIcon::fromTheme(QStringLiteral("system-reboot")));
    connect(m_rebootButton, &QPushButton::clicked, this, &MainWindow::onRebootClicked);

    bottomLayout->addWidget(m_saveButton);
    bottomLayout->addWidget(m_rebootButton);
    outer->addLayout(bottomLayout);

    setCentralWidget(central);
    setWindowTitle(i18n("Miryu GRUB2 Boot Config"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("system-boot-manager")));
    resize(840, 680);

    // Keep the Save button in sync with the panel's dirty state.
    m_saveButton->setEnabled(m_widget->isDirty());
    connect(m_widget, &GrubConfigWidget::changed, this, [this](bool dirty) {
        m_saveButton->setEnabled(dirty);
    });

    // The save flow is asynchronous (KAuth). Show the result in a modal
    // dialog when the helper finishes.
    connect(m_widget, &GrubConfigWidget::saveSucceeded, this, [this]() {
        QMessageBox::information(this, i18n("Saved"),
            i18n("GRUB2 settings were saved and grub2-mkconfig completed successfully."));
    });
    connect(m_widget, &GrubConfigWidget::saveFailed, this, [this](const QString &error) {
        QMessageBox::critical(this, i18n("Save Failed"),
            i18n("Could not save and regenerate GRUB2 configuration.\n\n%1", error));
    });
}

MainWindow::~MainWindow() = default;

void MainWindow::onSaveClicked()
{
    m_widget->applySettingsAsync();
}

void MainWindow::onRebootClicked()
{
    const auto answer = QMessageBox::question(
        this,
        i18n("Confirm Reboot"),
        i18n("Reboot now?\n\nUnsaved changes will not be applied."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (answer == QMessageBox::Yes) {
        QProcess::startDetached(QStringLiteral("systemctl"), QStringList() << QStringLiteral("reboot"));
    }
}
