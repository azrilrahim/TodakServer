/*
#------------------------------------------#
#                                          #
# Todak Server 0.1                         #
# Copyright (C) 2015 -2016 Azril Azam      #
#                                          #
# azrilazam@gmail.com                      #
#                                          #
#------------------------------------------#
*/


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
