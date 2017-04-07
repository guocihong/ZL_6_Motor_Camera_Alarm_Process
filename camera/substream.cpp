#include "substream.h"
#include "getcamerainfo.h"
#include "globalconfig.h"

SubStream *SubStream::instance = NULL;
QList<QImage> SubStream::SubImageBuffer = QList<QImage>();

SubStream::SubStream(QObject *parent) :
    QObject(parent)
{

}

void SubStream::Start()
{
    if (GlobalConfig::UseSubCamera == 1) {
        GetCameraInfo::GetSubCameraDevice();

        if (!GetCameraInfo::SubStreamCamera.isEmpty()) {
            rgb_buff = (uchar*)malloc(GlobalConfig::SampleWidth * GlobalConfig::SampleHeight * 24 / 8);

            SubStreamWorkThread = new DeviceCameraThread(
                        rgb_buff, GlobalConfig::SubCameraSleepTime,
                        GetCameraInfo::SubStreamCamera,
                        GlobalConfig::SampleWidth,GlobalConfig::SampleHeight,16,V4L2_PIX_FMT_YUYV);

            connect(SubStreamWorkThread,SIGNAL(signalCaptureFrame(QImage)),this,SLOT(slotCaptureFrame(QImage)));

            connect(SubStreamWorkThread,SIGNAL(signalCameraOffline()),this,SLOT(slotReInitCamera()));

            SubStreamWorkThread->start();
        }
    }
}

void SubStream::slotCaptureFrame(QImage image)
{
    if (SubImageBuffer.size() == 3) {
        SubImageBuffer.removeFirst();
    }

    SubImageBuffer.append(image);
}

void SubStream::slotReInitCamera()
{
    //释放资源
    SubStreamWorkThread->StopCapture = true;
    SubStreamWorkThread->Finished = true;

    while(!SubStreamWorkThread->isFinished());

    SubStreamWorkThread->Clear();

    delete SubStreamWorkThread;

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

    }

    CommonSetting::WriteCommonFile("/bin/log.txt", CommonSetting::GetCurrentDateTimeNoSpace() +
                                   " SubStream offline\n");
}
