#include "mainwindow.h"

#include <QApplication>
#include <KLocalizedString>
#include <QLocalServer>
#include <QLocalSocket>
#include <QWindow>

#include <unistd.h>

#ifndef APP_ID
#define APP_ID "org.miryu.grubconfig"
#endif

// 单实例 socket 名，按用户区分，避免多用户同时登录时冲突。
// 当第二次启动（例如从 KDE 系统设置的 KCM 自动唤起）连接到该 socket 时，
// 正在运行的实例会把自己的窗口提到前台，而不是再开一个重复窗口。
static QString instanceSocketName()
{
    return QStringLiteral("miryu-grub-config-%1").arg(::getuid());
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("miryu-grub-config");

    app.setApplicationName(QStringLiteral("miryu-grub-config"));
    app.setApplicationDisplayName(i18n("Miryu GRUB2 Boot Config"));
    app.setOrganizationName(QStringLiteral("Miryu"));
    app.setDesktopFileName(QStringLiteral(APP_ID));

    // 单实例守卫：若已有实例在运行，则通知它把自己的窗口置顶，然后立即退出，
    // 不再打开第二个窗口（KCM 自动唤起时由这里拦截）。
    const QString socketName = instanceSocketName();
    QLocalSocket probe;
    probe.connectToServer(socketName);
    if (probe.waitForConnected(200)) {
        probe.disconnectFromServer();
        return 0;
    }

    // 第一个实例：占用 socket，后续启动者连进来时把本窗口提到前台。
    QLocalServer::removeServer(socketName);
    QLocalServer server;
    server.listen(socketName);

    MainWindow window;
    window.show();

    QObject::connect(&server, &QLocalServer::newConnection, &server, [&window, &server]() {
        // 取走挂起的连接，连接事件本身就是“把已有窗口提到前台”的信号。
        if (QLocalSocket *s = server.nextPendingConnection())
            s->deleteLater();
        window.showNormal();
        window.raise();
        window.activateWindow();
        if (QWindow *w = window.windowHandle())
            w->requestActivate();
    });

    return app.exec();
}
