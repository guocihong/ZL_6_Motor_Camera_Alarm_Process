#ifndef PARSEALARMHOSTTCPMSG_H
#define PARSEALARMHOSTTCPMSG_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include "globalconfig.h"

class ParseAlarmHostTcpMsg : public QObject
{
    Q_OBJECT
public:
    explicit ParseAlarmHostTcpMsg(QObject *parent = 0);

    static ParseAlarmHostTcpMsg *newInstance() {
        static QMutex mutex;

        if (!instance) {
            QMutexLocker locker(&mutex);

            if (!instance) {
                instance = new ParseAlarmHostTcpMsg();
            }
        }

        return instance;
    }

    void Parse(void);

    void SendOKAckToAlarmHost(TcpHelper *tcpHelper);
    void SendConfigInfoToAlarmHost(TcpHelper *tcpHelper);

    void SendDoubleImageToAlarmHost(TcpHelper *tcpHelper);
    void SendLeftAreaImageToAlarmHost(TcpHelper *tcpHelper);
    void SendRightAreaImageToAlarmHost(TcpHelper *tcpHelper);

    void ParseMotorControlCmdFromAlarmHost(QString Msg, TcpHelper *tcpHelper);
    void SendMotorStatusInfoToAlarmHost(QString cmd, TcpHelper *tcpHelper);

    static void SaveMotorLastestStatusInfo();

    void CommonCode(QByteArray data, TcpHelper *tcpHelper);

private slots:
    void slotParseMsgFromAlarmHost();
    void slotUpdateDateTime();

private:
    static ParseAlarmHostTcpMsg *instance;

    //解析报警主机下发的所有命令
    QTimer *ParseMsgFromAlarmHostTimer;

    //与报警主机进行时间同步
    QTimer *UpdateDateTimeTimer;
    QString NowTime;
};

#endif // PARSEALARMHOSTTCPMSG_H
