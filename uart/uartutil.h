#ifndef UARTUTIL_H
#define UARTUTIL_H

#include <QObject>
#include <QMutex>
#include <QTimer>
#include "QextSerialPort/qextserialport.h"

class UartUtil : public QObject
{
    Q_OBJECT
public:
    explicit UartUtil(QObject *parent = 0);

    static UartUtil *newInstance() {
        static QMutex mutex;

        if (!instance) {
            QMutexLocker locker(&mutex);

            if (!instance) {
                instance = new UartUtil();
            }
        }

        return instance;
    }

    void Listen(void);//开启串口监听
    void Start(void);//启动延时返回数据包给报警主机

private slots:
    /****************与电机控制杆相关的槽函数**********************/
    void slotSendMsgToMotorControl();
    void slotRecvMsgFromMotorControl();
    void slotParseMsgFromMotorControl();


    /****************与报警主机相关的槽函数**********************/
    void slotSendMsgToAlarmHost();
    void slotRecvMsgFromAlarmHost();
    void slotParseMsgFromAlarmHost();
private:
    static UartUtil *instance;

    /****************与电机控制杆进行通信的串口参数**********************/
    //用来与电机控制杆进行通信的串口
    QextSerialPort *MotorControlUart;

    //用来发送数据包给电机控制杆
    QTimer *SendMsgToMotorControlTimer;

    //用来读电机控制杆发回来的数据包
    QTimer *RecvMsgFromMotorControlTimer;

    //从电机控制杆发回来的数据包中提取出一个个完整的数据包
    QTimer *ParseMsgFromMotorControlTimer;

    //电机控制杆发回来的原始数据全部都保存在这里
    QByteArray RecvOriginalMsgFromMotorControlBuffer;


    /****************与报警主机进行通信的串口参数**********************/
    //用来与报警主机进行通信的串口
    QextSerialPort *AlarmHostUart;

    //用来读报警主机发回来的数据包
    QTimer *RecvMsgFromAlarmHostTimer;

    //从报警主机发回来的数据包中提取出一个个完整的数据包
    QTimer *ParseMsgFromAlarmHostTimer;

    //报警主机发回来的原始数据全部都保存在这里
    QByteArray RecvOriginalMsgFromAlarmHostBuffer;
};

#endif // UARTUTIL_H
