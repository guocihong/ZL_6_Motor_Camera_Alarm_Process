#ifndef PARSEMOTORCONTROLUARTMSG_H
#define PARSEMOTORCONTROLUARTMSG_H

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <QStringList>
#include "globalconfig.h"

#define SAMPLE_POINT          10             //采样多少个点用来判断钢丝比较松
#define MIN_SAMPLE_VALUE      60             //瞬间张力采样值小于这个值，就认为钢丝比较松
#define TARGET_SAMPLE_VALUE   90             //收紧钢丝达到这个值，就认为调整结束，符合目标要求
#define MOTOR_RUN_TIME        5              //电机运转时间，单位为秒
#define WIRE_CUT_SAMPLE_VALUE 10             //判定钢丝被剪断时的瞬间张力采样值

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

    void motor_start(quint8 index);
    void motor_stop();

private slots:
    void slotParseMotorControltUartMsg(void);

private:
    static ParseMotorControlUartMsg *instance;

    QTimer *ParseMotorControlUartMsgTimer;

    //采样点数(计算静态基准值时总采样点数)
    quint16 ad_samp_pnum;

    //阶段求和
    //0-12分别代表：左1~6、右1~6、杆自身
    sAD_Sum ad_samp_sum[13];

    //各通道连续采样点(均衡后)的阀值判定： 0 - 范围内； 1 - 超阀值
    //0-12分别代表：左1~6、右1~6、杆自身
    quint8 ad_chn_over[13];

    //用于基准值跟踪的计量点数
    //0-12分别代表：左1~6、右1~6、杆自身
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

    //连续5次检测到钢丝的瞬间采样值小于WIRE_CUT_SAMPLE_VALUE，就认为钢丝被剪断
    quint8 ad_chnn_wire_cut_count[12];
};

#endif // PARSEMOTORCONTROLUARTMSG_H
