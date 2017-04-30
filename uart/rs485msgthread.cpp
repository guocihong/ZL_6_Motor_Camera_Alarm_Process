#include "rs485msgthread.h"
#include "globalconfig.h"
#include "parsealarmhostuartmsg.h"
#include <QDateTime>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

RS485MsgThread *RS485MsgThread::instance = NULL;

RS485MsgThread::RS485MsgThread(QObject *parent) :
    QThread(parent)
{
    //初始化与报警主机进行通信的RS485-->使用RS485通信，半双工，收发需要错开
    AlarmHostUart = new QextSerialPort("/dev/ttySAC2");
    if (AlarmHostUart->open(QIODevice::ReadWrite)) {
        AlarmHostUart->setBaudRate(BAUD9600);
        AlarmHostUart->setDataBits(DATA_8);
        AlarmHostUart->setParity(PAR_NONE);
        AlarmHostUart->setStopBits(STOP_1);
        AlarmHostUart->setFlowControl(FLOW_OFF);
        AlarmHostUart->setTimeout(1);
        qDebug() << "/dev/ttySAC2 Open Succeed";
    } else {
        qDebug() << "/dev/ttySAC2 Open Failed";
    }

    SendMsgToAlarmHostTimer = new QTimer(this);
    connect(SendMsgToAlarmHostTimer,SIGNAL(timeout()),this,SLOT(slotSendMsgToAlarmHost()));

    RecvOriginalMsgFromAlarmHostBuffer.clear();
}

void RS485MsgThread::run()
{
    while(1) {
        this->RecvMsgFromAlarmHost();
        this->ParseMsgFromAlarmHost();
        ParseAlarmHostUartMsg::newInstance()->slotParseAlarmHostUartMsg();
        msleep(1);
    }
}

void RS485MsgThread::RecvMsgFromAlarmHost()
{
    if(AlarmHostUart->bytesAvailable() <= 0){
        return;
    }

    QByteArray cmd = AlarmHostUart->readAll();
    RecvOriginalMsgFromAlarmHostBuffer.append(cmd);
}

void RS485MsgThread::ParseMsgFromAlarmHost()
{
    if(RecvOriginalMsgFromAlarmHostBuffer.size() == 0) {
        return;
    }

    while(RecvOriginalMsgFromAlarmHostBuffer.size() > 0) {
        //寻找帧头0x16的索引
        int index = RecvOriginalMsgFromAlarmHostBuffer.indexOf(0x16);
        if (index < 0) {//没有找到帧头
            return;
        }

        //取出命令长度
        if (RecvOriginalMsgFromAlarmHostBuffer.size() < (index + 3)) {
            return;
        }
        int length = RecvOriginalMsgFromAlarmHostBuffer.at(index + 3);

        //取出一个完整数据包，包含帧头0x16和校验和
        if (RecvOriginalMsgFromAlarmHostBuffer.size() < (index + length + 5)) {//没有收到一个完整的数据包
            return;
        }
        QByteArray VaildCompletePackage = RecvOriginalMsgFromAlarmHostBuffer.mid(index,length + 5);
        RecvOriginalMsgFromAlarmHostBuffer = RecvOriginalMsgFromAlarmHostBuffer.mid(index + length + 5);

        //计算校验和
        quint8 chk_sum = 0;
        int size = VaildCompletePackage.size();
        for (int i = 1; i < (size - 1); i++) {
            chk_sum += VaildCompletePackage[i];
        }

        if (chk_sum == VaildCompletePackage[size - 1]) {//数据校验正确，收到一个正确的完整的数据包
            if ((VaildCompletePackage[1] == CMD_ADDR_BC) ||
                    (VaildCompletePackage[1] == GlobalConfig::ip_addr)) {//只保存广播数据包/专门发给本设备的数据包
                GlobalConfig::RecvVaildCompletePackageFromAlarmHostBuffer.append(VaildCompletePackage);

                //                qDebug() << "Recv:" << VaildCompletePackage;
            }
        } else {//数据校验错误，数据包传输过程中发生错误
            qDebug() << "data from alarm host parity error = " << VaildCompletePackage.toHex();
        }
    }
}

void RS485MsgThread::Start(void)
{
    qDebug() << "start = " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss zzz");

    //方法一：摄像头掉线和tcp传输都会影响时间的精准度
//    if (GlobalConfig::system_mode == GlobalConfig::RS485Mode) {
//        SendMsgToAlarmHostTimer->setInterval(REPLY_DLY + (GlobalConfig::ip_addr - 16) * GlobalConfig::gl_reply_tick);
//        SendMsgToAlarmHostTimer->start();
//    }

    //方法二
//    if (GlobalConfig::system_mode == GlobalConfig::RS485Mode) {
//        unsigned long mSec = REPLY_DLY + (GlobalConfig::ip_addr - 16) * GlobalConfig::gl_reply_tick;
//        struct timeval tv;
//        tv.tv_sec = mSec / 1000;
//        tv.tv_usec = (mSec % 1000) * 1000;
//        int err;

//        do {
//            err = select(0,NULL,NULL,NULL,&tv);
//        } while ((err < 0));

//        this->slotSendMsgToAlarmHost();
//    }

    //有一些广播命令，需要延时返回
    //有一些广播命令，根本就不返回
    //所有的单播命令都会返回，并且不会延时
    if (GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd) {
        unsigned long mSec = REPLY_DLY + (GlobalConfig::ip_addr - 16) * GlobalConfig::gl_reply_tick;
        struct timeval tv;
        tv.tv_sec = mSec / 1000;
        tv.tv_usec = (mSec % 1000) * 1000;
        int err;

        do {
            err = select(0,NULL,NULL,NULL,&tv);
        } while ((err < 0));
    }

    this->slotSendMsgToAlarmHost();
}

void RS485MsgThread::slotSendMsgToAlarmHost()
{
    qDebug() << "end = " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss zzz");

//    SendMsgToAlarmHostTimer->stop();

    if (GlobalConfig::SendMsgToAlarmHostBuffer.size() == 0) {
        return;
    }

    while(GlobalConfig::SendMsgToAlarmHostBuffer.size() > 0) {
        QByteArray cmd = GlobalConfig::SendMsgToAlarmHostBuffer.takeFirst();
        AlarmHostUart->write(cmd);
    }
}
