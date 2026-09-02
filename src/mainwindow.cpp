#include "mainwindow.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTemporaryFile>
#include <QTextEdit>
#include <QVBoxLayout>
#include <KLocalizedString>

#ifndef HELPER_PATH
#define HELPER_PATH "/usr/libexec/miryu-grub-config-helper"
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_config(new GrubConfig(this))
{
    setupUi();
    loadConfig();

    setWindowTitle(i18n("Miryu GRUB2 Boot Config"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("system-boot-manager")));
    resize(840, 680);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("central"));

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(26, 24, 26, 20);
    mainLayout->setSpacing(16);

    auto *title = new QLabel(i18n("Miryu GRUB2 Boot Config"), this);
    title->setObjectName(QStringLiteral("windowTitle"));
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setBold(true);
    title->setFont(titleFont);

    auto *subtitle = new QLabel(i18n("Tune the GRUB2 boot menu, kernel parameters, and saved default entry."), this);
    subtitle->setObjectName(QStringLiteral("windowSubtitle"));
    subtitle->setWordWrap(true);

    mainLayout->addWidget(title);
    mainLayout->addWidget(subtitle);

    m_bootCard = new QFrame(this);
    m_bootCard->setObjectName(QStringLiteral("card"));
    m_bootCard->setFrameShape(QFrame::StyledPanel);
    m_bootCard->setFrameShadow(QFrame::Plain);
    auto *bootLayout = new QVBoxLayout(m_bootCard);
    bootLayout->setContentsMargins(18, 16, 18, 16);
    bootLayout->setSpacing(10);

    auto *bootTitle = new QLabel(i18n("Boot Menu"), this);
    bootTitle->setObjectName(QStringLiteral("cardTitle"));
    QFont cardTitleFont = bootTitle->font();
    cardTitleFont.setPointSize(cardTitleFont.pointSize() + 2);
    cardTitleFont.setBold(true);
    bootTitle->setFont(cardTitleFont);
    bootLayout->addWidget(bootTitle);

    m_showMenuCheck = new QCheckBox(i18n("Show boot menu"), this);
    m_showMenuCheck->setToolTip(i18n("Show the GRUB menu before starting the default system."));
    bootLayout->addWidget(m_showMenuCheck);

    auto *delayLayout = new QHBoxLayout();
    delayLayout->setSpacing(10);
    auto *delayLabel = new QLabel(i18n("Delay before booting"), this);
    m_delaySpin = new QSpinBox(this);
    m_delaySpin->setRange(-1, 3600);
    m_delaySpin->setSuffix(i18n(" seconds"));
    m_delaySpin->setSpecialValueText(i18n("wait indefinitely"));
    m_delaySpin->setToolTip(i18n("Use -1 to wait until a menu entry is selected."));
    delayLayout->addWidget(delayLabel);
    delayLayout->addWidget(m_delaySpin);
    delayLayout->addStretch();
    bootLayout->addLayout(delayLayout);

    m_rememberCheck = new QCheckBox(i18n("Remember last selected entry as the default"), this);
    bootLayout->addWidget(m_rememberCheck);

    mainLayout->addWidget(m_bootCard);

    m_paramsCard = new QFrame(this);
    m_paramsCard->setObjectName(QStringLiteral("card"));
    m_paramsCard->setFrameShape(QFrame::StyledPanel);
    m_paramsCard->setFrameShadow(QFrame::Plain);
    auto *paramsLayout = new QVBoxLayout(m_paramsCard);
    paramsLayout->setContentsMargins(18, 16, 18, 16);
    paramsLayout->setSpacing(10);

    auto *paramsTitle = new QLabel(i18n("Kernel Parameters"), this);
    paramsTitle->setObjectName(QStringLiteral("cardTitle"));
    paramsTitle->setFont(cardTitleFont);
    paramsLayout->addWidget(paramsTitle);

    auto *currentLabel = new QLabel(i18n("Current kernel command line"), this);
    currentLabel->setObjectName(QStringLiteral("sectionLabel"));
    paramsLayout->addWidget(currentLabel);

    m_currentParamsDisplay = new QTextEdit(this);
    m_currentParamsDisplay->setReadOnly(true);
    m_currentParamsDisplay->setMinimumHeight(76);
    m_currentParamsDisplay->setMaximumHeight(92);
    paramsLayout->addWidget(m_currentParamsDisplay);

    auto *customLabel = new QLabel(i18n("Custom parameters written by this tool"), this);
    customLabel->setObjectName(QStringLiteral("sectionLabel"));
    paramsLayout->addWidget(customLabel);

    auto *listLayout = new QHBoxLayout();
    listLayout->setSpacing(10);

    m_paramListWidget = new QListWidget(this);
    m_paramListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_paramListWidget->setAlternatingRowColors(true);
    m_paramListWidget->setMinimumHeight(132);
    connect(m_paramListWidget, &QListWidget::currentRowChanged, this, &MainWindow::refreshActionButtons);
    listLayout->addWidget(m_paramListWidget, 1);

    auto *sideButtons = new QVBoxLayout();
    sideButtons->setSpacing(7);

    m_removeParamButton = new QPushButton(i18n("Remove"), this);
    m_editParamButton = new QPushButton(i18n("Edit"), this);
    m_moveUpButton = new QPushButton(i18n("Up"), this);
    m_moveDownButton = new QPushButton(i18n("Down"), this);

    connect(m_removeParamButton, &QPushButton::clicked, this, &MainWindow::onRemoveParam);
    connect(m_editParamButton, &QPushButton::clicked, this, &MainWindow::onEditParam);
    connect(m_moveUpButton, &QPushButton::clicked, this, &MainWindow::onMoveUp);
    connect(m_moveDownButton, &QPushButton::clicked, this, &MainWindow::onMoveDown);

    sideButtons->addWidget(m_removeParamButton);
    sideButtons->addWidget(m_editParamButton);
    sideButtons->addWidget(m_moveUpButton);
    sideButtons->addWidget(m_moveDownButton);
    sideButtons->addStretch();
    listLayout->addLayout(sideButtons);
    paramsLayout->addLayout(listLayout);

    auto *addLayout = new QHBoxLayout();
    addLayout->setSpacing(10);
    auto *newLabel = new QLabel(i18n("New parameter"), this);
    m_newParamEdit = new QLineEdit(this);
    m_newParamEdit->setPlaceholderText(i18n("Example: nomodeset"));
    m_addParamButton = new QPushButton(i18n("Add"), this);
    m_addParamButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    connect(m_newParamEdit, &QLineEdit::returnPressed, this, &MainWindow::onAddParam);
    connect(m_addParamButton, &QPushButton::clicked, this, &MainWindow::onAddParam);
    addLayout->addWidget(newLabel);
    addLayout->addWidget(m_newParamEdit, 1);
    addLayout->addWidget(m_addParamButton);
    paramsLayout->addLayout(addLayout);

    mainLayout->addWidget(m_paramsCard, 1);

    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(10);

    m_statusLabel = new QLabel(i18n("Saving requires administrator authentication through polkit."), this);
    m_statusLabel->setObjectName(QStringLiteral("statusLabel"));
    m_statusLabel->setWordWrap(true);

    m_saveButton = new QPushButton(i18n("Save GRUB2 Settings"), this);
    m_saveButton->setObjectName(QStringLiteral("primaryButton"));
    m_saveButton->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::onSaveClicked);

    m_rebootButton = new QPushButton(i18n("Reboot"), this);
    m_rebootButton->setIcon(QIcon::fromTheme(QStringLiteral("system-reboot")));
    connect(m_rebootButton, &QPushButton::clicked, this, &MainWindow::onRebootClicked);

    bottomLayout->addWidget(m_statusLabel, 1);
    bottomLayout->addWidget(m_saveButton);
    bottomLayout->addWidget(m_rebootButton);
    mainLayout->addLayout(bottomLayout);

    setCentralWidget(central);
}

