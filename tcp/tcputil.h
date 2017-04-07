#ifndef TCPUTIL_H
#define TCPUTIL_H

#include <QObject>
#include <QMutex>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

class TcpUtil : public QObject
{
    Q_OBJECT
public:
    explicit TcpUtil(QObject *parent = 0);

    static TcpUtil *newInstance() {
        static QMutex mutex;

        if (!instance) {
            QMutexLocker locker(&mutex);

            if (!instance) {
                instance = new TcpUtil();
            }
        }

        return instance;
    }

    void Listen(void);

private slots:
    //处理报警主机的连接
    void slotProcessAlarmHostConnection();

    //报警主机断开连接,本设备是做为服务器,报警主机是做为客户端
    void slotAlarmHostDisconnect();

    //接收报警主机下发的所有网络配置信息和获取电机张力相关的信息
    void slotRecvMsgFromAlarmHost();

    //用来发送所有网络配置信息和电机张力的相关信息给报警主机
    void slotSendMsgToAlarmHost();

    //用来主动上报报警信息给报警主机
    void slotSendAlarmMsgToAlarmHost();

    //解析报警主机发送过来的数据包
    void slotParseMsgFromAlarmHost();

private:
    static TcpUtil *instance;

    //监听报警主机tcp连接
    QTcpServer *AlarmHostServerListener;

    //主动上报报警信息给报警主机
    QTcpSocket *SendAlarmMsgToAlarmHostSocket;

    //用来发送所有网络配置信息和电机张力的相关信息给报警主机
    QTimer *SendMsgToAlarmHostTimer;

    //用来主动上报报警信息给报警主机
    QTimer *SendAlarmMsgToAlarmHostTimer;

    //解析报警主机发送过来的数据包
    QTimer *ParseMsgFromAlarmHostTimer;
};

#endif // TCPUTIL_H
