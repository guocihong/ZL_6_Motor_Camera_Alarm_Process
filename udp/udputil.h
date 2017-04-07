#ifndef UDPUTIL_H
#define UDPUTIL_H

#include <QObject>
#include <QUdpSocket>
#include <QMutexLocker>

class UdpUtil : public QObject
{
    Q_OBJECT
public:
    explicit UdpUtil(QObject *parent = 0);

    static UdpUtil *newInstance() {
        static QMutex mutex;

        if (!instance) {
            QMutexLocker locker(&mutex);

            if (!instance) {
                instance = new UdpUtil();
            }
        }

        return instance;
    }

    void Listen(void);//开启组播监听

public slots:
    void slotProcessPendingDatagrams();

private:
    static UdpUtil *instance;

    QUdpSocket *udp_socket;
};

#endif // UDPUTIL_H
