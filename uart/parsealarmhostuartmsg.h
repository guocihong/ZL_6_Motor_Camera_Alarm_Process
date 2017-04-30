#ifndef PARSEALARMHOSTUARTMSG_H
#define PARSEALARMHOSTUARTMSG_H

#include <QObject>
#include <QMutex>
#include <QTimer>

class ParseAlarmHostUartMsg : public QObject
{
    Q_OBJECT
public:
    explicit ParseAlarmHostUartMsg(QObject *parent = 0);

    static ParseAlarmHostUartMsg *newInstance() {
        static QMutex mutex;

        if (!instance) {
            QMutexLocker locker(&mutex);

            if (!instance) {
                instance = new ParseAlarmHostUartMsg();
            }
        }

        return instance;
    }

    void Parse(void);//实时解析报警主机RS485发生过来的数据包
    void SendDataToAlarmHost(QByteArray data);

    void get_alarm_detail_info();//读取报警详细信息

public slots:
    void slotParseAlarmHostUartMsg(void);

private:
    static ParseAlarmHostUartMsg *instance;

    QTimer *ParseAlarmHostUartMsgTimer;
};

#endif // PARSEALARMHOSTUARTMSG_H
