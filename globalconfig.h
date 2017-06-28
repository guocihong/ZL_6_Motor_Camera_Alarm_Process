#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#include <QObject>
#include <QMutex>
#include <QDebug>
#include <QDateTime>
#include "tcp/tcphelper.h"
#include "CommonSetting.h"

//ZZX:  张力相关缺省参数 (单位：采样值)
#define STD_STILL_DN          0                           //张力静态值下限
#define STD_STILL_UP          999                         //张力静态值上限

#define DEF_ModiBASE_TH  5	                              //报警状态下(连续四点超限), 允许更新基准值需要的最小变化阀值, 单位: 采样值
#define DEF_ModiBASE_PT  6                                //缓慢拉紧时, 多少个计量点(一个计量点含多个均衡点)被连续拉紧, 用于跟踪基准值使递增1
                                                          //  例如: 为6时, 更新周期为 6 * 0.64 = 3.9 秒	  (一个计量点含2个均衡点时)

#define SCHEDULER_TICK        1000
#define DLY_BF_GetBase        (5000 / SCHEDULER_TICK)     //基准值采样前延时，单位：tick
#define ALARM_TEMPO           (3000 / SCHEDULER_TICK)     //报警信号持续时间
#define CHECK_WIRE_CUT_TICK   (5000 / SCHEDULER_TICK)     //判断钢丝是否被真正剪断计时tick

#define CMD_ADDR_BC      255                              //广播地址
#define CMD_ADDR_UNSOLV  254                              //未烧录或未正确设置的地址

#define LOW(U16)              ((quint8)U16)
#define HIGH(U16)             ((quint8)(U16>>8))

#define REPLY_DLY             40

/* AD */
typedef struct strAD_Sum
{ //采样值累加和
  quint16   sum;                                         //累计和 (最多达64点,不会溢出)
  quint8    point;                                       //已采样点数
}sAD_Sum;

typedef struct strAD_BASE
{ //系统运行时静态基准值对应的采样值
  quint16   base;                                       //静态基准值
  quint16   base_down;                                  //基准值下限(含)
  quint16   base_up;                                    //基准值上限(含)
}sAD_BASE;

typedef struct strAlarmDetailInfo
{
    quint16  InstantSampleValue[13];                   //瞬间张力:左1~6、右1~6、杆自身
    quint16  StaticBaseValue[13];                      //静态基准:左1~6、右1~6、杆自身
    quint16  ExternalAlarm;                            //外力报警:杆自身、左开关量、左6~左1,杆自身、右开关量、右6~右1
    quint16  StaticAlarm;	                           //静态报警:杆自身、左开关量、左6~左1,杆自身、右开关量、右6~右1
    quint8   DoorKeepAlarm;                            //门磁报警:!控制杆门磁 | 防水箱门磁
}sAlarmDetailInfo;

class GlobalConfig : public QObject
{
    Q_OBJECT
public:
    explicit GlobalConfig(QObject *parent = 0);

    static GlobalConfig *newInstance() {
        static QMutex mutex;

        if (!instance) {
            QMutexLocker locker(&mutex);

            if (!instance) {
                instance = new GlobalConfig();
            }
        }

        return instance;
    }

    enum SystemStatus {
        SYS_PowerON = 0,         // 0 - 初始上电
        SYS_B5S = 1,             // 1 - 基准值采样前延时(约5秒)
        SYS_SAMP_BASE = 2,       // 2 - 基准值采样(10秒左右)
        SYS_SELF_CHECK1 = 3,     // 3 - 开机自检阶段1
        SYS_SELF_CHECK2 = 4,     // 4 - 开机自检阶段2
        SYS_CHECK = 5,           // 5 - 实时监测
    };

    enum WorkMode {
        RS485Mode = 0,//网络不通的情况下，自动切换工作模式为RS485模式
        TcpMode = 1//网络正常的情况下，自动切换工作模式为tcp模式
    };

    static void init(void);
    static void UpdateIPAddr(void);

private :
    static GlobalConfig *instance;

public:
    //配置文件的文件名
    static QString ConfigFileName;


