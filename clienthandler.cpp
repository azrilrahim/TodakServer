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

#include "clienthandler.h"
#include <QDebug>

clientHandler::clientHandler(QObject *parent) : QObject(parent)
{
    //this->clientTCP = new QTcpSocket(this);
    this->workDir = ".";
}

clientHandler::~clientHandler()
{

}

void clientHandler::setWorkingDirectory(QString dir)
{
    this->workDir = dir;
}

void clientHandler::setProcessVector(QList<ProcessStat> *PROCVECT)
{
    this->PROCVECT = PROCVECT;
}

multipartArgv clientHandler::processPOSTMultiPart(QByteArray data)
{
    multipartArgv rs;

    qint64 loc;
    QList<QByteArray> hdr;
    QString tmp;
    QByteArray partData;
    QString argv;
    QFile tmpFile;

    argv.clear();
    tmp.clear();
    loc = 0;
    partData.clear();
    tmpFile.fileName().clear();

    rs.argv.clear();
    rs.tmpFile.clear();

    //remove header and content;
    loc = data.indexOf("\r\n\r\n");
    if (loc < 0){
        return rs;
    }

    //part data
    partData.append(data.mid(loc + 4,data.size() - (loc + 4)));

    //process header
    data = data.remove(loc,data.size() - loc);
    data = data.replace('\r',"");
    hdr = data.split('\n');
    data.clear();

    //skip saving as file.. concentrate on base64 first
    //save partData is size > 1024
    /*if (partData.size() > 1024)
    {
        qDebug() << "multipart saving data as file";
        tmpFile.setFileName("/tmp/" + this->getTmpRandomFileName()+ ".tmp");
        if (tmpFile.open(QIODevice::WriteOnly)){
            tmpFile.write(partData);
            tmpFile.close();
            rs.tmpFile = tmpFile.fileName();
    }
    }*/

    //rs.argv.append("'http' ");

    for (int i=0;i < hdr.size();i++){

        tmp.clear();
        QCoreApplication::processEvents();

        //we only process if the the line is content disposition
        if (hdr.at(i).contains("Content-Disposition:")){

            QString name, filename;
            name.clear();
            filename.clear();

            qDebug() << "processing:" << hdr.at(i) << " with data" << partData;

            //locate name;
            name = QString::fromLocal8Bit(this->utilExtractData("name=",";",0,&hdr.at(i)));
            name = name.replace('"',"");

            filename = QString::fromLocal8Bit(this->utilExtractData("filename=",";",0,&hdr.at(i)));
            filename = filename.replace('"',"");


            if (!name.isEmpty())
            {
                /*if (partData.size() > 1024){
                    rs.argv.append("'" + name + "?=localfile:" + tmpFile.fileName() + "' ");
                }
                else
                {*/
                    rs.argv.append("'" + name + "?=" + QString::fromLocal8Bit(partData.toBase64()) + "' ");
                //}
            }

            if (!filename.isEmpty())
            {
                rs.argv.append("'filename?=" + filename + "' ");
            }
        }
    }

    //qDebug() << "Done process multipart\n\n";

    return rs;
}

executeData clientHandler::processClientRequest(QByteArray *data,qint64 headerLoc)
{
    executeData ed;
    qint64 loc, locBoundary;
    QByteArray urlData;
    QByteArray boundary;

    urlData.clear();
    boundary.clear();
    loc =0;

    //ed default value for errror;
    ed.contentType ="text/html";
    ed.data = "Invalid http format";
    ed.OK = false;

    //Find GET
    if (data->indexOf("GET",0) == 0){
        qDebug() << "process GET";
        urlData.append(data->mid(0,data->indexOf("\n")).split(' ').at(1));
        return this->processGET(QString::fromLocal8Bit(urlData.trimmed()));
    }

    //Find POST
    if (data->indexOf("POST",0) == 0)
    {
        qDebug() << "process POST";
        urlData.append(data->mid(0,data->indexOf("\n")).split(' ').at(1));

        //check if there is multipart;
        loc = data->indexOf("multipart",0);
        if (loc > 0){
            //get boundary
            loc = data->indexOf("boundary",0);
            if (loc > 0){
                locBoundary = data->indexOf("\r\n",loc);
                if (locBoundary > 0){
                    boundary.append(data->mid(loc+9,locBoundary - (loc+9)).trimmed());
                }
                else{
                    return ed;
                }
            }
            else{
                return ed;
            }
        }

        *data = data->remove(0,headerLoc + 4);
        qDebug() << "===============================";
        return this->processPOST(QString::fromLocal8Bit(urlData),boundary,data);
    }

    return ed;
}

