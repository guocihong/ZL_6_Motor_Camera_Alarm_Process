#ifndef GETCAMERAINFO_H
#define GETCAMERAINFO_H

#include <QObject>
#include "devicecontrolutil.h"
#include "CommonSetting.h"

class GetCameraInfo : public QObject
{
    Q_OBJECT
public:
    static QString MainStreamCamera;//左防区摄像头设备文件
    static QString SubStreamCamera;//右防区摄像头设备文件

    static void GetMainCameraDevice();//主控制杆摄像头掉线时，获取主控制杆摄像头设备文件
    static void GetSubCameraDevice();//辅助控制杆摄像头掉线时，获取辅助控制杆摄像头设备文件
};

#endif // GETCAMERAINFO_H