    /*****************硬件配置参数**********************/
    //ip地址/rs485地址
    static quint8 ip_addr;

    //bit15~bit0分别代表: 杆、左开关量、左6 ~ 1,杆、右开关量、右6 ~ 1
    static quint16 ad_sensor_mask_LR;

    //bit12~bit0分别代表：杆自身，右6~右1,左6~左1
    static quint16 ad_sensor_mask;



    /*****************本设备网络详情*********************/
    //本机IP地址
    static QString LocalHostIP;

    //本机子网掩码
    static QString Mask;

    //本机网关
    static QString Gateway;

    //本机MAC地址
    static QString MAC;


    /*******************组播和tcp相关的端口***************************/
    //udp组播端口
    static quint16 UdpGroupPort;

    //udp组播地址
    static QString UdpGroupAddr;

    //升级端口
    static quint16 UpgradePort;

    //检测网络好坏端口
    static quint16 CheckNetworkPort;



    /*****************报警主机下发的配置参数*********************/
    //告警上报服务器IP
    static QString ServerIP;

    //告警上报服务器端口 默认6901
    static quint16 ServerPort;

    //是否启用左防区摄像头（启用1 不启用0） 默认1启用
    static quint8 UseMainCamera;

    //左防区号
    static QString MainDefenceID;

    //左防区摄像头采集间隔（单位毫秒） 默认300毫秒
    static quint16 MainCameraSleepTime;

    //是否启用辅摄像头（启用1 不启用0） 默认1启用
    static quint8 UseSubCamera;

    //右防区号
    static QString SubDefenceID;

    //右防区摄像头采集间隔（单位毫秒） 默认300毫秒
    static quint16 SubCameraSleepTime;

    //设备MAC地址
    static QString DeviceMacAddr;

    //设备网段
    static QString DeviceIPAddrPrefix;

    //静态拉力值下限
    static quint16 ad_still_dn;

    //静态拉力值上限
    static quint16 ad_still_up;

    //报警阀值 - 0-12分别代表：左1~6、右1~6、杆自身
    static quint8 ad_still_Dup[13];

    //声光报警时间
    static quint16 beep_during_temp;

    //电机张力通道数:4道电机、5道电机、6道电机
    static quint8 gl_motor_channel_number;

    //是否添加级联
    static quint8 is_motor_add_link;

    //设备返回延时时间
    static quint8 gl_reply_tick;

    //设置报警点数
    static quint8 alarm_point_num;

    //上传报警图片的张数
    static quint8 AlarmImageCount;



    /**************************视频参数**************************/
    static quint16 SampleWidth;
    static quint16 SampleHeight;


    /***********************用来控制程序的状态参数***************/
    //系统状态
    static quint8 system_status;

    //系统计时
    static quint16 gl_delay_tick;

    //工作模式：RS485模式，网络模式
    static enum WorkMode system_mode;

    //接收到报警主机最后一次消息的时间
    static QDateTime RecvAlarmHostLastMsgTime;

    //报警主机通过RS485发送的广播命令是否需要延时返回，延时时间为根据地址来计算
    //只有读取延时，设置平均值点数和设置报警点数这三条广播命令会延时返回，报警主机其他RS485广播命令一律不会返回
    static bool isDelayResponseAlarmHostRS485BroadcastCmd;

    //报警主机通过RS485发送的单播命令是否需要延时返回，延时时间为100ms
    //读取详细信息的7条命令需要延时返回，其他一律不需要延时返回
    static bool isDelayResponseAlarmHostRS485UnicastCmd;

    //版本号
    static QString Version;



    /********************用来与电机控制杆进行串口RS232通信的buffer******************/
    //发送给电机控制杆的数据包全部保存在这里
    static QList<QByteArray> SendMsgToMotorControlBuffer;

    //电机控制杆发回来的有效的完整数据包全部都保存在这里
    static QList<QByteArray> RecvVaildCompletePackageFromMotorControlBuffer;



