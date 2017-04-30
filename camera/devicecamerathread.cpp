#include "devicecamerathread.h"
#include "globalconfig.h"
#include <QDebug>

//必须加以下内容,否则编译不能通过,为了兼容C和C99标准
#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif

//引入ffmpeg头文件
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
}

DeviceCameraThread::DeviceCameraThread(uchar *ptr,quint16 SampleInterval, QString VideoName, quint32 width, quint32 height, quint32 BPP, quint32 pixelformat, QObject *parent) :
    QThread(parent)
{
    this->rgb_buff = ptr;
    this->SampleInterval = SampleInterval;
    this->VideoName = VideoName;
    this->width = width;
    this->height = height;

    this->Finished = false;
    this->StopCapture = true;
    this->CameraOfflineCount = 0;

    devicecamera = new DeviceCamera(VideoName,width,height,BPP,pixelformat,this);
    if(devicecamera->OpenCamera()){
        if(devicecamera->InitCameraDevice()){
            this->isValidInitCamera = true;
            qDebug() << VideoName << "success in VIDIOC_STREAMON";
        }else{
            this->isValidInitCamera = false;
        }
    }else{
        this->isValidInitCamera = false;
    }
}

void DeviceCameraThread::run()
{
    while(!Finished){
        while(!StopCapture){
            //只有网络通的情况下，才采集视频
            if (GlobalConfig::system_mode == GlobalConfig::TcpMode) {
                ReadFrame();
            }
            msleep(SampleInterval);
        }
        msleep(100);
    }
}

void DeviceCameraThread::ReadFrame()
{
    if (isValidInitCamera) {
        if(devicecamera->ReadFrame()){
            if(YUYVToRGB24_FFmpeg(devicecamera->yuyv_buff, rgb_buff)){
                emit signalCaptureFrame(QImage(rgb_buff,width,height,
                                               QImage::Format_RGB888));
            }
        } else {//摄像头离线
            CameraOfflineCount++;

            if (CameraOfflineCount == 5) {
                //需要重新初始化摄像头
                this->StopCapture = true;
                emit signalCameraOffline();
            }
        }
    } else {
        this->StopCapture = true;
        emit signalCameraOffline();
    }
}

bool DeviceCameraThread::YUYVToRGB24_FFmpeg(uchar *pYUV,uchar *pRGB24)
{
    if (width < 1 || height < 1 || pYUV == NULL || pRGB24 == NULL){
        return false;
    }

    AVPicture pFrameYUV,pFrameRGB;
    avpicture_fill(&pFrameYUV,pYUV,AV_PIX_FMT_YUYV422,width,height);
    avpicture_fill(&pFrameRGB,pRGB24,AV_PIX_FMT_RGB24,width,height);

    //U,V互换
    uint8_t *ptmp = pFrameYUV.data[1];
    pFrameYUV.data[1] = pFrameYUV.data[2];
    pFrameYUV.data [2] = ptmp;

    struct SwsContext *imgCtx =
            sws_getContext(width,height,AV_PIX_FMT_YUYV422,width,height,AV_PIX_FMT_RGB24,SWS_BILINEAR,0,0,0);

    if(imgCtx != NULL){
        sws_scale(imgCtx,pFrameYUV.data,pFrameYUV.linesize,0,height,pFrameRGB.data,pFrameRGB.linesize);
        if(imgCtx){
            sws_freeContext(imgCtx);
            imgCtx = NULL;
        }
        return true;
    }else{
        sws_freeContext(imgCtx);
        imgCtx = NULL;
        return false;
    }
}

void DeviceCameraThread::Clear()
{
    devicecamera->CleanupCaptureDevice();
    free(devicecamera->yuyv_buff);
}
