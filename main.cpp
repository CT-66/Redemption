#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QDir>
#include <clocale>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    setlocale(LC_NUMERIC, "C");

    // single instance check — user and session specific
    QString session = qgetenv("XDG_SESSION_ID");
    if (session.isEmpty())
        session = qgetenv("DISPLAY");
    if (session.isEmpty())
        session = "default";

    const QString serverName = QString("RedemptionInstance-%1-%2")
        .arg(qgetenv("USER").constData())
        .arg(session);

    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(500)) {
        // another instance is running, send a signal and exit
        socket.write("raise");
        socket.waitForBytesWritten(500);
        socket.close();
        return 0;
    }

    // no existing instance, start server
    QLocalServer server;
    QLocalServer::removeServer(serverName);
    server.listen(serverName);

    QApplication::setOrganizationName("Redemption");
    QApplication::setApplicationName("Redemption");

    QDir().mkpath(QStandardPaths::writableLocation(
        QStandardPaths::GenericConfigLocation) + "/Redemption");

    MainWindow window;
    window.show();

    // when another instance tries to connect, raise our window
    QObject::connect(&server, &QLocalServer::newConnection, [&]() {
        QLocalSocket *client = server.nextPendingConnection();
        client->deleteLater();
        window.raise();
        window.activateWindow();
        window.showNormal();
    });

    return app.exec();
}
