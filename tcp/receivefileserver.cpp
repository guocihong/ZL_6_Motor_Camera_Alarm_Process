#include "receivefileserver.h"
#include "receivefilethread.h"
#include <QDebug>

ReceiveFileServer::ReceiveFileServer(QObject *parent) : QTcpServer(parent)
{

}

ReceiveFileServer::~ReceiveFileServer()
{

}

bool ReceiveFileServer::startServer(quint16 port)
{
    return listen(QHostAddress::Any, port);
}

void ReceiveFileServer::stopServer()
{
    close();
}

void ReceiveFileServer::incomingConnection(int handle)
{
    ReceiveFileThread *receiveFile = new ReceiveFileThread(handle, this);
    connect(receiveFile, SIGNAL(message(QString)), this, SLOT(updateReceiveStatus(QString)));
    receiveFile->start();
}

void ReceiveFileServer::updateReceiveStatus(QString msg)
{
    qDebug() << "接收文件服务端:" << msg;
}