void MainWindow::loadConfig()
{
    if (!m_config->load()) {
        QMessageBox::warning(this, i18n("Load Failed"),
                             i18n("Could not load the GRUB configuration."));
    }

    updateUiFromConfig();
}

void MainWindow::updateUiFromConfig()
{
    m_showMenuCheck->setChecked(m_config->showMenu());
    m_delaySpin->setValue(m_config->timeout());
    m_rememberCheck->setChecked(m_config->rememberLast());
    m_currentParamsDisplay->setPlainText(m_config->currentCmdline());
    setParameterList(m_config->bootParams().split(QLatin1Char(' '), Qt::SkipEmptyParts));
    refreshActionButtons();
}

QStringList MainWindow::parameterList() const
{
    QStringList result;
    for (int i = 0; i < m_paramListWidget->count(); ++i) {
        const auto *item = m_paramListWidget->item(i);
        if (item && !item->text().trimmed().isEmpty()) {
            result << item->text().trimmed();
        }
    }
    return result;
}

void MainWindow::setParameterList(const QStringList &params)
{
    m_paramListWidget->clear();
    for (const QString &param : params) {
        const QString trimmed = param.trimmed();
        if (!trimmed.isEmpty()) {
            m_paramListWidget->addItem(trimmed);
        }
    }
}

