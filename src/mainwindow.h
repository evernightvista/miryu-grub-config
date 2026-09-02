#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "grubconfig.h"

#include <QMainWindow>
#include <QPointer>

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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private Q_SLOTS:
    void onSaveClicked();
    void onRebootClicked();
    void onAddParam();
    void onRemoveParam();
    void onEditParam();
    void onMoveUp();
    void onMoveDown();
    void refreshActionButtons();

private:
    void setupUi();
    void loadConfig();
    void updateUiFromConfig();
    void setBusy(bool busy);

    QStringList parameterList() const;
    void setParameterList(const QStringList &params);
    bool validateParameter(const QString &param, int ignoredRow = -1);
    QString resolveHelperPath() const;

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
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_rebootButton = nullptr;
};

#endif // MAINWINDOW_H
