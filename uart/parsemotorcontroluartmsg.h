#ifndef PARSEMOTORCONTROLUARTMSG_H
#define PARSEMOTORCONTROLUARTMSG_H

#include <QObject>
#include <QMutex>
#include <QTimer>
#include "globalconfig.h"

class ParseMotorControlUartMsg : public QObject
{
    Q_OBJECT
public:
    explicit ParseMotorControlUartMsg(QObject *parent = 0);

    static ParseMotorControlUartMsg *newInstance() {
        static QMutex mutex;

        if (!instance) {
            QMutexLocker locker(&mutex);

            if (!instance) {
                instance = new ParseMotorControlUartMsg();
            }
        }

        return instance;
    }

    void Parse(void);//实时解析电机控制杆串口232发生过来的数据包


    void check_still_stress(quint8 index);
    void update_led_status(void);
    void update_alarm_status(void);

private slots:
    void slotParseMotorControltUartMsg(void);

private:
    static ParseMotorControlUartMsg *instance;

    QTimer *ParseMotorControlUartMsgTimer;

    //采样点数(计算静态基准值时总采样点数)
    quint16 ad_samp_pnum;

    //阶段求和
    sAD_Sum ad_samp_sum[13];

    //各通道连续采样点(均衡后)的阀值判定： 0 - 范围内； 1 - 超阀值
    quint8 ad_chn_over[13];

    //用于基准值跟踪的计量点数
    quint8 md_point[13];

    //电机堵转次数
    quint8 gl_motor_overcur_point[12];

    //是否调整完成：0-没有调整完成;1-表示调整完成
    quint8 gl_motor_adjust_end[12];

    //用来分析钢丝的松紧程度，从0-11分别代表：左1~左6,右1~右6
    quint16 ad_chnn_state[12];

    //电机时间是否用完：0-没有;1-时间用完
    quint8 is_timeout;

    //电机是否处于堵转状态：0-正常工作;1-电机堵转
    quint8 gl_motor_overcur_flag;

    //电机控制杆门磁的状态
    quint8 gl_control_dk_status;
};

#endif // PARSEMOTORCONTROLUARTMSG_H
