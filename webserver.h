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

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QObject>
#include <QTcpServer>
#include <clienthandler.h>
#include <stdio.h>
#include <QDebug>
#include <structure.h>
#include <QFile>
#include <QDir>


class webServer : public QObject
{
    Q_OBJECT
public:
    explicit webServer(QObject *parent = 0);
    bool startServer(int port = 80, QString configFile = "");
    ~webServer();

private:
    QTcpServer *serverTCP;
    clientHandler *clientHWND;
    QList<FileExec>fileExecList;

    QList<ProcessStat>PROCVECT;

    QString workDir;



    bool readConfigFile(QString file);

private slots:
    void newConnection();

signals:

public slots:
};

#endif // WEBSERVER_H