void clientHandler::processClient(QTcpSocket *socket, QList<FileExec>fileExecList)
{
    //QByteArray inData;
    //QString inStr;
    //QStringList list;
    //QStringList ops;
    bool getProcess;
    //bool processHeader;
    //QString resultStr;
    executeData ed;
    QByteArray inData;
    qint64 loc;
    qint64 contentLength;
    qint64 requestLength;
    qint64 headerLength;
    quint64 timeout;
    bool processHeader;
    bool dataRecieved;

    processHeader = false;
    this->clientTCP = socket;
    this->fileExecList = fileExecList;
    contentLength = 0;
    requestLength = 0;
    loc = 0;
    getProcess = false;
    dataRecieved = false;
    timeout = 0;

    connect(this->clientTCP,SIGNAL(disconnected()),this,SLOT(socketDisconnected()));
    if (!this->clientTCP->waitForReadyRead(100)){
        this->clientTCP->disconnectFromHost();
        this->clientTCP->deleteLater();
        return;
    }


    inData.clear();
    while(1){

        if (timeout == 30000){
            break;
        }

        //we need to read the first data and determined content length
        if (this->clientTCP->bytesAvailable() > 0){
            qDebug() << this->clientTCP->bytesAvailable();
            timeout = 0;
            inData.append(this->clientTCP->readAll());
            QCoreApplication::processEvents();

            if (!processHeader){

                headerLength = inData.indexOf("\r\n\r\n",0);
                if (headerLength > 0){
                    processHeader = true;

                    //find content length;
                    loc = inData.indexOf("Content-Length:");
                    if (loc < 0){
                        contentLength = 0;
                    }

                    if (loc > 0){
                        //get the size;
                        qint64 loc2 = inData.indexOf("\r\n",loc);
                        if (loc2 < 0){
                            contentLength = 0;
                        }
                        else
                        {
                            QByteArray size;
                            size.clear();
                            size.append(inData.mid(loc,loc2 - loc).replace("Content-Length:","").trimmed());
                            contentLength = size.toULongLong();
                        }
                    }
                }
            }

            if (processHeader){
                requestLength = headerLength + contentLength;
                if (inData.size() >= requestLength){
                    dataRecieved = true;
                    break;
                }
            }
        }
        QCoreApplication::processEvents();
        timeout++;
    }

    if (!dataRecieved){
        this->clientTCP->disconnectFromHost();
        this->clientTCP->deleteLater();
        return;
    }

    //qDebug() << "DEBUG STOP" << requestLength << timeout;
    //qDebug() << "inData size" << inData.size();
    ed = this->processClientRequest(&inData,headerLength);

    /*
    //read from client browser
    inStr = QString::fromLocal8Bit(this->clientTCP->readAll());

    list = inStr.split('\n');

    resultStr.clear();
    resultStr = "<br><font size=1>Infoblox TIG DGA Web System<br>\n";
    resultStr.append("Copyright (C) 2015 Azril arahimATinfobloxDOTcom</font><br><br><b><br>\n");

    for (quint16 i =0;i < list.size();i++){

        ops = list.at(i).split(QChar(32));

        if (ops.at(0).contains("GET")){
            ed = this->processGET(ops.at(1));
            getProcess = true;
            break;
        }

        if (ops.at(0).contains("POST")){
            QString url = this->getPOSTURL(list);
            qDebug() << "POST:" << url;
            //qDebug() << "ACTUAL RAW\n" << inStr << "\n\n";
            ed = this->processPOST(url);
            getProcess = true;
            break;
        }

    }

    if(!getProcess){
        //none get process by GET or POST
        //create error Message
        resultStr.append("Invalid Request. Unknown Request</b><br>\n");
    }

    //qDebug() << resultStr << "\n";

    */

    this->clientTCP->write(this->produceHttpReply(ed));
    this->clientTCP->flush();
    this->clientTCP->waitForBytesWritten();

    //close the socker
    this->clientTCP->disconnectFromHost();
    this->clientTCP->abort();

    return;
}

