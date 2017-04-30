#ifndef DEVICECONTROLUTIL_H
#define DEVICECONTROLUTIL_H

#include <QObject>
#include <QMutex>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define DEVICE_NAME                        "/dev/s5pv210_motor_camera"

//驱动命令定义
//定义ioctl命令幻数
#define MOTOR_CAMERA_IOC_MAGIC             'G'

//门磁
#define GET_DOORKEEP_STATE                 _IOR(MOTOR_CAMERA_IOC_MAGIC,1,int)

//控制杆攀爬报警灯
#define SET_SELF_CLIMB_ALARM_LED_STATE     _IOW(MOTOR_CAMERA_IOC_MAGIC,2,int)

//左开关量拨码开关
#define GET_LEFT_SWITCH_STATE              _IOR(MOTOR_CAMERA_IOC_MAGIC,3,int)

//右开关量拨码开关
#define GET_RIGHT_SWITCH_STATE             _IOR(MOTOR_CAMERA_IOC_MAGIC,4,int)

//杆自身拨码开关
#define GET_SELF_SWITCH_STATE              _IOR(MOTOR_CAMERA_IOC_MAGIC,5,int)

//IP地址拨码开关
#define GET_IP_ADDR_SWITCH_STATE           _IOR(MOTOR_CAMERA_IOC_MAGIC,6,int)

//左开关量输入
#define GET_LEFT_SWITCH_ALARM_STATE        _IOR(MOTOR_CAMERA_IOC_MAGIC,7,int)

//右开关量输入
#define GET_RIGHT_SWITCH_ALARM_STATE       _IOR(MOTOR_CAMERA_IOC_MAGIC,8,int)

//蜂鸣器
#define SET_BUZZER_STATE                   _IOW(MOTOR_CAMERA_IOC_MAGIC,9,int)

//USB HUB复位
#define SET_HUB_STATE                      _IOW(MOTOR_CAMERA_IOC_MAGIC,10,int)

//摄像头电源使能1
#define SET_CAMERA1_POWER_STATE            _IOW(MOTOR_CAMERA_IOC_MAGIC,11,int)

//摄像头电源使能2
#define SET_CAMERA2_POWER_STATE            _IOW(MOTOR_CAMERA_IOC_MAGIC,12,int)

//警号使能
#define SET_BEEP_STATE                     _IOW(MOTOR_CAMERA_IOC_MAGIC,13,int)

//左防区拨码开关
#define GET_LEFT_AREA_SWITCH_STATE         _IOR(MOTOR_CAMERA_IOC_MAGIC,14,int)

//右防区拨码开关
#define GET_RIGHT_AREA_SWITCH_STATE        _IOR(MOTOR_CAMERA_IOC_MAGIC,15,int)

//报警灯设置
#define SET_ALARM_LED_STATE                _IOR(MOTOR_CAMERA_IOC_MAGIC,16,int)

class DeviceControlUtil : public QObject
{
    Q_OBJECT
public:
    explicit DeviceControlUtil(QObject *parent = 0);

    static DeviceControlUtil *newInstance() {
        static QMutex mutex;

        if (!instance) {
            QMutexLocker locker(&mutex);

            if (!instance) {
                instance = new DeviceControlUtil();
            }
        }

        return instance;
    }

public:
    static quint8 ReadIPAddr();                       //读IP地址/RS485地址

    static bool ReadLeftSwitchState();                //读左开关量拨码开关的状态：0-打开；1-关闭
    static bool ReadRightSwitchState();               //读右开关量拨码开关的状态：0-打开；1-关闭
    static bool ReadSelfSwitchState();                //读杆自身拨码开关的状态：0-打开；1-关闭

    static quint8 ReadLeftAreaSwitchState();          //读左防区拨码开关的状态
    static quint8 ReadRightAreaSwitchState();         //读右防区拨码开关的状态

    static void EnableMainStreamCamera();             //打开左防区摄像头电源
    static void DisableMainStreamCamera();            //关闭左防区摄像头电源

    static void EnableSubStreamCamera();              //打开右防区摄像头电源
    static void DisableSubStreamCamera();             //关闭右防区摄像头电源

    static void UpdateLedStatus(quint16 alarm_state); //设置报警灯的状态

    static quint8 ReadLeftSwitchAlarmState();         //读左开关量报警状态：0-报警；1-不报警
    static quint8 ReadRightSwitchAlarmState();        //读右开关量报警状态：0-报警；1-不报警

    static quint8 ReadDoorkeepAlarmState();           //读门磁报警状态：0-报警；1-不报警

    static void EnableBeep();                         //警号鸣叫
    static void DisableBeep();                        //警号静鸣

private :
    static DeviceControlUtil *instance;

    //设备文件句柄
    static int fd;
};

#endif // DEVICECONTROLUTIL_H
