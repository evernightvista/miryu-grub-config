#include "grubconfigwidget.h"

#include <KAuth/Action>
#include <KAuth/ExecuteJob>
#include <KLocalizedString>

#include <QAbstractItemView>
#include <QCheckBox>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <KJob>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QOverload>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>

static const char *const kDomain = "miryu-grub-config";

GrubConfigWidget::GrubConfigWidget(QWidget *parent)
    : QWidget(parent)
    , m_config(new GrubConfig(this))
{
    setupUi();
    loadFromConfig();
}

GrubConfigWidget::~GrubConfigWidget() = default;

void GrubConfigWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(26, 24, 26, 20);
    mainLayout->setSpacing(16);

    auto *subtitle = new QLabel(i18nd(kDomain, "Tune the GRUB2 boot menu, kernel parameters, and saved default entry."), this);
    subtitle->setObjectName(QStringLiteral("windowSubtitle"));
    subtitle->setWordWrap(true);
    mainLayout->addWidget(subtitle);

    m_bootCard = new QFrame(this);
    m_bootCard->setObjectName(QStringLiteral("card"));
    m_bootCard->setFrameShape(QFrame::StyledPanel);
    m_bootCard->setFrameShadow(QFrame::Plain);
    auto *bootLayout = new QVBoxLayout(m_bootCard);
    bootLayout->setContentsMargins(18, 16, 18, 16);
    bootLayout->setSpacing(10);

    auto *bootTitle = new QLabel(i18nd(kDomain, "Boot Menu"), this);
    bootTitle->setObjectName(QStringLiteral("cardTitle"));
    QFont cardTitleFont = bootTitle->font();
    cardTitleFont.setPointSize(cardTitleFont.pointSize() + 2);
    cardTitleFont.setBold(true);
    bootTitle->setFont(cardTitleFont);
    bootLayout->addWidget(bootTitle);

    m_showMenuCheck = new QCheckBox(i18nd(kDomain, "Show boot menu"), this);
    m_showMenuCheck->setToolTip(i18nd(kDomain, "Show the GRUB menu before starting the default system."));
    bootLayout->addWidget(m_showMenuCheck);

    auto *delayLayout = new QHBoxLayout();
    delayLayout->setSpacing(10);
    auto *delayLabel = new QLabel(i18nd(kDomain, "Delay before booting"), this);
    m_delaySpin = new QSpinBox(this);
    m_delaySpin->setRange(-1, 3600);
    m_delaySpin->setSuffix(i18nd(kDomain, " seconds"));
    m_delaySpin->setSpecialValueText(i18nd(kDomain, "wait indefinitely"));
    m_delaySpin->setToolTip(i18nd(kDomain, "Use -1 to wait until a menu entry is selected."));
    delayLayout->addWidget(delayLabel);
    delayLayout->addWidget(m_delaySpin);
    delayLayout->addStretch();
    bootLayout->addLayout(delayLayout);

    m_rememberCheck = new QCheckBox(i18nd(kDomain, "Remember last selected entry as the default"), this);
    bootLayout->addWidget(m_rememberCheck);

    mainLayout->addWidget(m_bootCard);

    m_paramsCard = new QFrame(this);
    m_paramsCard->setObjectName(QStringLiteral("card"));
    m_paramsCard->setFrameShape(QFrame::StyledPanel);
    m_paramsCard->setFrameShadow(QFrame::Plain);
    auto *paramsLayout = new QVBoxLayout(m_paramsCard);
    paramsLayout->setContentsMargins(18, 16, 18, 16);
    paramsLayout->setSpacing(10);

    auto *paramsTitle = new QLabel(i18nd(kDomain, "Kernel Parameters"), this);
    paramsTitle->setObjectName(QStringLiteral("cardTitle"));
    paramsTitle->setFont(cardTitleFont);
    paramsLayout->addWidget(paramsTitle);

    auto *currentLabel = new QLabel(i18nd(kDomain, "Current kernel command line"), this);
    currentLabel->setObjectName(QStringLiteral("sectionLabel"));
    paramsLayout->addWidget(currentLabel);

    m_currentParamsDisplay = new QTextEdit(this);
    m_currentParamsDisplay->setReadOnly(true);
    m_currentParamsDisplay->setMinimumHeight(76);
    m_currentParamsDisplay->setMaximumHeight(92);
    paramsLayout->addWidget(m_currentParamsDisplay);

    auto *customLabel = new QLabel(i18nd(kDomain, "Custom parameters written by this tool"), this);
    customLabel->setObjectName(QStringLiteral("sectionLabel"));
    paramsLayout->addWidget(customLabel);

    auto *listLayout = new QHBoxLayout();
    listLayout->setSpacing(10);

    m_paramListWidget = new QListWidget(this);
    m_paramListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_paramListWidget->setAlternatingRowColors(true);
    m_paramListWidget->setMinimumHeight(132);
    connect(m_paramListWidget, &QListWidget::currentRowChanged, this, &GrubConfigWidget::refreshActionButtons);
    listLayout->addWidget(m_paramListWidget, 1);

    auto *sideButtons = new QVBoxLayout();
    sideButtons->setSpacing(7);

    m_removeParamButton = new QPushButton(i18nd(kDomain, "Remove"), this);
    m_editParamButton = new QPushButton(i18nd(kDomain, "Edit"), this);
    m_moveUpButton = new QPushButton(i18nd(kDomain, "Up"), this);
    m_moveDownButton = new QPushButton(i18nd(kDomain, "Down"), this);

    connect(m_removeParamButton, &QPushButton::clicked, this, &GrubConfigWidget::onRemoveParam);
    connect(m_editParamButton, &QPushButton::clicked, this, &GrubConfigWidget::onEditParam);
    connect(m_moveUpButton, &QPushButton::clicked, this, &GrubConfigWidget::onMoveUp);
    connect(m_moveDownButton, &QPushButton::clicked, this, &GrubConfigWidget::onMoveDown);

    sideButtons->addWidget(m_removeParamButton);
    sideButtons->addWidget(m_editParamButton);
    sideButtons->addWidget(m_moveUpButton);
    sideButtons->addWidget(m_moveDownButton);
    sideButtons->addStretch();
    listLayout->addLayout(sideButtons);
    paramsLayout->addLayout(listLayout);

    auto *addLayout = new QHBoxLayout();
    addLayout->setSpacing(10);
    auto *newLabel = new QLabel(i18nd(kDomain, "New parameter"), this);
    m_newParamEdit = new QLineEdit(this);
    m_newParamEdit->setPlaceholderText(i18nd(kDomain, "Example: nomodeset"));
    m_addParamButton = new QPushButton(i18nd(kDomain, "Add"), this);
    m_addParamButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    connect(m_newParamEdit, &QLineEdit::returnPressed, this, &GrubConfigWidget::onAddParam);
    connect(m_addParamButton, &QPushButton::clicked, this, &GrubConfigWidget::onAddParam);
    addLayout->addWidget(newLabel);
    addLayout->addWidget(m_newParamEdit, 1);
    addLayout->addWidget(m_addParamButton);
    paramsLayout->addLayout(addLayout);

    mainLayout->addWidget(m_paramsCard, 1);

    m_statusLabel = new QLabel(i18nd(kDomain, "Saving requires administrator authentication."), this);
    m_statusLabel->setObjectName(QStringLiteral("statusLabel"));
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);

    // Any value edit toggles the dirty flag.
    connect(m_showMenuCheck, &QCheckBox::toggled, this, &GrubConfigWidget::emitChanged);
    connect(m_delaySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { emitChanged(); });
    connect(m_rememberCheck, &QCheckBox::toggled, this, &GrubConfigWidget::emitChanged);

    refreshActionButtons();
}

