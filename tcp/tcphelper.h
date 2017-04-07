#ifndef TCPHELPER_H
#define TCPHELPER_H

#include <QObject>
#include <QTcpSocket>

class TcpHelper : public QObject
{
    Q_OBJECT
public:
    explicit TcpHelper(QObject *parent = 0);

    //保存报警主机tcp连接对象
    QTcpSocket *Socket;

    //保存报警主机发送的所有原始数据
    QByteArray RecvOriginalMsgBuffer;

    //从报警主机发送的所有原始数据中解析出一个个完整的数据包
    QList<QByteArray> RecvVaildCompleteMsgBuffer;

    //本设备返回给报警主机的数据包
    QList<QByteArray> SendMsgBuffer;
};

#endif // TCPHELPER_H
