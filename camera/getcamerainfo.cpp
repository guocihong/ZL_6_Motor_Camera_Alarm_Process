#include "getcamerainfo.h"
#include <stdio.h>
#include <QFile>
#include <QDebug>

QString GetCameraInfo::MainStreamCamera = QString("");
QString GetCameraInfo::SubStreamCamera = QString("");

void GetCameraInfo::GetMainCameraDevice()
{
    /************切断左防区摄像头电源**************/
    DeviceControlUtil::DisableMainStreamCamera();
    CommonSetting::Sleep(1000);

    system("find /sys/devices/platform -name video4linux > path.txt");//arm

    QString SubCameraPath = QString("");
    QFile SubCameraFile("path.txt");
    if(SubCameraFile.open(QFile::ReadOnly)){
        SubCameraPath =  QString(SubCameraFile.readAll()).trimmed().simplified();
        SubCameraFile.close();
    }

    /************使能主控制杆摄像头电源**************/
    DeviceControlUtil::EnableMainStreamCamera();
    CommonSetting::Sleep(1000);


    if (SubCameraPath.isEmpty()) {
        system("find /sys/devices/platform -name video4linux > path.txt");
    } else {
        system(tr("find /sys/devices/platform -name video4linux | grep -v \"%1\" > path.txt").arg(SubCameraPath).toAscii().data());
    }

    QString MainCameraPath = QString("");
    QFile MainCameraFile("path.txt");
    if(MainCameraFile.open(QFile::ReadOnly)){
        MainCameraPath =  QString(MainCameraFile.readAll()).trimmed().simplified();
        MainCameraFile.close();
    }
    if(MainCameraPath.isEmpty()){
        printf("not find main usb camera\n");
        GetCameraInfo::MainStreamCamera = QString("");
    }else{
        QString cmd = QString("ls %1 > device.txt").arg(MainCameraPath);
        system(cmd.toAscii().data());

        QFile MainCameraDeviceFile("device.txt");
        if(MainCameraDeviceFile.open(QFile::ReadOnly)){
            GetCameraInfo::MainStreamCamera = QString("/dev/") + QString(MainCameraDeviceFile.readAll()).trimmed();
            qDebug() << "MainStreamCamera = " << GetCameraInfo::MainStreamCamera;
            MainCameraDeviceFile.close();
        }
    }
}

void GetCameraInfo::GetSubCameraDevice()
{
    /************切断辅助控制摄像头电源**************/
    DeviceControlUtil::DisableSubStreamCamera();
    CommonSetting::Sleep(1000);

    system("find /sys/devices/platform -name video4linux > path.txt");//arm

    QString MainCameraPath = QString("");
    QFile MainCameraFile("path.txt");
    if(MainCameraFile.open(QFile::ReadOnly)){
        MainCameraPath =  QString(MainCameraFile.readAll()).trimmed().simplified();
        MainCameraFile.close();
    }

    /************使能辅助控制杆摄像头电源**************/
    DeviceControlUtil::EnableSubStreamCamera();
    CommonSetting::Sleep(1000);

    if (MainCameraPath.isEmpty()) {
        system("find /sys/devices/platform -name video4linux > path.txt");
    } else {
        system(tr("find /sys/devices/platform -name video4linux | grep -v \"%1\" > path.txt").arg(MainCameraPath).toAscii().data());
    }

    QString SubCameraPath = QString("");
    QFile SubCameraFile("path.txt");
    if(SubCameraFile.open(QFile::ReadOnly)){
        SubCameraPath =  QString(SubCameraFile.readAll()).trimmed().simplified();
        SubCameraFile.close();
    }
    if(SubCameraPath.isEmpty()){
        printf("not find sub usb camera\n");
        GetCameraInfo::SubStreamCamera = QString("");
    }else{
        QString cmd = QString("ls %1 > device.txt").arg(SubCameraPath);
        system(cmd.toAscii().data());

        QFile SubCameraDeviceFile("device.txt");
        if(SubCameraDeviceFile.open(QFile::ReadOnly)){
            GetCameraInfo::SubStreamCamera = QString("/dev/") + QString(SubCameraDeviceFile.readAll()).trimmed();
            qDebug() << "SubStreamCamera = " << GetCameraInfo::SubStreamCamera;
            SubCameraDeviceFile.close();
        }
    }
}
