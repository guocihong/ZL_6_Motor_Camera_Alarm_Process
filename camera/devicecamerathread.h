#ifndef DeviceCameraThread_H
#define DeviceCameraThread_H

#include <QThread>
#include <QImage>
#include "devicecamera.h"

class DeviceCameraThread : public QThread
{
    Q_OBJECT

public:
    explicit DeviceCameraThread(uchar *ptr, quint16 SampleInterval,QString VideoName, quint32 width, quint32 height, quint32 BPP, quint32 pixelformat, QObject *parent = 0);

    void ReadFrame();
    bool YUYVToRGB24_FFmpeg(uchar *pYUV,uchar *pRGB24);
    void Clear();

protected:
    void run();

signals:
    void signalCaptureFrame(QImage image);
    void signalCameraOffline();

public:
    DeviceCamera *devicecamera;
    bool isValidInitCamera;
    quint8 CameraOfflineCount;

    uchar *rgb_buff;
    quint16 SampleInterval;
    QString VideoName;
    quint32 width;
    quint32 height;

    volatile bool Finished;
    volatile bool StopCapture;//拍照是暂停采集图片
};

#endif // DeviceCameraThread_H
