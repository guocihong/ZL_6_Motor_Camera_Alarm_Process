#include "mainstream.h"
#include "getcamerainfo.h"
#include "globalconfig.h"

MainStream *MainStream::instance = NULL;
QList<QByteArray> MainStream::MainImageBuffer = QList<QByteArray>();
QList<QString> MainStream::TriggerTimeBuffer = QList<QString>();

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

void MainStream::slotCaptureFrame(const QImage &image)
{
    if (MainImageBuffer.size() == 10) {//每秒保存一张图片，buffer里面保存的是10s之前的图片
        MainImageBuffer.removeFirst();
        TriggerTimeBuffer.removeFirst();
    }

    QByteArray tempData;
    QBuffer tempBuffer(&tempData);
    image.save(&tempBuffer,"JPG");//按照JPG解码保存数据
    QByteArray Base64 = tempData.toBase64();

    MainImageBuffer.append(Base64);
    TriggerTimeBuffer.append(CommonSetting::GetCurrentDateTimeNoSpace());

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
