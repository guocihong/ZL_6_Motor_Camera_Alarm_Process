#include <QApplication>
#include "globalconfig.h"
#include "udp/udputil.h"
#include "uart/uartutil.h"
#include "uart/parsemotorcontroluartmsg.h"
#include "uart/rs485msgthread.h"
#include "tcp/tcputil.h"
#include "tcp/parsealarmhosttcpmsg.h"
#include "tcp/uploadalarmmsgtoalarmhostusingtcp.h"
#include "tcp/checknetwork.h"
#include "tcp/receivefileserver.h"
#include "camera/mainstream.h"
#include "camera/substream.h"
#include "camera/getcamerainfo.h"
#include "scheduler.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //1、全局初始化，必须首先执行
    GlobalConfig::newInstance()->init();

    //2、开启组播监听-->处理来自报警主机的报文
    UdpUtil::newInstance()->Listen();

    //3、程序升级监听
    ReceiveFileServer receive;
    receive.startServer(GlobalConfig::UpgradePort);

    //4.左防区摄像头实时采集
    MainStream::newInstance()->Start();

    //5、右防区摄像头实时采集
    SubStream::newInstance()->Start();

    //6、开启tcp监听-->实时接收和发生数据包给报警主机
    TcpUtil::newInstance()->Listen();

    //7、开启tcp数据解析-->实时解析报警主机tcp发送过来的数据包
    ParseAlarmHostTcpMsg::newInstance()->Parse();

    //8、开启报警监听-->实时分析电机控制杆是否发生报警，如果发生报警，通过tcp主动上传报警详细信息
    UploadAlarmMsgToAlarmHostUsingTcp::newInstance()->Listen();

    //9、开启网络监听-->主动通过tcp发送ok数据包给报警主机，如果网络不通，重启网卡，并切换成RS485模式
    CheckNetWork::newInstance()->Listen();

    //10、开启串口监听-->实时接收和发送数据包给电机控制杆
    UartUtil::newInstance()->Listen();

    //11、开启RS485线程-->实时接收和发生数据包给报警主机
    RS485MsgThread::newInstance()->start();

    //12、开启串口232数据解析-->实时解析电机控制杆串口232发生过来的数据包
    ParseMotorControlUartMsg::newInstance()->Parse();

    //13、开启任务调度
    Scheduler::newInstance()->Start();

    return a.exec();
}
