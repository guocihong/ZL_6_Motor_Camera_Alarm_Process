#include "substream.h"
#include "getcamerainfo.h"
#include "globalconfig.h"

SubStream *SubStream::instance = NULL;
QList<QByteArray> SubStream::SubImageBuffer = QList<QByteArray>();
QList<QString> SubStream::TriggerTimeBuffer = QList<QString>();

SubStream::SubStream(QObject *parent) :
    QObject(parent)
{
    SubStreamWorkThread = NULL;
}

void SubStream::Start()
{
    if (GlobalConfig::UseSubCamera == 1) {
        GetCameraInfo::GetSubCameraDevice();

        rgb_buff = (uchar*)malloc(GlobalConfig::SampleWidth * GlobalConfig::SampleHeight * 24 / 8);

        SubStreamWorkThread = new DeviceCameraThread(
                    rgb_buff, GlobalConfig::SubCameraSleepTime,
                    GetCameraInfo::SubStreamCamera,
                    GlobalConfig::SampleWidth,GlobalConfig::SampleHeight,16,V4L2_PIX_FMT_YUYV);

        connect(SubStreamWorkThread,SIGNAL(signalCaptureFrame(QImage)),this,SLOT(slotCaptureFrame(QImage)));

        connect(SubStreamWorkThread,SIGNAL(signalCameraOffline()),this,SLOT(slotReInitCamera()));

        SubStreamWorkThread->start();

        CameraOfflineCount = 0;
    }
}

void SubStream::slotCaptureFrame(const QImage &image)
{
    if (SubImageBuffer.size() == 10) {//每秒保存一张图片，buffer里面保存的是10s之前的图片
        SubImageBuffer.removeFirst();
        TriggerTimeBuffer.removeFirst();
    }

    QByteArray tempData;
    QBuffer tempBuffer(&tempData);
    image.save(&tempBuffer,"JPG");//按照JPG解码保存数据
    QByteArray Base64 = tempData.toBase64();

    SubImageBuffer.append(Base64);
    TriggerTimeBuffer.append(CommonSetting::GetCurrentDateTimeNoSpace());

    //摄像头在线
    GlobalConfig::SubStreamStateInfo = 1;
    CameraOfflineCount = 0;
}

void SubStream::slotReInitCamera()
{
    //摄像头离线
    GlobalConfig::SubStreamStateInfo = 0;
    CameraOfflineCount++;

    if (CameraOfflineCount == 10) {
        system("reboot");
    }

    //释放资源
    SubStreamWorkThread->StopCapture = true;
    SubStreamWorkThread->Finished = true;

    while(!SubStreamWorkThread->isFinished());

    SubStreamWorkThread->Clear();

    delete SubStreamWorkThread;

    SubStreamWorkThread = NULL;

    //重新初始化
    if (GlobalConfig::UseSubCamera == 1) {
        GetCameraInfo::GetSubCameraDevice();

        SubStreamWorkThread = new DeviceCameraThread(
                    rgb_buff,GlobalConfig::SubCameraSleepTime,
                    GetCameraInfo::SubStreamCamera,
                    GlobalConfig::SampleWidth,GlobalConfig::SampleHeight,16,V4L2_PIX_FMT_YUYV,this);

        connect(SubStreamWorkThread,SIGNAL(signalCaptureFrame(QImage)),this,SLOT(slotCaptureFrame(QImage)));

        connect(SubStreamWorkThread,SIGNAL(signalCameraOffline()),this,SLOT(slotReInitCamera()));

        SubStreamWorkThread->start();

        SubStreamWorkThread->StopCapture = false;

    }

    CommonSetting::WriteCommonFile("/bin/log.txt", CommonSetting::GetCurrentDateTimeNoSpace() +
                                   " SubStream offline\n");
}
