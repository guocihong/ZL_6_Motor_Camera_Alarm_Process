#include "mainstream.h"
#include "getcamerainfo.h"
#include "globalconfig.h"

MainStream *MainStream::instance = NULL;
QList<QImage> MainStream::MainImageBuffer = QList<QImage>();

MainStream::MainStream(QObject *parent) :
    QObject(parent)
{
    MainStreamWorkThread = NULL;
}

void MainStream::Start()
{
    if (GlobalConfig::UseMainCamera == 1) {
        GetCameraInfo::GetMainCameraDevice();

        rgb_buff = (uchar*)malloc(GlobalConfig::SampleWidth *
                                  GlobalConfig::SampleHeight * 24 / 8);

        MainStreamWorkThread = new DeviceCameraThread(
                    rgb_buff,GlobalConfig::MainCameraSleepTime,
                    GetCameraInfo::MainStreamCamera,
                    GlobalConfig::SampleWidth,GlobalConfig::SampleHeight,16,V4L2_PIX_FMT_YUYV,this);

        connect(MainStreamWorkThread,SIGNAL(signalCaptureFrame(QImage)),this,SLOT(slotCaptureFrame(QImage)));

        connect(MainStreamWorkThread,SIGNAL(signalCameraOffline()),this,SLOT(slotReInitCamera()));

        MainStreamWorkThread->start();

        CameraOfflineCount = 0;
    }
}

void MainStream::slotCaptureFrame(QImage image)
{
    if (MainImageBuffer.size() == 3) {
        MainImageBuffer.removeFirst();
    }

    MainImageBuffer.append(image);

    //摄像头在线
    GlobalConfig::MainStreamStateInfo = 1;
    CameraOfflineCount = 0;
}

void MainStream::slotReInitCamera()
{
    //摄像头离线
    GlobalConfig::MainStreamStateInfo = 0;
    CameraOfflineCount++;

    if (CameraOfflineCount == 10) {
        system("reboot");
    }

    //释放资源
    MainStreamWorkThread->StopCapture = true;
    MainStreamWorkThread->Finished = true;

    while(!MainStreamWorkThread->isFinished());

    MainStreamWorkThread->Clear();

    delete MainStreamWorkThread;

    MainStreamWorkThread = NULL;

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

        MainStreamWorkThread->StopCapture = false;
    }

    CommonSetting::WriteCommonFile("/bin/log.txt", CommonSetting::GetCurrentDateTimeNoSpace() +
                                   " MainStream offline\n");
}