QString clientHandler::getPOSTURL(QStringList postDataList)
{
    //to get the target file and its query from post data method
    //and reformat it into GET url

    quint16 line;
    quint16 querySize;
    QStringList part;
    QString tgtFile;
    QString query;

    tgtFile = "";
    querySize = 0;
    query = "";

    for (line = 0; line < postDataList.size(); line++){

        part = postDataList.at(line).split(QChar(32));

        //get the file name
        if (part.at(0).compare("POST") == 0){
            tgtFile = part.at(1);
        }

        //get the POST query length
        if (part.at(0).compare("Content-Length:") == 0){
            querySize = part.at(1).toInt();
        }

        //stop looping when both values are accepted
        if ((!tgtFile.isEmpty()) && (querySize > 0)){
            break;
        }
    }

    //to be safe
    if (tgtFile.isEmpty())
        return "";


    //get the post query.. by moving from behind
    for (line = postDataList.size() -1; line >=0; line--){

        query = postDataList.at(line).trimmed() + query;
        if (query.size() >= querySize){
            break;
        }
    }

    //create the query similar to GET format
    if (query.isEmpty()){
        return tgtFile;
    }

    return tgtFile + "?" + query;
}

QString clientHandler::getRequestURLFile(QString rURL)
{
    qint16 queryLoc;

    queryLoc = 0;
    //backslashLoc = 0;

    //check if it has query indication
    queryLoc = rURL.indexOf("?");

    //return the entire url
    if (queryLoc <= 0)
        return rURL;

    //return the partial without the query
    return rURL.mid(0,queryLoc);

}

QString clientHandler::getRequestURLQuery(QString rURL)
{
    qint16 queryLoc = 0;

    queryLoc = rURL.indexOf("?");

    if (queryLoc <= 0)
        return "";

    queryLoc++;

    return rURL.mid(queryLoc,rURL.size() - queryLoc);
}

QString clientHandler::urlDecode(QString url)
{
    QString t;

    t = url;

    if (t.indexOf("'",0) < 0)
    {
        t.clear();
        t = "'" + url;
    }

    if (t.indexOf("'",t.size() - 1) <0)
    {
        t.append("'");
    }

    //replace & with "' '"
    t = t.replace("&","' '");

    //replace %2C with comma ,
    t = t.replace("%2C",",");

    //replace %2B with +
    t = t.replace("%2B","+");

    //replace %20 with " "
    t = t.replace("%20", " ");

    return t;
}

executeData clientHandler::processGET(QString getURL)
{
    executeData ed;
    QString file;
    QString query;

    //we need to decoded
    file = this->getRequestURLFile(getURL);
    query = this->urlDecode(this->getRequestURLQuery(getURL));

    qDebug() << "executing" << file << "with" << query;

    if (file.isEmpty()){
        ed.contentType = "text/html";
        ed.data = "Invalid Request. Bad Structure";
    }

    //process system data if its call
    if (file.compare("/sysstat") == 0){
        return this->processSystemStatus();
    }

    //execute the request
    return this->processFile(file,query);
}

executeData clientHandler::processPOST(QString postURL, QByteArray boundary, QByteArray *data){

    executeData ed;
    QString file;
    multipartArgv margv;
    QStringList tmpFiles;

    qint64 loc1, loc2, qSize;
    quint8 dd;
    QString argv;

    dd = 0;

    //we need to decoded
    //postURL = this->urlDecode(postURL);
    file = this->getRequestURLFile(postURL);
    //query = this->getRequestURLQuery(postURL);

    if (file.isEmpty()){
        ed.contentType = "text/html";
        ed.data = "Invalid POST Request. Bad Structure";
    }

    //if the post contains no boundary, then its not multiform
    //the post data will be always the entire content body
    if (boundary.isEmpty()){

        argv.clear();
        argv.append("'data=" + QString::fromLocal8Bit(data->replace("\r\n","").trimmed().toBase64()) + "' ");
    }

    else {

        qDebug() << "processin boundary:" << boundary;
        //we process every boundary
        while(1){
            loc1 = 0;
            loc2 = 0;

            margv.argv.clear();
            margv.tmpFile.clear();

            QCoreApplication::processEvents();

            //get the first boundary location;
            loc1 = data->indexOf(boundary,0);
            if (loc1 <0){
                break;
            }

            //boundary correction
            if (loc1 != 0){
                boundary = data->mid(0,loc1 + boundary.size());
                qDebug() << "new boundary" << boundary;
            }

            loc1 = boundary.size();
            loc2 = data->indexOf(boundary,loc1);
            if (loc2 < 0){
                break;
            }

            qSize = loc2 - loc1;

            margv = this->processPOSTMultiPart(data->mid(loc1,qSize));
            if (!margv.tmpFile.isEmpty()){
                tmpFiles.append(margv.tmpFile);
            }

            if (!margv.argv.isEmpty()){
                argv.append(margv.argv);
            }

            //remove the process part data;
            *data = data->remove(0,loc2);
        }
    }

    //execute the query;

    //format argv
    qDebug() << "execute query";
    qDebug().noquote() << file << argv;

    ed = this->processFile(file,argv);

    //after properly execute file, make sure we delete the tmp;
    for (int a=0; a < tmpFiles.size();a++){
        QFile f(tmpFiles.at(a));
        f.remove();
    }

    return ed;
}

