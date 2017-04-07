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

private:
    static SubStream *instance;

    uchar *rgb_buff;
    DeviceCameraThread *SubStreamWorkThread;
};

#endif // SUBSTREAM_H
