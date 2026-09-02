#ifndef GRUBCONFIGWIDGET_H
#define GRUBCONFIGWIDGET_H

#include "grubconfig.h"

#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QFrame;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QTextEdit;
QT_END_NAMESPACE

// Reusable panel holding every GRUB2 boot-menu and kernel-parameter control.
//
// It is shared by the standalone MainWindow (which appends its own Save and
// Reboot buttons) and by the KDE System Settings KCM, whose Reset/Apply
// buttons are provided by the KCModule framework. The widget owns a GrubConfig
// model and centralizes the parameter-editing logic, the dirty-state tracking,
// and the KAuth-protected asynchronous save flow so neither host has to
// duplicate it.
//
// Hosts wire their framework entry points like this:
//   * "Reset" / KCModule::load()  -> loadFromConfig()
//   * "Apply" / KCModule::save()  -> applySettingsAsync()
//   * "Defaults"                  -> restoreDefaults()
//
// The save is asynchronous: applySettingsAsync() returns immediately and
// emits saveSucceeded() or saveFailed() when the KAuth helper finishes.
//
// All user-visible strings use i18nd("miryu-grub-config", ...) so the
// translation catalog is found both inside the standalone application (which
// sets the application domain) and inside the KCM plugin (loaded into the
// systemsettings process).
class GrubConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GrubConfigWidget(QWidget *parent = nullptr);
    ~GrubConfigWidget() override;

    // Repopulate every control from the GrubConfig model (re-reads
    // /etc/default/grub and /proc/cmdline) and clears the dirty flag.
    void loadFromConfig();

    // Copy the on-screen values back into the GrubConfig model so that
    // GrubConfig::generateConfigContent() produces the drop-in content that is
    // handed to the privileged helper.
    void writeToConfig();

    // Reset the on-screen controls to sensible GRUB defaults (no save).
    void restoreDefaults();

    // Start the asynchronous KAuth save flow. Returns immediately; the
    // result is delivered via saveSucceeded() / saveFailed() signals.
    // While the save is in progress, input controls are disabled and
    // the status line shows the current state.
    void applySettingsAsync();

    GrubConfig *config() const { return m_config; }

    // True when the on-screen values differ from the loaded configuration.
    bool isDirty() const;

    // Status line shown at the bottom of the settings area.
    QLabel *statusLabel() const { return m_statusLabel; }
    void setStatus(const QString &text);

    // Disable/enable input controls while a save is in progress.
    void setBusy(bool busy);

Q_SIGNALS:
    // Emitted whenever the dirty state (isDirty()) flips. The carried value
    // is the new dirty state, so hosts can drive Apply/Save enablement.
    void changed(bool dirty);

    // Emitted when the asynchronous KAuth save completes successfully.
    void saveSucceeded();

    // Emitted when the asynchronous KAuth save fails. The string carries
    // a human-readable error message.
    void saveFailed(const QString &errorMessage);

private Q_SLOTS:
    void onAddParam();
    void onRemoveParam();
    void onEditParam();
    void onMoveUp();
    void onMoveDown();
    void refreshActionButtons();
    void emitChanged();

private:
    void setupUi();
    void updateUiFromConfig();
    void snapshotLoadedState();

    QStringList parameterList() const;
    void setParameterList(const QStringList &params);
    bool validateParameter(const QString &param, int ignoredRow = -1);

    QPointer<GrubConfig> m_config;

    QFrame *m_bootCard = nullptr;
    QCheckBox *m_showMenuCheck = nullptr;
    QSpinBox *m_delaySpin = nullptr;
    QCheckBox *m_rememberCheck = nullptr;

    QFrame *m_paramsCard = nullptr;
    QTextEdit *m_currentParamsDisplay = nullptr;
    QListWidget *m_paramListWidget = nullptr;
    QLineEdit *m_newParamEdit = nullptr;
    QPushButton *m_addParamButton = nullptr;
    QPushButton *m_removeParamButton = nullptr;
    QPushButton *m_editParamButton = nullptr;
    QPushButton *m_moveUpButton = nullptr;
    QPushButton *m_moveDownButton = nullptr;

    QLabel *m_statusLabel = nullptr;

    // Snapshot of the loaded values, used to compute isDirty().
    bool m_loadedShowMenu = true;
    int m_loadedTimeout = 10;
    bool m_loadedRememberLast = false;
    QString m_loadedParams;
    bool m_dirty = false;
    bool m_loading = false;  // suppresses change signals while reloading
    bool m_saving = false;   // true while an async KAuth save is in flight
};

#endif // GRUBCONFIGWIDGET_H