bool MainWindow::validateParameter(const QString &param, int ignoredRow)
{
    if (param.isEmpty()) {
        QMessageBox::information(this, i18n("Empty Parameter"), i18n("Enter one kernel parameter first."));
        return false;
    }

    if (param.contains(QRegularExpression(QStringLiteral(R"(\s)")))) {
        QMessageBox::warning(this, i18n("Invalid Parameter"),
                             i18n("Add one parameter at a time. Spaces are not allowed."));
        return false;
    }

    const QRegularExpression safeParamRe(QStringLiteral(R"(^[A-Za-z0-9_./:=,+@%-]+$)"));
    if (!safeParamRe.match(param).hasMatch()) {
        QMessageBox::warning(this, i18n("Invalid Parameter"),
                             i18n("Use only letters, numbers, and these characters: _ . / : = , + @ % -"));
        return false;
    }

    for (int i = 0; i < m_paramListWidget->count(); ++i) {
        if (i == ignoredRow) {
            continue;
        }
        const auto *item = m_paramListWidget->item(i);
        if (item && item->text() == param) {
            QMessageBox::information(this, i18n("Duplicate Parameter"),
                                     i18n("The parameter “%1” already exists.", param));
            return false;
        }
    }

    return true;
}

void MainWindow::onAddParam()
{
    const QString param = m_newParamEdit->text().trimmed();
    if (!validateParameter(param)) {
        return;
    }

    m_paramListWidget->addItem(param);
    m_newParamEdit->clear();
    m_newParamEdit->setFocus();
    refreshActionButtons();
}

void MainWindow::onRemoveParam()
{
    const int row = m_paramListWidget->currentRow();
    if (row >= 0) {
        delete m_paramListWidget->takeItem(row);
    }
    refreshActionButtons();
}

void MainWindow::onEditParam()
{
    const int row = m_paramListWidget->currentRow();
    if (row < 0) {
        return;
    }

    auto *item = m_paramListWidget->item(row);
    bool ok = false;
    const QString edited = QInputDialog::getText(this, i18n("Edit Parameter"), i18n("Parameter"),
                                                 QLineEdit::Normal, item->text(), &ok).trimmed();
    if (ok && validateParameter(edited, row)) {
        item->setText(edited);
    }
}

void MainWindow::onMoveUp()
{
    const int row = m_paramListWidget->currentRow();
    if (row <= 0) {
        return;
    }

    auto *item = m_paramListWidget->takeItem(row);
    m_paramListWidget->insertItem(row - 1, item);
    m_paramListWidget->setCurrentRow(row - 1);
    refreshActionButtons();
}

void MainWindow::onMoveDown()
{
    const int row = m_paramListWidget->currentRow();
    if (row < 0 || row >= m_paramListWidget->count() - 1) {
        return;
    }

    auto *item = m_paramListWidget->takeItem(row);
    m_paramListWidget->insertItem(row + 1, item);
    m_paramListWidget->setCurrentRow(row + 1);
    refreshActionButtons();
}