void GrubConfigWidget::loadFromConfig()
{
    // Suppress change signals while repopulating, otherwise the setValue /
    // setChecked calls below would briefly report the new values as dirty
    // against the still-old snapshot.
    m_loading = true;
    if (!m_config->load()) {
        setStatus(i18nd(kDomain, "Could not load the GRUB configuration."));
    }

    updateUiFromConfig();
    snapshotLoadedState();

    m_loading = false;
    m_dirty = false;
    Q_EMIT changed(false);
}

void GrubConfigWidget::updateUiFromConfig()
{
    m_showMenuCheck->setChecked(m_config->showMenu());
    m_delaySpin->setValue(m_config->timeout());
    m_rememberCheck->setChecked(m_config->rememberLast());
    m_currentParamsDisplay->setPlainText(m_config->currentCmdline());
    setParameterList(m_config->bootParams().split(QLatin1Char(' '), Qt::SkipEmptyParts));
    refreshActionButtons();
}

void GrubConfigWidget::writeToConfig()
{
    m_config->setShowMenu(m_showMenuCheck->isChecked());
    m_config->setTimeout(m_delaySpin->value());
    m_config->setRememberLast(m_rememberCheck->isChecked());
    m_config->setBootParams(parameterList().join(QLatin1Char(' ')));
}

