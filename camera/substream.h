#ifndef SUBSTREAM_H
#define SUBSTREAM_H

#include <QObject>
#include <QMutexLocker>
#include "devicecamerathread.h"

class SubStream : public QObject
{
    Q_OBJECT
public:
    explicit SubStream(QObject *parent = 0);

    static SubStream *newInstance() {
        static QMutex mutex;

        if (!instance) {
            QMutexLocker locker(&mutex);

            if (!instance) {
                instance = new SubStream();
            }
        }

        return instance;
    }

    void Start();

public slots:
    void slotCaptureFrame(QImage image);
    void slotReInitCamera();

public:
    static QList<QImage> SubImageBuffer;
    DeviceCameraThread *SubStreamWorkThread;

private:
    static SubStream *instance;

    uchar *rgb_buff;

    //摄像头离线后，重复初始化摄像头10次以后，还是不能成功启动摄像头，则重启系统
    quint8 CameraOfflineCount;
};

#endif // SUBSTREAM_H
