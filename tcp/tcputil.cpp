#include "tcputil.h"
#include "globalconfig.h"
#include <QDebug>
#include "CommonSetting.h"

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

    //主动发生ok数据包给报警主机用来测试网络的好坏
    SendOkMsgToAlarmHostSocket = new QTcpSocket(this);

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

    //主动发生ok数据包给报警主机用来测试网络的好坏
    CheckNetworkTimer = new QTimer(this);
    CheckNetworkTimer->setInterval(200);
    connect(CheckNetworkTimer,SIGNAL(timeout()),this,SLOT(slotCheckNetwork()));
    CheckNetworkTimer->start();
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

        //切换工作模式为tcp模式
//        GlobalConfig::system_mode = GlobalConfig::TcpMode;

        qDebug() << QString("AlarmHost connect:\n\tIP = ") +
                    RecvMsgFromAlarmHostSocket->peerAddress().toString() +
                    QString("\n\tPort = ") + QString::number(RecvMsgFromAlarmHostSocket->peerPort());
    }
}

//接收报警主机下发的所有网络配置信息和获取电机张力相关的信息
void TcpUtil::slotRecvMsgFromAlarmHost()
{
    GlobalConfig::RecvAlarmHostLastMsgTime = QDateTime::currentDateTime();

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

//报警主机断开连接,本设备是做为服务器,报警主机是做为客户端
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

//用来发送所有网络配置信息和电机张力的相关信息给报警主机
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

//用来主动上报报警信息给报警主机:tcp短连接
void TcpUtil::slotSendAlarmMsgToAlarmHost()
{
    while(GlobalConfig::SendAlarmMsgToAlarmHostBuffer.size() > 0) {
        QByteArray XmlData = GlobalConfig::SendAlarmMsgToAlarmHostBuffer.takeFirst();

        SendAlarmMsgToAlarmHostSocket->connectToHost(GlobalConfig::ServerIP, GlobalConfig::ServerPort);
        bool isConnect = SendAlarmMsgToAlarmHostSocket->waitForConnected(100);
        if (isConnect) {
            SendAlarmMsgToAlarmHostSocket->write(XmlData);
//            SendAlarmMsgToAlarmHostSocket->waitForBytesWritten(3000);//不能用这个函数，服务器会存在接收不全的情况
            CommonSetting::Sleep(500);
            SendAlarmMsgToAlarmHostSocket->disconnectFromHost();
            SendAlarmMsgToAlarmHostSocket->abort();
            qDebug() << "upload alarm info succeed";

            //默认情况下，报警会保持15s，如果网络是正常，主动上传报警信息后，报警会继续保持3s，然后报警恢复
            //如果网络不通，报警主机通过RS485读取本设备的报警详细信息后，报警会立即恢复
            for (int i = 0; i < 13; i++) {
                GlobalConfig::ad_alarm_tick[i] = 3000 / SCHEDULER_TICK;
            }
        } else {
            qDebug() << QString("upload alarm info failed for connect to alarm host failed:\n\tIP = ") +
                       GlobalConfig::ServerIP + QString("\n\tPort = ") +
                       QString::number(GlobalConfig::ServerPort);
        }
    }
}

//主动发生ok数据包给报警主机用来测试网络的好坏:tcp短连接
void TcpUtil::slotCheckNetwork()
{
    //网络是否正常
    bool isNetworkOnline = true;

    while(GlobalConfig::SendOkMsgToAlarmHostBuffer.size() > 0) {
        QByteArray XmlData = GlobalConfig::SendOkMsgToAlarmHostBuffer.takeFirst();

        SendOkMsgToAlarmHostSocket->connectToHost(GlobalConfig::ServerIP, GlobalConfig::CheckNetworkPort);
        bool isConnect = SendOkMsgToAlarmHostSocket->waitForConnected(100);
        if (isConnect) {
            SendOkMsgToAlarmHostSocket->write(XmlData);
//            SendAlarmMsgToAlarmHostSocket->waitForBytesWritten(3000);//不能用这个函数，服务器会存在接收不全的情况
            CommonSetting::Sleep(100);
            SendOkMsgToAlarmHostSocket->disconnectFromHost();
            SendOkMsgToAlarmHostSocket->abort();
            qDebug() << "upload ok info succeed";
        } else {
            //网络不通，切换工作模式为RS485模式
            GlobalConfig::RecvVaildCompletePackageFromAlarmHostBuffer.clear();
            GlobalConfig::SendMsgToAlarmHostBuffer.clear();
            GlobalConfig::system_mode = GlobalConfig::RS485Mode;

            isNetworkOnline = false;
            qDebug() << QString("upload ok info failed for connect to alarm host failed:\n\tIP = ") +
                       GlobalConfig::ServerIP + QString("\n\tPort = ") +
                       QString::number(GlobalConfig::CheckNetworkPort);

            break;
        }
    }

    //网络不通的情况下，重启网卡芯片,同时需要将之前的报警主机连接对象列表清空
    if (!isNetworkOnline) {
        //清空报警主机连接对象列表
        GlobalConfig::TcpHelperBuffer.clear();

        //重启网卡
        system("/etc/init.d/ifconfig-eth0");
    }
}

//解析报警主机发送过来的数据包
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