void GrubConfigWidget::restoreDefaults()
{
    m_showMenuCheck->setChecked(true);
    m_delaySpin->setValue(10);
    m_rememberCheck->setChecked(false);
    setParameterList({});
    emitChanged();
}

void GrubConfigWidget::snapshotLoadedState()
{
    m_loadedShowMenu = m_config->showMenu();
    m_loadedTimeout = m_config->timeout();
    m_loadedRememberLast = m_config->rememberLast();
    m_loadedParams = m_config->bootParams().split(QLatin1Char(' '), Qt::SkipEmptyParts).join(QLatin1Char(' '));
}

bool GrubConfigWidget::isDirty() const
{
    if (m_showMenuCheck->isChecked() != m_loadedShowMenu) {
        return true;
    }
    if (m_delaySpin->value() != m_loadedTimeout) {
        return true;
    }
    if (m_rememberCheck->isChecked() != m_loadedRememberLast) {
        return true;
    }
    if (parameterList().join(QLatin1Char(' ')) != m_loadedParams) {
        return true;
    }
    return false;
}

void GrubConfigWidget::setStatus(const QString &text)
{
    if (m_statusLabel) {
        m_statusLabel->setText(text);
    }
}

QStringList GrubConfigWidget::parameterList() const
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

void GrubConfigWidget::setParameterList(const QStringList &params)
{
    m_paramListWidget->clear();
    for (const QString &param : params) {
        const QString trimmed = param.trimmed();
        if (!trimmed.isEmpty()) {
            m_paramListWidget->addItem(trimmed);
        }
    }
}

bool GrubConfigWidget::validateParameter(const QString &param, int ignoredRow)
{
    if (param.isEmpty()) {
        QMessageBox::information(this, i18nd(kDomain, "Empty Parameter"), i18nd(kDomain, "Enter one kernel parameter first."));
        return false;
    }

    if (param.contains(QRegularExpression(QStringLiteral(R"(\s)")))) {
        QMessageBox::warning(this, i18nd(kDomain, "Invalid Parameter"),
                             i18nd(kDomain, "Add one parameter at a time. Spaces are not allowed."));
        return false;
    }

    const QRegularExpression safeParamRe(QStringLiteral(R"(^[A-Za-z0-9_./:=,+@%-]+$)"));
    if (!safeParamRe.match(param).hasMatch()) {
        QMessageBox::warning(this, i18nd(kDomain, "Invalid Parameter"),
                             i18nd(kDomain, "Use only letters, numbers, and these characters: _ . / : = , + @ % -"));
        return false;
    }

    for (int i = 0; i < m_paramListWidget->count(); ++i) {
        if (i == ignoredRow) {
            continue;
        }
        const auto *item = m_paramListWidget->item(i);
        if (item && item->text() == param) {
            QMessageBox::information(this, i18nd(kDomain, "Duplicate Parameter"),
                                     i18nd(kDomain, "The parameter \xe2\x80\x9c%1\xe2\x80\x9d already exists.", param));
            return false;
        }
    }

    return true;
}

