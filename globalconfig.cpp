#include "globalconfig.h"
#include "devicecontrolutil.h"
#include "tcp/tcphelper.h"
#include "CommonSetting.h"

GlobalConfig *GlobalConfig::instance = NULL;

QString GlobalConfig::ConfigFileName = "/bin/ZL_6_Motor_Camera_Alarm_Process_Config.ini";//配置文件的文件名

/*****************硬件配置参数**********************/
quint8 GlobalConfig::ip_addr = 254;
quint16 GlobalConfig::ad_sensor_mask_LR = 0;
quint16 GlobalConfig::ad_sensor_mask = 0;


/*****************本设备网络详情*********************/
QString GlobalConfig::LocalHostIP = CommonSetting::GetLocalHostIP();
QString GlobalConfig::Mask = CommonSetting::GetMask();
QString GlobalConfig::Gateway = CommonSetting::GetGateway();
QString GlobalConfig::MAC = CommonSetting::ReadMacAddress();


/*******************组播和tcp相关的端口***************************/
quint16 GlobalConfig::UdpGroupPort = 6905;
QString GlobalConfig::UdpGroupAddr = "224.0.0.17";
quint16 GlobalConfig::UpgradePort = 6904;
quint16 GlobalConfig::CheckNetworkPort = 6900;

/*****************报警主机下发的配置参数*********************/
QString GlobalConfig::ServerIP = QString("192.168.8.238");
quint16 GlobalConfig::ServerPort = 6901;
quint8 GlobalConfig::UseMainCamera = 1;
QString GlobalConfig::MainDefenceID = QString("010");
quint16 GlobalConfig::MainCameraSleepTime = 300;
quint8 GlobalConfig::UseSubCamera = 1;
QString GlobalConfig::SubDefenceID = QString("011");
quint16 GlobalConfig::SubCameraSleepTime = 300;
QString GlobalConfig::DeviceMacAddr = QString("00:60:6E:A4:BA:1C");
QString GlobalConfig::DeviceIPAddrPrefix = QString("192.168.8");
quint16 GlobalConfig::ad_still_dn = 0;
quint16 GlobalConfig::ad_still_up = 999;
quint8 GlobalConfig::ad_still_Dup[13] = {50,50,50,50,50,50,50,50,50,50,50,50,50};
quint16 GlobalConfig::beep_during_temp = 0;
quint8 GlobalConfig::gl_motor_channel_number = 5;
quint8 GlobalConfig::is_motor_add_link = 0;
quint8 GlobalConfig::gl_reply_tick = 0;
quint8 GlobalConfig::alarm_point_num = 4;


/**************************视频参数**************************/
quint16 GlobalConfig::SampleWidth = 640;
quint16 GlobalConfig::SampleHeight = 480;


/***********************用来控制程序的状态参数***************/
quint8 GlobalConfig::system_status = GlobalConfig::SYS_PowerON;
quint16 GlobalConfig::gl_delay_tick = DLY_BF_GetBase;
enum GlobalConfig::WorkMode GlobalConfig::system_mode = GlobalConfig::RS485Mode;
QDateTime GlobalConfig::RecvAlarmHostLastMsgTime = QDateTime::currentDateTime();
bool GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = false;


/********************用来与电机控制杆进行串口RS232通信的buffer******************/
QList<QByteArray> GlobalConfig::SendMsgToMotorControlBuffer = QList<QByteArray>();
QList<QByteArray> GlobalConfig::RecvVaildCompletePackageFromMotorControlBuffer = QList<QByteArray>();

/********************用来与报警主机进行RS485通信的buffer******************/
QList<QByteArray> GlobalConfig::SendMsgToAlarmHostBuffer = QList<QByteArray>();
QList<QByteArray> GlobalConfig::RecvVaildCompletePackageFromAlarmHostBuffer = QList<QByteArray>();


