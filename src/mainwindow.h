#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
QT_END_NAMESPACE

class GrubConfigWidget;

// Standalone window for "miryu-grub-config". It wraps the shared
// GrubConfigWidget and adds a window title plus its own Save and Reboot
// buttons. The actual boot-menu/kernel-parameter editing and the KAuth save
// flow live in GrubConfigWidget so they can be reused verbatim by the KDE
// System Settings KCM.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private Q_SLOTS:
    void onSaveClicked();
    void onRebootClicked();

private:
    QPointer<GrubConfigWidget> m_widget;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_rebootButton = nullptr;
};

#endif // MAINWINDOW_H
