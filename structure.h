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

#ifndef STRUCTURE
#define STRUCTURE

//#include <QList>
//#include <QVector>
#include <QStringList>

struct HttpHeader
{
    QString Request;
    QString multiSep;
    quint16 sizeToContent;
};

/*struct Query
{
    quint8 type; //0 text, 1 file;
    QString name;
    QString fileName;
    QString contentType;
    QByteArray data;
};*/

struct FileEx
{
    QString ext;
    QString loader;
    QString tgtFile;
    QString argvDeliminator;
};

struct ProcessStat
{
    QString processName;
    QString processMD5;
    QString processCreated;
    quint64 call;
    quint64 fail;
    quint64 lastFailTIMESTMAP;
    qint64 slowestTIME;
    quint64 lastSlowTIMESTAMP;
    qint64 fastestTIME;
    quint64 lastFastTIMESTAMP;
};

struct FileExec
{
    QString ext; //file extension
    quint8 execType;
    QString contentType;
    QString scriptParent;
};

struct executeData
{
    bool OK;
    QStringList tmpFiles;
    QByteArray data;
    QString contentType;
};

struct multipartArgv
{
    QString tmpFile;
    QString argv;
};

//QList<FileExec> gdbFileExec;

#endif // STRUCTURE

