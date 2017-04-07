#include "uartutil.h"
#include "globalconfig.h"

/*
 本类只进行串口数据的收发，不进行数据的解析
 */

UartUtil *UartUtil::instance = NULL;

UartUtil::UartUtil(QObject *parent) :
    QObject(parent)
{

}

void UartUtil::Listen(void)
{
    //1、初始化与电机控制杆进行通信的串口-->使用串口通信，全双工，收发可以同时
    MotorControlUart = new QextSerialPort("/dev/ttySAC1");
    if (MotorControlUart->open(QIODevice::ReadWrite)) {
        MotorControlUart->setBaudRate(BAUD9600);
        MotorControlUart->setDataBits(DATA_8);
        MotorControlUart->setParity(PAR_NONE);
        MotorControlUart->setStopBits(STOP_1);
        MotorControlUart->setFlowControl(FLOW_OFF);
        MotorControlUart->setTimeout(10);
        qDebug() << "/dev/ttySAC1 Open Succeed";
    } else {
        qDebug() << "/dev/ttySAC1 Open Failed";
    }

    SendMsgToMotorControlTimer = new QTimer(this);
    SendMsgToMotorControlTimer->setInterval(200);
    connect(SendMsgToMotorControlTimer,SIGNAL(timeout()),this,SLOT(slotSendMsgToMotorControl()));
    SendMsgToMotorControlTimer->start();

    RecvMsgFromMotorControlTimer = new QTimer(this);
    RecvMsgFromMotorControlTimer->setInterval(200);
    connect(RecvMsgFromMotorControlTimer,SIGNAL(timeout()),this,SLOT(slotRecvMsgFromMotorControl()));
    RecvMsgFromMotorControlTimer->start();

    ParseMsgFromMotorControlTimer = new QTimer(this);
    ParseMsgFromMotorControlTimer->setInterval(200);
    connect(ParseMsgFromMotorControlTimer,SIGNAL(timeout()),this,SLOT(slotParseMsgFromMotorControl()));
    ParseMsgFromMotorControlTimer->start();

    RecvOriginalMsgFromMotorControlBuffer.clear();


    //2、初始化与报警主机进行通信的串口-->使用RS485通信，半双工，收发需要错开
    AlarmHostUart = new QextSerialPort("/dev/ttySAC2");
    if (AlarmHostUart->open(QIODevice::ReadWrite)) {
        AlarmHostUart->setBaudRate(BAUD9600);
        AlarmHostUart->setDataBits(DATA_8);
        AlarmHostUart->setParity(PAR_NONE);
        AlarmHostUart->setStopBits(STOP_1);
        AlarmHostUart->setFlowControl(FLOW_OFF);
        AlarmHostUart->setTimeout(10);
        qDebug() << "/dev/ttySAC2 Open Succeed";
    } else {
        qDebug() << "/dev/ttySAC2 Open Failed";
    }

    RecvMsgFromAlarmHostTimer = new QTimer(this);
    RecvMsgFromAlarmHostTimer->setInterval(200);
    connect(RecvMsgFromAlarmHostTimer,SIGNAL(timeout()),this,SLOT(slotRecvMsgFromAlarmHost()));
    RecvMsgFromAlarmHostTimer->start();

    ParseMsgFromAlarmHostTimer = new QTimer(this);
    ParseMsgFromAlarmHostTimer->setInterval(200);
    connect(ParseMsgFromAlarmHostTimer,SIGNAL(timeout()),this,SLOT(slotParseMsgFromAlarmHost()));
    ParseMsgFromAlarmHostTimer->start();

    RecvOriginalMsgFromAlarmHostBuffer.clear();
}

/****************与电机控制杆相关的槽函数**********************/
void UartUtil::slotSendMsgToMotorControl()
{
    if (GlobalConfig::SendMsgToMotorControlBuffer.size() == 0) {
        return;
    }

    while(GlobalConfig::SendMsgToMotorControlBuffer.size() > 0) {
        QByteArray cmd = GlobalConfig::SendMsgToMotorControlBuffer.takeFirst();
        MotorControlUart->write(cmd);
    }
}

void UartUtil::slotRecvMsgFromMotorControl()
{
    if(MotorControlUart->bytesAvailable() <= 0){
        return;
    }

    QByteArray cmd = MotorControlUart->readAll();
    RecvOriginalMsgFromMotorControlBuffer.append(cmd);
}

void UartUtil::slotParseMsgFromMotorControl()
{
    if(RecvOriginalMsgFromMotorControlBuffer.size() == 0) {
        return;
    }

    while(RecvOriginalMsgFromMotorControlBuffer.size() > 0) {
        //寻找帧头0x16的索引
        int index = RecvOriginalMsgFromMotorControlBuffer.indexOf(0x16);
        if (index < 0) {//没有找到帧头
            return;
        }

        //取出命令长度
        if (RecvOriginalMsgFromMotorControlBuffer.size() < (index + 3)) {
            return;
        }
        int length = RecvOriginalMsgFromMotorControlBuffer.at(index + 3);

        //取出一个完整数据包,包含帧头0x16和校验和
        if (RecvOriginalMsgFromMotorControlBuffer.size() < (index + length + 5)) {//没有收到一个完整的数据包
            return;
        }
        QByteArray VaildCompletePackage = RecvOriginalMsgFromMotorControlBuffer.mid(index,length + 5);
        RecvOriginalMsgFromMotorControlBuffer = RecvOriginalMsgFromMotorControlBuffer.mid(index + length + 5);

        //计算校验和
        quint8 chk_sum = 0;
        int size = VaildCompletePackage.size();
        for (int i = 1; i < (size - 1); i++) {
            chk_sum += VaildCompletePackage[i];
        }

        if (chk_sum == VaildCompletePackage[size - 1]) {//数据校验正确，收到一个正确的完整的数据包
            GlobalConfig::RecvVaildCompletePackageFromMotorControlBuffer.append(VaildCompletePackage);
        } else {//数据校验错误，数据包传输过程中发生错误
            qDebug() << "data from motor control parity error";
        }
    }
}

/****************与报警主机相关的槽函数**********************/
void UartUtil::slotSendMsgToAlarmHost()
{
    if (GlobalConfig::SendMsgToAlarmHostBuffer.size() == 0) {
        return;
    }

    while(GlobalConfig::SendMsgToAlarmHostBuffer.size() > 0) {
        QByteArray cmd = GlobalConfig::SendMsgToAlarmHostBuffer.takeFirst();
        AlarmHostUart->write(cmd);
    }
}

void UartUtil::slotRecvMsgFromAlarmHost()
{
    if(AlarmHostUart->bytesAvailable() <= 0){
        return;
    }

    QByteArray cmd = AlarmHostUart->readAll();
    RecvOriginalMsgFromAlarmHostBuffer.append(cmd);
}

void UartUtil::slotParseMsgFromAlarmHost()
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
            if (((quint8)VaildCompletePackage[1] == 0xff) ||
                    ((quint8)VaildCompletePackage[1] == GlobalConfig::ip_addr)) {//只保存广播数据包/专门发给本设备的数据包
                GlobalConfig::RecvVaildCompletePackageFromAlarmHostBuffer.append(VaildCompletePackage);

//                qDebug() << "Recv:" << VaildCompletePackage;
            }
        } else {//数据校验错误，数据包传输过程中发生错误
            qDebug() << "data from alarm host parity error";
        }
    }

}

void UartUtil::Start(void)
{
    QTimer::singleShot(REPLY_DLY + (GlobalConfig::ip_addr - 16) * GlobalConfig::gl_reply_tick,this,
                       SLOT(slotSendMsgToAlarmHost()));
}
