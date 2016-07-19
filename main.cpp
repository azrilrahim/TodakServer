#include <QCoreApplication>
#include "webserver.h"


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    webServer server;

    int port;
    port = QString(argv[1]).toInt();

    server.startServer(port,argv[2]);
    return a.exec();
}