QString clientHandler::getFileExtention(QString file)
{

    qint16 loc;

    //get file extention from file
    loc = file.lastIndexOf(".");
    if (loc <=0){
        return "";
    }
    loc++;
    return file.mid(loc,file.size() - loc);
}

FileExec clientHandler::getFileExecStatus(QString ext)
{
    FileExec fe;

    //if the extension is not in declared config list, the it is read only

    //default
    fe.contentType = "text/html";
    fe.ext = ext;
    fe.scriptParent = "";
    fe.execType = 1; //read only

    qDebug() << "EXT:" << ext;
    for (quint16 i = 0; i < this->fileExecList.size(); i++){
        if (this->fileExecList.at(i).ext.compare(ext) == 0){

            return this->fileExecList.at(i);
        }
    }
    return fe;
}

executeData clientHandler::processSystemStatus()
{
    //get system data from PROCVECT;

    executeData ep;
    QString ts;
    quint16 id;
    ep.contentType = "text/html";


    double successEfficient;
    double execEfficient;
    double efficientScore;
    quint16 median;

    qDebug() << "vector size:" << this->PROCVECT->size();

    ep.data.clear();
    ep.data.append("<html><br><b>System Process Status</b><br><br>\n");
    ep.data.append("<table border='1' style='width:800' >\n");
    ep.data.append("<tr><td align='center'><font size='2'><b>Process Name<b></td>\n");
    ep.data.append("<td align='center'><font size='2'><b>Invoked</b></td>\n");
    ep.data.append("<td align='center'><font size='2'><b>Failed</b></td>\n");
    ep.data.append("<td align='center'><font size='2'><b>Success Score</b></td>\n");
    ep.data.append("<td align='center'><font size='2'><b>Fastest</b></td>\n");
    ep.data.append("<td align='center'><font size='2'><b>Slowest</b></td>\n");
    ep.data.append("<td align='center'><font size='2'><b>Exec score </b></td>\n");
    ep.data.append("<td align='center'><font size='2'><b>Efficient %</b></td></tr>\n");

    for (id = 0; id < this->PROCVECT->size(); id++){

        successEfficient = ((this->PROCVECT->at(id).call - this->PROCVECT->at(id).fail)/this->PROCVECT->at(id).call) * 40;
        median = this->PROCVECT->at(id).slowestTIME - this->PROCVECT->at(id).fastestTIME;
        qDebug() << "median" << median;
        execEfficient = (100 - median);
        qDebug() << "execefficient" << execEfficient;

        execEfficient = (execEfficient / 100);
        qDebug() << "execefficient" << execEfficient;

        execEfficient = execEfficient * 60;
        qDebug() << "execefficient" << execEfficient;

        efficientScore = successEfficient + execEfficient;

        ts = "<tr><td align='center'><font size='1'>" + this->PROCVECT->at(id).processName + "</td>\n";
        ts.append("<td align='center'><font size='1'>" + QString::number(this->PROCVECT->at(id).call) + "</small></td>\n");
        ts.append("<td align='center'><font size='1'>" + QString::number(this->PROCVECT->at(id).fail) + "</td>\n");
        ts.append("<td align='center'><font size='1'>" + QString::number(successEfficient) + "</td>\n");
        ts.append("<td align='center'><font size='1'>" + QString::number(this->PROCVECT->at(id).fastestTIME) + "</td>\n");
        ts.append("<td align='center'><font size='1'>" + QString::number(this->PROCVECT->at(id).slowestTIME) + "</td>\n");
        ts.append("<td align='center'><font size='1'>" + QString::number(execEfficient) + "</td>\n");
        ts.append("<td align='center'><font size='1'>" + QString::number(efficientScore) + "</td></tr>\n");
        ep.data.append(ts);
    }

    if (id == 0){
        ep.data.append("<tr><td align='center'><font size='1'>No Process Found</font></td><td></td><td></td><td></td><td></td></tr>");
    }
    ep.data.append("</table></html>");

    return ep;
}

