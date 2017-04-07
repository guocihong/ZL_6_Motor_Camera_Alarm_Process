#include "tcputil.h"
#include "globalconfig.h"
#include <QDebug>

TcpUtil *TcpUtil::instance = NULL;

TcpUtil::TcpUtil(QObject *parent) :
    QObject(parent)
{
}

void TcpUtil::Listen(void)
{
    //监听报警主机连接
    AlarmHostServerListener = new QTcpServer(this);
    connect(AlarmHostServerListener,SIGNAL(newConnection()),this,SLOT(slotProcessAlarmHostConnection()));
    AlarmHostServerListener->listen(QHostAddress::Any,6902);

    //主动上报报警信息给报警主机
    SendAlarmMsgToAlarmHostSocket = new QTcpSocket(this);

    //用来发送所有网络配置信息和电机张力的相关信息给报警主机
    SendMsgToAlarmHostTimer = new QTimer(this);
    SendMsgToAlarmHostTimer->setInterval(200);
    connect(SendMsgToAlarmHostTimer,SIGNAL(timeout()),this,SLOT(slotSendMsgToAlarmHost()));
    SendMsgToAlarmHostTimer->start();

    //用来主动上报报警信息给报警主机
    SendAlarmMsgToAlarmHostTimer = new QTimer(this);
    SendAlarmMsgToAlarmHostTimer->setInterval(200);
    connect(SendAlarmMsgToAlarmHostTimer,SIGNAL(timeout()),this,SLOT(slotSendAlarmMsgToAlarmHost()));
    SendAlarmMsgToAlarmHostTimer->start();

    //解析报警主机发送过来的数据包
    ParseMsgFromAlarmHostTimer = new QTimer(this);
    ParseMsgFromAlarmHostTimer->setInterval(200);
    connect(ParseMsgFromAlarmHostTimer,SIGNAL(timeout()),this,SLOT(slotParseMsgFromAlarmHost()));
    ParseMsgFromAlarmHostTimer->start();
}

//处理报警主机的tcp连接
void TcpUtil::slotProcessAlarmHostConnection()
{
    QTcpSocket *RecvMsgFromAlarmHostSocket = AlarmHostServerListener->nextPendingConnection();

    if (!RecvMsgFromAlarmHostSocket->peerAddress().toString().isEmpty()) {
        connect(RecvMsgFromAlarmHostSocket, SIGNAL(readyRead()), this, SLOT(slotRecvMsgFromAlarmHost()));
        connect(RecvMsgFromAlarmHostSocket, SIGNAL(disconnected()), this, SLOT(slotAlarmHostDisconnect()));

        TcpHelper *tcpHelper = new TcpHelper();
        tcpHelper->Socket = RecvMsgFromAlarmHostSocket;
        GlobalConfig::TcpHelperBuffer.append(tcpHelper);

        qDebug() << QString("AlarmHost connect:\n\tIP = ") +
                    RecvMsgFromAlarmHostSocket->peerAddress().toString() +
                    QString("\n\tPort = ") + QString::number(RecvMsgFromAlarmHostSocket->peerPort());
    }
}

//接收报警主机下发的所有网络配置信息和获取电机张力相关的信息
void TcpUtil::slotRecvMsgFromAlarmHost()
{
    QTcpSocket *RecvMsgFromAlarmHostSocket = (QTcpSocket *)sender();

    if (RecvMsgFromAlarmHostSocket->bytesAvailable() <= 0) {
        return;
    }


    foreach (TcpHelper *tcpHelper, GlobalConfig::TcpHelperBuffer) {
       if (tcpHelper->Socket == RecvMsgFromAlarmHostSocket) {
            tcpHelper->RecvOriginalMsgBuffer.append(RecvMsgFromAlarmHostSocket->readAll());
       }
    }
}

void TcpUtil::slotAlarmHostDisconnect()
{
    QTcpSocket *RecvMsgFromAlarmHostSocket = (QTcpSocket *)sender();

    foreach (TcpHelper *tcpHelper, GlobalConfig::TcpHelperBuffer) {
       if (tcpHelper->Socket == RecvMsgFromAlarmHostSocket) {
            GlobalConfig::TcpHelperBuffer.removeAll(tcpHelper);
            delete tcpHelper;
       }
    }

    qDebug() << QString("AlarmHost disconnect:\n\tIP = ") +
                RecvMsgFromAlarmHostSocket->peerAddress().toString() +
                QString("\n\tPort = ") + QString::number(RecvMsgFromAlarmHostSocket->peerPort());
}

void TcpUtil::slotSendMsgToAlarmHost()
{
    foreach (TcpHelper *tcpHelper, GlobalConfig::TcpHelperBuffer) {
       //没有数据包等待发送
       if (tcpHelper->SendMsgBuffer.size() == 0) {
           continue;
       }

       //把所有等待发送的数据包全部发送完成
       while(tcpHelper->SendMsgBuffer.size() > 0) {
           QByteArray data = tcpHelper->SendMsgBuffer.takeFirst();
           tcpHelper->Socket->write(data);
       }
    }
}

void TcpUtil::slotSendAlarmMsgToAlarmHost()
{

}

void TcpUtil::slotParseMsgFromAlarmHost()
{
    foreach (TcpHelper *tcpHelper, GlobalConfig::TcpHelperBuffer) {
        //没有数据包等待解析
        if (tcpHelper->RecvOriginalMsgBuffer.size() == 0) {
            continue;
        }

        while(tcpHelper->RecvOriginalMsgBuffer.size() > 0) {
            //寻找帧头的索引
            int FrameHeadIndex = tcpHelper->RecvOriginalMsgBuffer.indexOf("IALARM");
            if (FrameHeadIndex < 0) {
                break;
            }


            if (tcpHelper->RecvOriginalMsgBuffer.size() < (FrameHeadIndex + 20)) {
                break;
            }

            //取出xml数据包的长度，不包括帧头的20个字节
            int length = tcpHelper->RecvOriginalMsgBuffer.mid(FrameHeadIndex + 7,13).toUInt();

            //没有收到一个完整的数据包
            if (tcpHelper->RecvOriginalMsgBuffer.size() < (FrameHeadIndex + 20 + length)) {
                break;
            }

            //取出一个完整的xml数据包,不包括帧头20个字节
            QByteArray VaildCompletePackage =
                    tcpHelper->RecvOriginalMsgBuffer.mid(FrameHeadIndex + 20,length);

            tcpHelper->RecvOriginalMsgBuffer =
                    tcpHelper->RecvOriginalMsgBuffer.mid(FrameHeadIndex + 20 + length);

            tcpHelper->RecvVaildCompleteMsgBuffer.append(VaildCompletePackage);
        }
    }
}
