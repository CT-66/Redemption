#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <clocale>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    setlocale(LC_NUMERIC, "C");

    // single instance check
    const QString serverName = "RedemptionInstance";
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