    /********************用来与报警主机进行RS485通信的buffer******************/
    //发送给报警主机的数据包全部保存在这里
    static QList<QByteArray> SendMsgToAlarmHostBuffer;

    //报警主机发回来的有效的完整数据包全部都保存在这里
    static QList<QByteArray> RecvVaildCompletePackageFromAlarmHostBuffer;



    /******************保存电机张力控制杆的实时状态参数*********************/
    //瞬间张力:0-12分别代表：左1~6、右1~6、杆自身
    static quint16 ad_chn_sample[13];

    //静态基准:0-12分别代表：左1~6、右1~6、杆自身
    static sAD_BASE ad_chn_base[13];

    //外力报警标志:bit15~bit0分别代表：杆自身、左开关量、左6~1   杆自身、右开关量、右6~1
    static quint16 ad_alarm_exts;

    //静态张力报警标志:bit15~bit0分别代表：杆自身、左开关量、左6~1   杆自身、右开关量、右6~1
    static quint16 ad_alarm_base;

    //保存本地门磁和电机控制杆门磁的状态
    static quint8 doorkeep_state;

    //0-表示钢丝没有被剪断;1-表示钢丝被剪断 -->X X 左6~左1、X X 右6~右1
    static quint16 ad_chnn_wire_cut;

    //电机是否处于工作状态：0-停止工作状态;1-正处于工作状态
    static quint8 gl_motor_adjust_flag;

    //各通道报警计时tick
    static quint16 ad_alarm_tick[13];

    //左侧张力组合报警标志: 0 - 无报警; 1 - 超阀值，报警
    static quint8 adl_alarm_flag;

    //右侧张力组合报警标志: 0 - 无报警; 1 - 超阀值，报警
    static quint8 adr_alarm_flag;

    //杆自身攀爬报警标志：0-无报警；1-报警
    static quint8 zs_climb_alarm_flag;

    //bit7-bit0分别代表:X X 杆自身攀爬报警 右防区报警 左防区报警 X X X
    static quint8 alarm_out_flag;

    //当前正在调整的钢丝的索引:0-11分别代表：左1 右1 左2 右2 左3 右3 左4 右4 左5 右5 左6 右6
    static quint8 gl_chnn_index;

    //蜂鸣标志 : 0 - 禁鸣; 1 - 正在蜂鸣
    static quint8  beep_flag;

    //剩余蜂鸣时间, 单位tick
    static quint16 beep_timer;

    //是否进入自动调整钢丝模式，优先级比手动调整模式低
    static bool isEnterAutoAdjustMotorMode;

    //是否进入手动调整钢丝模式，优先级比自动调整模式高
    static bool isEnterManualAdjustMotorMode;

    //当前调整钢丝的状态
    static quint8 adjust_status;

    //采样值是否清零,这个变量只有在加级联的情况下才有用
    static quint8 is_sample_clear;

    //加级联的情况下，开机自检阶段，采样值清零成功后，需要延时一段时间，主要是同步静态基准和瞬间张力
    static quint8 check_sample_clear_tick;

    //保存电机张力控制杆的实时状态详细信息
    static QString status;

    //左防区摄像头状态信息：0-离线；1-在线
    static bool MainStreamStateInfo;

    //右防区摄像头状态信息:0-离线；1-在线
    static bool SubStreamStateInfo;

    //保存最后一次报警详细信息
    static sAlarmDetailInfo AlarmDetailInfo;

    //检测钢丝是否被真正剪断计时tick
    static quint16 check_wire_cut_tick;

    //是否检测钢丝是否被剪断
    static bool isCheckWireCut;



    /************用来与报警主机进行tcp通信的buffer******************/
    //保存报警主机的所有连接对象和数据包
    static QList<TcpHelper *> TcpHelperBuffer;

    //发送给报警主机的报警信息
    static QList<QByteArray> SendAlarmMsgToAlarmHostBuffer;

    //主动发生ok数据包给报警主机用来测试网络的好坏
    static QList<QByteArray> SendOkMsgToAlarmHostBuffer;
};

#endif // GLOBALCONFIG_H