void MainWindow::refreshActionButtons()
{
    if (!m_paramListWidget->isEnabled()) {
        m_removeParamButton->setEnabled(false);
        m_editParamButton->setEnabled(false);
        m_moveUpButton->setEnabled(false);
        m_moveDownButton->setEnabled(false);
        return;
    }

    const int row = m_paramListWidget->currentRow();
    const bool hasSelection = row >= 0;
    m_removeParamButton->setEnabled(hasSelection);
    m_editParamButton->setEnabled(hasSelection);
    m_moveUpButton->setEnabled(row > 0);
    m_moveDownButton->setEnabled(row >= 0 && row < m_paramListWidget->count() - 1);
}

void MainWindow::onSaveClicked()
{
    m_config->setShowMenu(m_showMenuCheck->isChecked());
    m_config->setTimeout(m_delaySpin->value());
    m_config->setRememberLast(m_rememberCheck->isChecked());
    m_config->setBootParams(parameterList().join(QLatin1Char(' ')));

    QTemporaryFile tempFile(QDir::tempPath() + QStringLiteral("/miryu-grub-config-XXXXXX.cfg"));
    tempFile.setAutoRemove(false);
    if (!tempFile.open()) {
        QMessageBox::critical(this, i18n("Save Failed"), i18n("Could not create a temporary configuration file."));
        return;
    }

    const QString configContent = m_config->generateConfigContent();
    tempFile.write(configContent.toUtf8());
    tempFile.flush();
    const QString tempPath = tempFile.fileName();
    tempFile.close();

    const QString helperPath = resolveHelperPath();
    if (!QFileInfo::exists(helperPath)) {
        QFile::remove(tempPath);
        QMessageBox::critical(this, i18n("Helper Not Found"),
                              i18n("The privileged helper was not found at:\n%1\n\nInstall the project before saving GRUB2 settings.",
                                   helperPath));
        return;
    }

    setBusy(true);
    m_statusLabel->setText(i18n("Waiting for administrator authentication…"));

    QProcess process;
    process.start(QStringLiteral("pkexec"), QStringList() << helperPath << QStringLiteral("--apply") << tempPath);
    if (!process.waitForFinished(120000)) {
        process.kill();
        process.waitForFinished();
        QFile::remove(tempPath);
        setBusy(false);
        QMessageBox::critical(this, i18n("Save Failed"), i18n("Timed out while waiting for polkit authentication."));
        m_statusLabel->setText(i18n("Save did not complete."));
        return;
    }

    const QString stdOut = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QString stdErr = QString::fromUtf8(process.readAllStandardError()).trimmed();
    QFile::remove(tempPath);
    setBusy(false);

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        const QString details = stdErr.isEmpty() ? stdOut : stdErr;
        QMessageBox::critical(this, i18n("Save Failed"),
                              i18n("Could not save and regenerate GRUB2 configuration.\n\n%1", details));
        m_statusLabel->setText(i18n("Save failed. No reboot has been requested."));
        return;
    }

    m_statusLabel->setText(i18n("GRUB2 settings saved. Reboot to use the new boot menu."));
    QMessageBox::information(this, i18n("Saved"),
                             i18n("GRUB2 settings were saved and grub2-mkconfig completed successfully."));
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

void MainWindow::setBusy(bool busy)
{
    m_saveButton->setEnabled(!busy);
    m_rebootButton->setEnabled(!busy);
    m_showMenuCheck->setEnabled(!busy);
    m_delaySpin->setEnabled(!busy);
    m_rememberCheck->setEnabled(!busy);
    m_paramListWidget->setEnabled(!busy);
    m_newParamEdit->setEnabled(!busy);
    m_addParamButton->setEnabled(!busy);
    refreshActionButtons();
}

QString MainWindow::resolveHelperPath() const
{
    const QString installedPath = QStringLiteral(HELPER_PATH);
    if (QFileInfo::exists(installedPath)) {
        return installedPath;
    }

    const QString localPath = QCoreApplication::applicationDirPath()
                              + QLatin1Char('/')
                              + QStringLiteral("miryu-grub-config-helper");
    if (QFileInfo::exists(localPath)) {
        return localPath;
    }

    return installedPath;
}
