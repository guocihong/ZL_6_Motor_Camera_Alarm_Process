#ifndef CHECKNETWORK_H
#define CHECKNETWORK_H

#include <QObject>
#include <QMutex>
#include <QTimer>

class CheckNetWork : public QObject
{
    Q_OBJECT
public:
    explicit CheckNetWork(QObject *parent = 0);

    static CheckNetWork *newInstance() {
        static QMutex mutex;

        if (!instance) {
            QMutexLocker locker(&mutex);

            if (!instance) {
                instance = new CheckNetWork();
            }
        }

        return instance;
    }

public:
    void Listen();
    void SendOKAckToAlarmHost();

private slots:
    void slotCheckNetworkState();

private:
    static CheckNetWork *instance;

    QTimer *UploadOkMsgToAlarmHostTimer;
};

#endif // CHECKNETWORK_H
