#ifndef OPERATECAMERA_H
#define OPERATECAMERA_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QDebug>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef Q_OS_LINUX
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#endif

#define BUFFER_NUMBER 4

typedef struct ImgBuffer
{
    quint8 *start;
    size_t length;
}ImgBuffer;

class DeviceCamera : public QObject
{
    Q_OBJECT
public:
    explicit DeviceCamera(QString VideoName, quint32 width, quint32 height, quint32 BPP, quint32 pixelformat, QObject *parent = 0);
    bool OpenCamera();
    bool InitCameraDevice();
    void CleanupCaptureDevice();
    bool ReadFrame();

public:
    QString VideoName;
    int fd;
    ImgBuffer *img_buffers;
    quint32 width;
    quint32 height;
    quint32 pixelformat;
    quint8 *yuyv_buff;
};

#endif // OPERATECAMERA_H
