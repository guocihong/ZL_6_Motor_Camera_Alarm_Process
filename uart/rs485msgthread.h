#ifndef RS485MSGTHREAD_H
#define RS485MSGTHREAD_H

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <QThread>
#include "QextSerialPort/qextserialport.h"

class RS485MsgThread : public QThread
{
    Q_OBJECT
public:
    explicit RS485MsgThread(QObject *parent = 0);

    static RS485MsgThread *newInstance() {
        static QMutex mutex;

        if (!instance) {
            QMutexLocker locker(&mutex);

            if (!instance) {
                instance = new RS485MsgThread();
            }
        }

        return instance;
    }

    void Start(void);//启动延时返回数据包给报警主机

protected:
    void run();

private slots:
    void slotSendMsgToAlarmHost();

private:
    void RecvMsgFromAlarmHost();
    void ParseMsgFromAlarmHost();

private:
    static RS485MsgThread *instance;

    /****************与报警主机进行通信的串口参数**********************/
    //用来与报警主机进行通信的串口
    QextSerialPort *AlarmHostUart;

    //用来发送数据包给报警主机
    QTimer *SendMsgToAlarmHostTimer;

    //报警主机发回来的原始数据全部都保存在这里
    QByteArray RecvOriginalMsgFromAlarmHostBuffer;
};

#endif // RS485MSGTHREAD_H
