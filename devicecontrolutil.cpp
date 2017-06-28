#include "devicecontrolutil.h"
#include <QDebug>

DeviceControlUtil *DeviceControlUtil::instance = NULL;
int DeviceControlUtil::fd = -1;

DeviceControlUtil::DeviceControlUtil(QObject *parent) :
    QObject(parent)
{
    DeviceControlUtil::fd = open(DEVICE_NAME, O_RDWR);
    if (DeviceControlUtil::fd == -1) {
        qDebug() << QString("open %1 failed").arg(DEVICE_NAME);
    } else {
        qDebug() << QString("open %1 succeed").arg(DEVICE_NAME);
    }

    //开启看门狗
    wdt_fd = open("/dev/watchdog",O_RDWR);
    if(wdt_fd == -1){
        qDebug() << QString("open /dev/watchdog failed");
    } else {
        qDebug() << QString("open /dev/watchdog succeed");
    }

    //设置看门狗超时时间
    ioctl(wdt_fd,WDIOC_SETTIMEOUT,10);

    FeedWatchDogTimer = new QTimer(this);
    FeedWatchDogTimer->setInterval(5 * 1000);
    connect(FeedWatchDogTimer,SIGNAL(timeout()),this,SLOT(slotFeedWatchDog()));
    FeedWatchDogTimer->start();
}


quint8 DeviceControlUtil::ReadIPAddr()
{
    quint8 ip_addr = 254;

    //读IP地址/RS485地址
    if (ioctl(DeviceControlUtil::fd, GET_IP_ADDR_SWITCH_STATE, &ip_addr) < 0) {
        qDebug() << "read ip addr failed";
    } else {
        qDebug() << QString("ip_addr = %1").arg(ip_addr);

        //使用8位拨码开关，地址范围为 0x00 ~ 0xFF,总线设备地址要求从0x10开始
        if ((ip_addr < 0x10) || (ip_addr == 0xFF) ||
            (ip_addr == 0xFE)) {
            //设备地址必须为 0x10 ~ 0xFD 之间(含),0xFF为广播地址,0xFE表示未定义地址
            ip_addr = 0xFE;
        }
    }

    return ip_addr;
}

bool DeviceControlUtil::ReadLeftSwitchState()
{
    quint8 left_switch_state;

    //读左开关量拨码开关
    if (ioctl(DeviceControlUtil::fd, GET_LEFT_SWITCH_STATE, &left_switch_state) < 0) {
        qDebug() << "read left switch state failed";
    } else {
        qDebug() << QString("left switch state =  %1").arg(left_switch_state);

        if (left_switch_state == 0) {//拨码开关打开
            return true;
        } else {
            return false;
        }
    }

    return false;
}

bool DeviceControlUtil::ReadRightSwitchState()
{
    quint8 right_switch_state;

    //读右开关量拨码开关
    if (ioctl(DeviceControlUtil::fd, GET_RIGHT_SWITCH_STATE, &right_switch_state) < 0) {
        qDebug() << "read right switch state failed";
    } else {
        qDebug() << QString("right switch state =  %1").arg(right_switch_state);

        if (right_switch_state == 0) {//拨码开关打开
            return true;
        } else {
            return false;
        }
    }

    return false;
}

bool DeviceControlUtil::ReadSelfSwitchState()
{
    quint8 self_switch_state;

    //读杆自身拨码开关
    if (ioctl(DeviceControlUtil::fd, GET_SELF_SWITCH_STATE, &self_switch_state) < 0) {
        qDebug() << "read self switch state failed";
    } else {
        qDebug() << QString("self switch state =  %1").arg(self_switch_state);

        if (self_switch_state == 0) {//拨码开关打开
            return true;
        } else {
            return false;
        }
    }

    return false;
}

quint8 DeviceControlUtil::ReadLeftAreaSwitchState()
{
    quint8 left_area_switch_state = 0;

    //读左防区拨码开关
    if (ioctl(DeviceControlUtil::fd, GET_LEFT_AREA_SWITCH_STATE, &left_area_switch_state) < 0) {
        qDebug() << "read left area switch state failed";
    } else {
        qDebug() << QString("left area switch state =  %1").arg(left_area_switch_state);
    }

    return left_area_switch_state;
}

quint8 DeviceControlUtil::ReadRightAreaSwitchState()
{
    quint8 right_area_switch_state = 0;

    //读右防区拨码开关
    if (ioctl(DeviceControlUtil::fd, GET_RIGHT_AREA_SWITCH_STATE, &right_area_switch_state) < 0) {
        qDebug() << "read right area switch state failed";
    } else {
        qDebug() << QString("right area switch state =  %1").arg(right_area_switch_state);
    }

    return right_area_switch_state;
}

void DeviceControlUtil::EnableMainStreamCamera()
{
    quint8 MainStreamCameraState = 1;
    ioctl(DeviceControlUtil::fd, SET_CAMERA2_POWER_STATE, &MainStreamCameraState);
}

void DeviceControlUtil::DisableMainStreamCamera()
{
    quint8 MainStreamCameraState = 0;
    ioctl(DeviceControlUtil::fd, SET_CAMERA2_POWER_STATE, &MainStreamCameraState);
}

void DeviceControlUtil::EnableSubStreamCamera()
{
    quint8 SubStreamCameraState = 1;
    ioctl(DeviceControlUtil::fd, SET_CAMERA1_POWER_STATE, &SubStreamCameraState);
}

void DeviceControlUtil::DisableSubStreamCamera()
{
    quint8 SubStreamCameraState = 0;
    ioctl(DeviceControlUtil::fd, SET_CAMERA1_POWER_STATE, &SubStreamCameraState);
}

void DeviceControlUtil::UpdateLedStatus(quint16 alarm_state)
{
    ioctl(DeviceControlUtil::fd, SET_ALARM_LED_STATE, &alarm_state);
}

quint8 DeviceControlUtil::ReadLeftSwitchAlarmState()
{
    quint8 left_switch_alarm_state = 1;
    ioctl(DeviceControlUtil::fd, GET_LEFT_SWITCH_ALARM_STATE , &left_switch_alarm_state);
    return left_switch_alarm_state;
}

quint8 DeviceControlUtil::ReadRightSwitchAlarmState()
{
    quint8 right_switch_alarm_state = 1;
    ioctl(DeviceControlUtil::fd, GET_RIGHT_SWITCH_ALARM_STATE , &right_switch_alarm_state);
    return right_switch_alarm_state;
}

quint8 DeviceControlUtil::ReadDoorkeepAlarmState()
{
    quint8 doorkeep_alarm_state = 0;
    ioctl(DeviceControlUtil::fd, GET_DOORKEEP_STATE , &doorkeep_alarm_state);
    return doorkeep_alarm_state;
}

void DeviceControlUtil::EnableBeep()
{
    quint8 beep_flag = 1;
    ioctl(DeviceControlUtil::fd, SET_BEEP_STATE , &beep_flag);
}

void DeviceControlUtil::DisableBeep()
{
    quint8 beep_flag = 0;
    ioctl(DeviceControlUtil::fd, SET_BEEP_STATE , &beep_flag);
}


void DeviceControlUtil::slotFeedWatchDog()
{
    ioctl(wdt_fd,WDIOC_KEEPALIVE);
}