executeData clientHandler::processFile(QString file, QString argv)
{

    executeData ed;

    QString ext;
    FileExec fe;

    ed.contentType ="text/html";
    ed.data = "Error: Unknown Extension";

    //get file extension
    ext = this->getFileExtention(file);
    if (ext.isEmpty()){
        return ed;
    }

    //check file extention for execution;
    fe = this->getFileExecStatus(ext);
    ed.contentType = fe.contentType;

    //qDebug() << file << fe.execType;
    //qDebug() << "content type" << ed.contentType;

    //check file exists
    QFile fs(this->workDir + file);

    if (!fs.exists())
    {
        ed.contentType ="text/html";
        ed.data = "Error: Resource "; ed.data.append(file); ed.data.append(" does not exist");
        return ed;
    }

    switch (fe.execType)
    {
        case 1: ed.data = this->readFile(file);break; //readfile;
        case 2: ed.data = this->executeFile(file,argv);break;
    }

    //convert all bin regardless text or not into base64;
    return ed;
}

QByteArray clientHandler::produceHttpReply(executeData ed)
{
    QByteArray out;

    //if content type is ? then the application will produced their own header.
    out.clear();
    out.append("HTTP/1.0 200 OK\r\n");
    out.append("Server: Azril TODAK WEB SERVER v0.1\r\n");
    out.append("Connection: Closed\r\n");
    if (!ed.contentType.contains("?")){

        out.append("Content-Length:");
        out.append(QString::number(ed.data.size()));
        out.append("\r\n");
        out.append("Content-Type:" + ed.contentType + "\r\n");
        out.append("\r\n");
        out.append(ed.data);
    }
    else
    {
        out.append(ed.data);
    }

    //qDebug() << "\n====REPLY BEGIN=====\n" << out << "\n=====REPLY END======\n";

    return out;
}

void clientHandler::socketDisconnected()
{
    this->clientTCP->deleteLater();
}


QByteArray clientHandler::executeFile(QString file, QString argv)
{
    QProcess pro;
    QByteArray out;

    QFile fs(this->workDir + file);

    if (!fs.exists())
    {
        out = "Error: Resource "; out.append(file); out.append(" does not exist");
        return out;
    }

    pro.start(this->workDir + file + " " + argv);

    //start it
    if (!pro.waitForStarted()){
        out.append(pro.errorString());
        return out;
    }

    //wait it to finished
    if (!pro.waitForFinished()){
        out.append(pro.errorString());
        pro.close();
        return out;
    }

    //read result
    pro.waitForReadyRead();
    out = pro.readAllStandardOutput();
    pro.close();

    return out;
}

QByteArray clientHandler::readFile(QString file){

    QByteArray out;
    QFile fs(this->workDir + file);

    out.clear();

    if (!fs.exists())
    {
        out = "Error: Resource "; out.append(file); out.append(" does not exist");
        return out;
    }

    if (!fs.open(QIODevice::ReadOnly)){
        out = "Error: Resource "; out.append(file); out.append(" does not exist");
        return out;
    }

    //read it
    qDebug() << "readOnly" << fs.fileName();
    out = fs.readAll();
    fs.close();
    qDebug() << "Total read bytes are" << out.size();
    return out;
}

QByteArray clientHandler::utilExtractData(const QByteArray searchKey, const QByteArray endKey, quint64 start, const QByteArray *srcData){

    QByteArray result;
    qint64 loc, loc2;

    result.clear();
    loc = srcData->indexOf(searchKey,start);
    if (loc < 0){
        return result;
    }

    loc2 = srcData->indexOf(endKey,loc);
    if (loc2 < 0){
        loc2 = srcData->size();
    }

    result.append(srcData->mid(loc,loc2 - loc).remove(0,searchKey.size()).trimmed());
    return result;

}

QString clientHandler::getTmpRandomFileName()
{
    //get new primary is pseudo generate algo for creating DB primary
    //key based DDMMYYHHMMSS. This is to replace DB default auto increment
    //that will lead inconsistantcy during data update, append, migration
    //and rollback
    QDateTime now;
    QString utc;
    quint64 primaryKey = 0;

    utc = now.currentDateTimeUtc().toString();
    utc.remove(QRegExp("[^0-9]"));
    primaryKey = utc.toLongLong() + now.currentMSecsSinceEpoch();

    return QString::number(primaryKey);
}