void GrubConfigWidget::onAddParam()
{
    const QString param = m_newParamEdit->text().trimmed();
    if (!validateParameter(param)) {
        return;
    }

    m_paramListWidget->addItem(param);
    m_newParamEdit->clear();
    m_newParamEdit->setFocus();
    refreshActionButtons();
    emitChanged();
}

void GrubConfigWidget::onRemoveParam()
{
    const int row = m_paramListWidget->currentRow();
    if (row >= 0) {
        delete m_paramListWidget->takeItem(row);
    }
    refreshActionButtons();
    emitChanged();
}

void GrubConfigWidget::onEditParam()
{
    const int row = m_paramListWidget->currentRow();
    if (row < 0) {
        return;
    }

    auto *item = m_paramListWidget->item(row);
    bool ok = false;
    const QString edited = QInputDialog::getText(this, i18nd(kDomain, "Edit Parameter"), i18nd(kDomain, "Parameter"),
                                                  QLineEdit::Normal, item->text(), &ok).trimmed();
    if (ok && validateParameter(edited, row)) {
        item->setText(edited);
        emitChanged();
    }
}

void GrubConfigWidget::onMoveUp()
{
    const int row = m_paramListWidget->currentRow();
    if (row <= 0) {
        return;
    }

    auto *item = m_paramListWidget->takeItem(row);
    m_paramListWidget->insertItem(row - 1, item);
    m_paramListWidget->setCurrentRow(row - 1);
    refreshActionButtons();
    emitChanged();
}

void GrubConfigWidget::onMoveDown()
{
    const int row = m_paramListWidget->currentRow();
    if (row < 0 || row >= m_paramListWidget->count() - 1) {
        return;
    }

    auto *item = m_paramListWidget->takeItem(row);
    m_paramListWidget->insertItem(row + 1, item);
    m_paramListWidget->setCurrentRow(row + 1);
    refreshActionButtons();
    emitChanged();
}

void GrubConfigWidget::refreshActionButtons()
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

void GrubConfigWidget::emitChanged()
{
    if (m_loading) {
        return;
    }
    const bool dirty = isDirty();
    if (dirty != m_dirty) {
        m_dirty = dirty;
        Q_EMIT changed(m_dirty);
    }
}

void GrubConfigWidget::setBusy(bool busy)
{
    m_showMenuCheck->setEnabled(!busy);
    m_delaySpin->setEnabled(!busy);
    m_rememberCheck->setEnabled(!busy);
    m_paramListWidget->setEnabled(!busy);
    m_newParamEdit->setEnabled(!busy);
    m_addParamButton->setEnabled(!busy);
    refreshActionButtons();
}

void GrubConfigWidget::applySettingsAsync()
{
    if (m_saving) {
        return;
    }

    writeToConfig();

    const QString configContent = m_config->generateConfigContent();

    m_saving = true;
    setBusy(true);
    setStatus(i18nd(kDomain, "Waiting for administrator authentication\xe2\x80\xa6"));

    KAuth::Action action(QStringLiteral("org.miryugaming.grubconfig.save"));
    action.setHelperId(QStringLiteral("org.miryugaming.grubconfig"));
    action.setArguments({{QStringLiteral("content"), configContent}});

    KAuth::ExecuteJob *job = action.execute();
    connect(job, &KJob::result, this, [this](KJob *job) {
        m_saving = false;
        setBusy(false);

        if (job->error() != KJob::NoError) {
            const QString error = job->errorText();
            setStatus(i18nd(kDomain, "Save failed. No reboot has been requested."));
            Q_EMIT saveFailed(error);
            return;
        }

        // The helper may have rewritten /etc/default/grub and grubby may have
        // touched BLS entries, so reload the model to reflect the new on-disk
        // state and clear the dirty flag.
        loadFromConfig();
        setStatus(i18nd(kDomain, "GRUB2 settings saved. Reboot to use the new boot menu."));
        Q_EMIT saveSucceeded();
    });
    job->start();
}
