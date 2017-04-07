#include "mainstream.h"
#include "getcamerainfo.h"
#include "globalconfig.h"

MainStream *MainStream::instance = NULL;
QList<QImage> MainStream::MainImageBuffer = QList<QImage>();

MainStream::MainStream(QObject *parent) :
    QObject(parent)
{
}

void MainStream::Start()
{
    if (GlobalConfig::UseMainCamera == 1) {
        GetCameraInfo::GetMainCameraDevice();

        if (!GetCameraInfo::MainStreamCamera.isEmpty()) {
            rgb_buff = (uchar*)malloc(GlobalConfig::SampleWidth *
                                      GlobalConfig::SampleHeight * 24 / 8);

            MainStreamWorkThread = new DeviceCameraThread(
                        rgb_buff,GlobalConfig::MainCameraSleepTime,
                        GetCameraInfo::MainStreamCamera,
                        GlobalConfig::SampleWidth,GlobalConfig::SampleHeight,16,V4L2_PIX_FMT_YUYV,this);

            connect(MainStreamWorkThread,SIGNAL(signalCaptureFrame(QImage)),this,SLOT(slotCaptureFrame(QImage)));

            connect(MainStreamWorkThread,SIGNAL(signalCameraOffline()),this,SLOT(slotReInitCamera()));

            MainStreamWorkThread->start();
        }
    }
}

void MainStream::slotCaptureFrame(QImage image)
{
    if (MainImageBuffer.size() == 3) {
        MainImageBuffer.removeFirst();
    }

    MainImageBuffer.append(image);
}

void MainStream::slotReInitCamera()
{
    //释放资源
    MainStreamWorkThread->StopCapture = true;
    MainStreamWorkThread->Finished = true;

    while(!MainStreamWorkThread->isFinished());

    MainStreamWorkThread->Clear();

    delete MainStreamWorkThread;

    //重新初始化
    if (GlobalConfig::UseMainCamera == 1) {
        GetCameraInfo::GetMainCameraDevice();

        MainStreamWorkThread = new DeviceCameraThread(
                    rgb_buff,GlobalConfig::MainCameraSleepTime,
                    GetCameraInfo::MainStreamCamera,
                    GlobalConfig::SampleWidth,GlobalConfig::SampleHeight,16,V4L2_PIX_FMT_YUYV,this);

        connect(MainStreamWorkThread,SIGNAL(signalCaptureFrame(QImage)),this,SLOT(slotCaptureFrame(QImage)));

        connect(MainStreamWorkThread,SIGNAL(signalCameraOffline()),this,SLOT(slotReInitCamera()));

        MainStreamWorkThread->start();

    }

    CommonSetting::WriteCommonFile("/bin/log.txt", CommonSetting::GetCurrentDateTimeNoSpace() +
                                   " MainStream offline\n");
}
