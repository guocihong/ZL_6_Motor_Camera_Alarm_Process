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
    void slotCaptureFrame(QImage image);
    void slotReInitCamera();

public:
    static QList<QImage> MainImageBuffer;

private:
    static MainStream *instance;

    uchar *rgb_buff;
    DeviceCameraThread *MainStreamWorkThread;
};

#endif // MAINSTREAM_H