/******************保存电机张力控制杆的实时状态参数*********************/
quint16 GlobalConfig::ad_chn_sample[13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
sAD_BASE GlobalConfig::ad_chn_base[13] = {{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}};
quint16 GlobalConfig::ad_alarm_exts = 0;
quint16 GlobalConfig::ad_alarm_base = 0;
quint8  GlobalConfig::doorkeep_state = 0;
quint16 GlobalConfig::ad_chnn_wire_cut = 0;
quint8 GlobalConfig::gl_motor_adjust_flag = 0;
quint16 GlobalConfig::ad_alarm_tick[13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
quint8 GlobalConfig::adl_alarm_flag = 0;
quint8 GlobalConfig::adr_alarm_flag = 0;
quint8 GlobalConfig::zs_climb_alarm_flag = 0;
quint8 GlobalConfig::alarm_out_flag = 0;
quint8 GlobalConfig::gl_chnn_index = 0;
quint8  GlobalConfig::beep_flag = 0;
quint16 GlobalConfig::beep_timer = 0;
bool GlobalConfig::isEnterAutoAdjustMotorMode = false;
bool GlobalConfig::isEnterManualAdjustMotorMode = false;
quint8 GlobalConfig::adjust_status = 1;
quint8 GlobalConfig::is_sample_clear = 0;
quint8 GlobalConfig::check_sample_clear_tick = 0;
QString GlobalConfig::status = QString("");
bool GlobalConfig::MainStreamStateInfo = 0;
bool GlobalConfig::SubStreamStateInfo = 0;
sAlarmDetailInfo GlobalConfig::AlarmDetailInfo;

/************用来与报警主机进行tcp通信的buffer******************/
QList<TcpHelper *> GlobalConfig::TcpHelperBuffer = QList<TcpHelper *>();
QList<QByteArray> GlobalConfig::SendAlarmMsgToAlarmHostBuffer = QList<QByteArray>();
QList<QByteArray> GlobalConfig::SendOkMsgToAlarmHostBuffer = QList<QByteArray>();

GlobalConfig::GlobalConfig(QObject *parent) :
    QObject(parent)
{
    DeviceControlUtil::newInstance();
}

void GlobalConfig::init(void)
{
    //1、读取硬件配置
    //读IP地址/RS485地址
    GlobalConfig::ip_addr = DeviceControlUtil::ReadIPAddr();

    //使用8位拨码开关，地址范围为 0x00 ~ 0xFF,总线设备地址要求从0x10开始
    if ((GlobalConfig::ip_addr < 0x10) || (GlobalConfig::ip_addr == 0xFF) ||
        (GlobalConfig::ip_addr == 0xFE)) {
        //设备地址必须为 0x10 ~ 0xFD 之间(含),0xFF为广播地址,0xFE表示未定义地址
        GlobalConfig::ip_addr = CMD_ADDR_UNSOLV;
    }

    //读左开关量拨码开关
    if (DeviceControlUtil::ReadLeftSwitchState()) {//拨码开关打开
        GlobalConfig::ad_sensor_mask_LR |= (1 << 14);
    }

    //读右开关量拨码开关
    if (DeviceControlUtil::ReadRightSwitchState()) {//拨码开关打开
        GlobalConfig::ad_sensor_mask_LR |= (1 << 6);
    }

    //读杆自身拨码开关
    if (DeviceControlUtil::ReadSelfSwitchState()) {//拨码开关打开
        GlobalConfig::ad_sensor_mask_LR |= (1 << 7);
        GlobalConfig::ad_sensor_mask_LR |= (1 << 15);
    }

    //读左防区拨码开关
    quint8 left_area_switch_state = DeviceControlUtil::ReadLeftAreaSwitchState();
    GlobalConfig::ad_sensor_mask_LR |= ((left_area_switch_state & 0x3F) << 8);
    GlobalConfig::ad_sensor_mask |= (left_area_switch_state & 0x3F);

    //读右防区拨码开关
    quint8 right_area_switch_state = DeviceControlUtil::ReadRightAreaSwitchState();
    GlobalConfig::ad_sensor_mask_LR |= (right_area_switch_state & 0x3F);
    GlobalConfig::ad_sensor_mask |= ((right_area_switch_state & 0x3F) << 6);

    //2、读取报警主机下发的配置参数
    if (CommonSetting::FileExists(GlobalConfig::ConfigFileName)) {//配置文件存在
        //告警上报服务器IP
        GlobalConfig::ServerIP =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/ServerIP");

        //告警上报服务器端口 默认6901
        GlobalConfig::ServerPort =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/ServerPort").toUShort();

        //是否启用左防区摄像头（启用1 不启用0） 默认1启用
        GlobalConfig::UseMainCamera =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/UseMainCamera").toUShort();

        //左防区号
        GlobalConfig::MainDefenceID =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/MainDefenceID");

        //左防区摄像头采集间隔（单位毫秒） 默认300毫秒
        GlobalConfig::MainCameraSleepTime =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/MainCameraSleepTime").toUShort();

        //是否启用辅摄像头（启用1 不启用0） 默认1启用
        GlobalConfig::UseSubCamera =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/UseSubCamera").toUShort();
        //右防区号
        GlobalConfig::SubDefenceID =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/SubDefenceID");

        //右防区摄像头采集间隔（单位毫秒） 默认300毫秒
        GlobalConfig::SubCameraSleepTime =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/SubCameraSleepTime").toUShort();

        //设备MAC地址
        GlobalConfig::DeviceMacAddr =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/DeviceMacAddr");

        //设备网段
        GlobalConfig::DeviceIPAddrPrefix =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/DeviceIPAddrPrefix");

        //读静态张力值范围
        GlobalConfig::ad_still_dn =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/ad_still_dn").toUShort();

        GlobalConfig::ad_still_up =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/ad_still_up").toUShort();

        //是否有效？
        if ((GlobalConfig::ad_still_up > STD_STILL_UP) ||
                (GlobalConfig::ad_still_dn >= GlobalConfig::ad_still_up)) { //无合法数据，取缺省值
            GlobalConfig::ad_still_dn = STD_STILL_DN;
            GlobalConfig::ad_still_up = STD_STILL_UP;
        }

        //读报警阀值参数
        QStringList DupList =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/ad_still_Dup").split("|");

        for (int i = 0; i < DupList.size(); i++) {
            GlobalConfig::ad_still_Dup[i] = DupList.at(i).toUShort();
        }

        //读声光报警时间设置
        GlobalConfig::beep_during_temp =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/beep_during_temp")
                .toUShort() * 1000 / SCHEDULER_TICK;

        //读取电机张力通道数：4道电机、5道电机张力、6道电机张力
        GlobalConfig::gl_motor_channel_number =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/gl_motor_channel_number").toUShort();

        //读电机张力是否添加级联
        GlobalConfig::is_motor_add_link =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/is_motor_add_link").toUShort();

        //读采样值是否清零
        GlobalConfig::is_sample_clear =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/is_sample_clear").toUShort();

        //读取延时时间
        GlobalConfig::gl_reply_tick =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/gl_reply_tick").toUShort();

        //读取报警点数-->连续多少个点报警才判定为报警
        GlobalConfig::alarm_point_num =
                CommonSetting::ReadSettings(GlobalConfig::ConfigFileName,
                                            "AppGlobalConfig/alarm_point_num").toUShort();
    } else {//配置文件不存在,使用默认值生成配置文件
        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/ServerIP",GlobalConfig::ServerIP);

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/ServerPort",
                                     QString::number(GlobalConfig::ServerPort));

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/UseMainCamera",
                                     QString::number(GlobalConfig::UseMainCamera));

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/MainDefenceID",
                                     GlobalConfig::MainDefenceID);

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/MainCameraSleepTime",
                                     QString::number(GlobalConfig::MainCameraSleepTime));

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/UseSubCamera",
                                     QString::number(GlobalConfig::UseSubCamera));

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/SubDefenceID",
                                     GlobalConfig::SubDefenceID);

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/SubCameraSleepTime",
                                     QString::number(GlobalConfig::SubCameraSleepTime));

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/DeviceMacAddr",
                                     GlobalConfig::DeviceMacAddr);

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/DeviceIPAddrPrefix",
                                     GlobalConfig::DeviceIPAddrPrefix);


        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/ad_still_dn",
                                     QString::number(GlobalConfig::ad_still_dn));

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/ad_still_up",
                                     QString::number(GlobalConfig::ad_still_up));

        QStringList DupList = QStringList();
        for (int i = 0; i < 13; i++) {
            DupList << QString::number(GlobalConfig::ad_still_Dup[i]);
        }

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/ad_still_Dup",
                                     DupList.join("|"));

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/beep_during_temp",
                                     QString::number(GlobalConfig::beep_during_temp));

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/gl_motor_channel_number",
                                     QString::number(GlobalConfig::gl_motor_channel_number));

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/is_motor_add_link",
                                     QString::number(GlobalConfig::is_motor_add_link));

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/is_sample_clear",
                                     QString::number(GlobalConfig::is_sample_clear));

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/gl_reply_tick",
                                     QString::number(GlobalConfig::gl_reply_tick));

        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/alarm_point_num",
                                     QString::number(GlobalConfig::alarm_point_num));
    }

    //3、根据拨码开关设置IP地址
    GlobalConfig::UpdateIPAddr();
}

void GlobalConfig::UpdateIPAddr()
{
    QString NewIP = GlobalConfig::DeviceIPAddrPrefix + QString(".") +
            QString::number(GlobalConfig::ip_addr);

    if ((QString::compare(GlobalConfig::LocalHostIP,NewIP) == 0) &&
            (QString::compare(GlobalConfig::DeviceMacAddr,GlobalConfig::MAC) == 0)) {
        //do nothing
    } else {
        //根据拨码开关设置ip
        QStringList NetWorkConfigInfo;
        NetWorkConfigInfo << tr("IP=%1\n").arg(NewIP)
                             << tr("Mask=%1\n").arg("255.255.255.0")
                             << tr("Gateway=%1\n").arg(GlobalConfig::DeviceIPAddrPrefix + QString(".") +  QString("1"))
                             << tr("DNS=%1\n").arg("202.96.209.133")
                             << tr("MAC=%1\n").arg(GlobalConfig::DeviceMacAddr);
        QString ConfigureInfo = NetWorkConfigInfo.join("");
        QFile file("/etc/eth0-setting");
        if(file.open(QFile::WriteOnly | QFile::Truncate)){
            file.write(ConfigureInfo.toAscii());
            file.close();
        }

        //重启系统
        system("reboot");
    }
}
