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
#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <structure.h>
#include <QFile>
#include <QProcess>
#include <QElapsedTimer>
#include <QUrl>
#include <QCoreApplication>
#include <QChar>
#include <QDateTime>

class clientHandler : public QObject
{
    Q_OBJECT
public:
    explicit clientHandler(QObject *parent = 0);
    ~clientHandler();
    void processClient(QTcpSocket *socket, QList<FileExec> fileExecList);
    void setWorkingDirectory(QString dir);
    void setProcessVector(QList<ProcessStat> *PROCVECT);

private:
    QTcpSocket *clientTCP;
    QList<FileExec> fileExecList;
    QList<ProcessStat> *PROCVECT;

    executeData processGET(QString getURL);
    executeData processPOST(QString postURL, QByteArray boundary, QByteArray *data);
    QByteArray produceHttpReply(executeData ed);
    QString getRequestURLFile(QString rURL);
    QString getRequestURLQuery(QString rURL);

    executeData processSystemStatus();
    executeData processClientRequest(QByteArray *data, qint64 headerLoc);
    multipartArgv processPOSTMultiPart(QByteArray data);

    executeData processFile(QString file, QString argv);
    QString getFileExtention(QString file);
    FileExec getFileExecStatus(QString ext);

    QByteArray executeFile(QString file, QString argv);
    QByteArray readFile(QString file);

    QString getPOSTURL(QStringList postDataList);
    QString extractStr(QString src, QChar start, QChar end, quint8 direction);
    QString urlDecode(QString url);

    QByteArray utilExtractData(const QByteArray searchKey, const QByteArray endKey, quint64 start, const QByteArray *srcData);

    QString workDir;

    QString getTmpRandomFileName();





private slots:
    void socketDisconnected();

signals:

public slots:
};

#endif // CLIENTHANDLER_H
