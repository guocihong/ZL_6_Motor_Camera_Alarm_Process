#ifndef MAINSTREAM_H
#define MAINSTREAM_H

#include <QObject>
#include <QMutexLocker>
#include "devicecamerathread.h"

class MainStream : public QObject
{
    Q_OBJECT
public:
    explicit MainStream(QObject *parent = 0);

    static MainStream *newInstance() {
        static QMutex mutex;

        if (!instance) {
            QMutexLocker locker(&mutex);

            if (!instance) {
                instance = new MainStream();
            }
        }

        return instance;
    }

    void Start();

public slots:
    void slotCaptureFrame(const QImage &image);
    void slotReInitCamera();

public:
    static QList<QByteArray> MainImageBuffer;
    static QList<QString> TriggerTimeBuffer;

    DeviceCameraThread *MainStreamWorkThread;

private:
    static MainStream *instance;

    uchar *rgb_buff;

    //摄像头离线后，重复初始化摄像头10次以后，还是不能成功启动摄像头，则重启系统
    quint8 CameraOfflineCount;
};

#endif // MAINSTREAM_H
