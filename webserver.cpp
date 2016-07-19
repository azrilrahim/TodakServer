#include "webserver.h"
#include <qcoreapplication.h>

webServer::webServer(QObject *parent) : QObject(parent)
{
    this->serverTCP = new QTcpServer(this);
    connect(this->serverTCP,SIGNAL(newConnection()),this,SLOT(newConnection()));
}

bool webServer::startServer(int port, QString configFile)
{

    if (configFile.isEmpty()){
        configFile = "/opt/web.cfg";
    }

    //read config file
    qDebug() << "reading " << configFile;
    if(!this->readConfigFile(configFile)){
        return false;
    }

    //read process stat
    if (!this->serverTCP->listen(QHostAddress::Any,port)){
        printf("Error:%s\n",this->serverTCP->errorString().toStdString().c_str());
        printf("TODAK WEB SERVER stop\n");
        return false;
    }

    printf("TODAK WEB SERVER started at %d\n",port);
    printf("Working dir is %s\n",this->workDir.toStdString().c_str());
    return true;
}

webServer::~webServer()
{
    this->serverTCP->close();
    this->clientHWND->deleteLater();
}

void webServer::newConnection()
{
    this->clientHWND = new clientHandler(this);

    this->clientHWND->setWorkingDirectory(this->workDir);
    this->clientHWND->setProcessVector(&this->PROCVECT);
    this->clientHWND->processClient(this->serverTCP->nextPendingConnection(),this->fileExecList);

    this->clientHWND->deleteLater();
}

bool webServer::readConfigFile(QString file){

    //start readingfile
    QList<QString>line;
    QFile cfgFile(file);
    QString lineStr;
    QList<QString>process;
    FileExec fe;

    quint16 at;


    if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        printf("Error: Could not open config file %s\n",file.toStdString().c_str());
        return false;
    }

    line = QString(cfgFile.readAll()).split("\n");
    cfgFile.close();

    for (at = 0; at < line.size(); at++){
        lineStr = line.at(at).trimmed();
        if (!lineStr.isEmpty()){
            if (!lineStr.contains("#")){

                qDebug() << lineStr;

                //executed files
                if (lineStr.contains("fileexec:")){
                    process = lineStr.split(":");
                    if (process.size() != 4){
                        return false;
                        printf("Error: Config File line %d\n",at);
                    }
                    fe.ext = process.at(1);
                    fe.execType = 2; //execute code
                    fe.contentType = process.at(2);
                    fe.scriptParent = process.at(3);

                    //check if script parent == ? then exec type is 2 (execute + produced content type)
                    //if (fe.contentType.contains("?")){
                     //   fe.execType = 2;
                    //}
                    this->fileExecList.append(fe);
                }

                //readonly files
                if (lineStr.contains("fileread:")){
                    process = lineStr.split(":");
                    if (process.size() != 3){
                        return false;
                        printf("Error: Config File line %d\n",at);
                    }
                    fe.ext = process.at(1);
                    fe.execType = 1; //execute code
                    fe.contentType = process.at(2);
                    fe.scriptParent.clear();
                    this->fileExecList.append(fe);
                }

                if (lineStr.contains("workingdir:")){
                    process = lineStr.split(":");
                    this->workDir = process.at(1);

                    //check workdir
                    if (!QDir(this->workDir).exists()){
                        printf("Error: Invalid working directory\n");
                        return false;
                    }

                }

                //index page

                //error page

            }
        }

    }

    return true;

}

