#include "uploadalarmmsgtoalarmhostusingtcp.h"
#include "parsealarmhosttcpmsg.h"
#include "globalconfig.h"
#include "camera/mainstream.h"
#include "camera/substream.h"
#include <QDomDocument>
#include <QStringList>
#include <QBuffer>

UploadAlarmMsgToAlarmHostUsingTcp *UploadAlarmMsgToAlarmHostUsingTcp::instance = NULL;

UploadAlarmMsgToAlarmHostUsingTcp::UploadAlarmMsgToAlarmHostUsingTcp(QObject *parent) :
    QObject(parent)
{
}

void UploadAlarmMsgToAlarmHostUsingTcp::Listen()
{
    UploadAlarmMsgToAlarmHostTimer = new QTimer(this);
    UploadAlarmMsgToAlarmHostTimer->setInterval(200);
    connect(UploadAlarmMsgToAlarmHostTimer,SIGNAL(timeout()),this,SLOT(slotUploadAlarmMsgToAlarmHost()));
    UploadAlarmMsgToAlarmHostTimer->start();
}

void UploadAlarmMsgToAlarmHostUsingTcp::slotUploadAlarmMsgToAlarmHost()
{
    if (GlobalConfig::system_status < GlobalConfig::SYS_CHECK) {//只有实时检测状态才上传报警
        return;
    }

    //1、判断是否发生报警
    //门磁发生报警
    static quint8 PreDoorKeepState = 0;
    quint8 temp8 = GlobalConfig::doorkeep_state;
    if (temp8 != 0) {//发生报警
        if (temp8 != PreDoorKeepState) {//新报警
            PreDoorKeepState = temp8;//保存上一次报警状态,每次报警只上传一次报警信息

            //摄像头停止采集
            if (GlobalConfig::UseMainCamera) {
                if (MainStream::newInstance()->MainStreamWorkThread != NULL) {
                    MainStream::newInstance()->MainStreamWorkThread->StopCapture = true;
                }
            }

            if (GlobalConfig::UseSubCamera) {
                if (SubStream::newInstance()->SubStreamWorkThread != NULL) {
                    SubStream::newInstance()->SubStreamWorkThread->StopCapture = true;
                }
            }

            //上传报警信息
            for (int i = (10 - 1); i >= (10 - GlobalConfig::AlarmImageCount); i--) {
                AlarmMsg(0,i);
            }

            for (int i = (10 - 1); i >= (10 - GlobalConfig::AlarmImageCount); i--) {
                AlarmMsg(1,i);
            }

            //摄像头恢复采集
            if (GlobalConfig::UseMainCamera) {
                if (MainStream::newInstance()->MainStreamWorkThread != NULL) {
                    MainStream::newInstance()->MainStreamWorkThread->StopCapture = false;
                }
            }

            if (GlobalConfig::UseSubCamera) {
                if (SubStream::newInstance()->SubStreamWorkThread != NULL) {
                    SubStream::newInstance()->SubStreamWorkThread->StopCapture = false;
                }
            }
        }
    } else {//没有发生报警
        PreDoorKeepState = temp8;
    }

    //杆自身攀爬报警
    static quint8 PreConrolState = 0;
    temp8 = GlobalConfig::zs_climb_alarm_flag;
    if (temp8 != 0) {//发生报警
        if (temp8 != PreConrolState) {//新报警
            PreConrolState = temp8;//保存上一次报警状态,每次报警只上传一次报警信息

            //摄像头停止采集
            if (GlobalConfig::UseMainCamera) {
                if (MainStream::newInstance()->MainStreamWorkThread != NULL) {
                    MainStream::newInstance()->MainStreamWorkThread->StopCapture = true;
                }
            }

            if (GlobalConfig::UseSubCamera) {
                if (SubStream::newInstance()->SubStreamWorkThread != NULL) {
                    SubStream::newInstance()->SubStreamWorkThread->StopCapture = true;
                }
            }

            //上传报警信息
            for (int i = (10 - 1); i >= (10 - GlobalConfig::AlarmImageCount); i--) {
                AlarmMsg(0,i);
            }

            for (int i = (10 - 1); i >= (10 - GlobalConfig::AlarmImageCount); i--) {
                AlarmMsg(1,i);
            }

            //摄像头恢复采集
            if (GlobalConfig::UseMainCamera) {
                if (MainStream::newInstance()->MainStreamWorkThread != NULL) {
                    MainStream::newInstance()->MainStreamWorkThread->StopCapture = false;
                }
            }

            if (GlobalConfig::UseSubCamera) {
                if (SubStream::newInstance()->SubStreamWorkThread != NULL) {
                    SubStream::newInstance()->SubStreamWorkThread->StopCapture = false;
                }
            }
        }
    } else {//没有发生报警
        PreConrolState = temp8;
    }

    //左防区钢丝报警
    static quint8 PreLeftAreaState = 0;
    temp8 = GlobalConfig::adl_alarm_flag;
    if (temp8 != 0) {//发生报警
        if (temp8 != PreLeftAreaState) {//新报警
            PreLeftAreaState = temp8;//保存上一次报警状态,每次报警只上传一次报警信息

            //摄像头停止采集
            if (GlobalConfig::UseMainCamera) {
                if (MainStream::newInstance()->MainStreamWorkThread != NULL) {
                    MainStream::newInstance()->MainStreamWorkThread->StopCapture = true;
                }
            }

            //上传报警信息
            for (int i = (10 - 1); i >= (10 - GlobalConfig::AlarmImageCount); i--) {
                AlarmMsg(0,i);
            }

            //摄像头恢复采集
            if (GlobalConfig::UseMainCamera) {
                if (MainStream::newInstance()->MainStreamWorkThread != NULL) {
                    MainStream::newInstance()->MainStreamWorkThread->StopCapture = false;
                }
            }
        }
    } else {//没有发生报警
        PreLeftAreaState = temp8;
    }

    //右防区钢丝报警
    static quint8 PreRightAreaState = 0;
    temp8 = GlobalConfig::adr_alarm_flag;
    if (temp8 != 0) {//发生报警
        if (temp8 != PreRightAreaState) {//新报警
            PreRightAreaState = temp8;//保存上一次报警状态,每次报警只上传一次报警信息

            //摄像头停止采集
            if (GlobalConfig::UseSubCamera) {
                if (SubStream::newInstance()->SubStreamWorkThread != NULL) {
                    SubStream::newInstance()->SubStreamWorkThread->StopCapture = true;
                }
            }

            //上传报警信息
            for (int i = (10 - 1); i >= (10 - GlobalConfig::AlarmImageCount); i--) {
                AlarmMsg(1,i);
            }

            //摄像头恢复采集
            if (GlobalConfig::UseSubCamera) {
                if (SubStream::newInstance()->SubStreamWorkThread != NULL) {
                    SubStream::newInstance()->SubStreamWorkThread->StopCapture = false;
                }
            }        }
    } else {//没有发生报警
        PreRightAreaState = temp8;
    }

    //分析左防区摄像头的状态信息
    if (GlobalConfig::UseMainCamera) {
        static bool PreMainStreamStateInfo = 0;

        if (GlobalConfig::MainStreamStateInfo != PreMainStreamStateInfo) {
            PreMainStreamStateInfo = GlobalConfig::MainStreamStateInfo;

            if (GlobalConfig::MainStreamStateInfo) {//摄像头在线
                CameraStatusInfo(0,"摄像头恢复");
            } else {//摄像头离线
                CameraStatusInfo(0,"摄像头掉线");
            }
        }
    }

    //分析右防区摄像头的状态信息
    if (GlobalConfig::UseSubCamera) {
        static bool PreSubStreamStateInfo = 0;

        if (GlobalConfig::SubStreamStateInfo != PreSubStreamStateInfo) {
            PreSubStreamStateInfo = GlobalConfig::SubStreamStateInfo;

            if (GlobalConfig::SubStreamStateInfo) {//摄像头在线
                CameraStatusInfo(1,"摄像头恢复");
            } else {//摄像头离线
                CameraStatusInfo(1,"摄像头掉线");
            }
        }
    }
}

void UploadAlarmMsgToAlarmHostUsingTcp::AlarmMsg(int id,int index)
{
    //1、获取最后一次报警时的详细信息
    QString AlarmDetailInfo = GetAlarmDetailInfo();

    //2、打包报警信息
    QString MessageMerge;
    QDomDocument AckDom;

    //xml声明
    QString XmlHeader("version=\"1.0\" encoding=\"UTF-8\"");
    AckDom.appendChild(AckDom.createProcessingInstruction("xml", XmlHeader));

    //创建根元素
    QDomElement RootElement = AckDom.createElement("Device");

    RootElement.setAttribute("DeviceIP", GlobalConfig::LocalHostIP);

    if (id == 0) {//左防区
        RootElement.setAttribute("DefenceID", GlobalConfig::MainDefenceID);
    } else if (id == 1) {//右防区
        RootElement.setAttribute("DefenceID", GlobalConfig::SubDefenceID);
    }

    AckDom.appendChild(RootElement);

    //创建DeviceAlarmImage元素
    QDomElement DeviceAlarmImage = AckDom.createElement("DeviceAlarmImage");

    DeviceAlarmImage.setAttribute("id", QString::number(id));
    DeviceAlarmImage.setAttribute("status", AlarmDetailInfo);

    if (id == 0) {//左防区报警详细信息
        QString msg = QString("");

        //1、分析左防区瞬间张力报警通道
        QStringList LeftMomentAlarmChannelList = QStringList();
        for (int i = 0; i < 8; i++) {
            //分析到底是左防区哪一位瞬间张力报警
            if (HIGH(GlobalConfig::ad_alarm_exts) & (1 << i)) {//报警
                //分析拨码开关是否打开
                if (HIGH(GlobalConfig::ad_sensor_mask_LR) & (1 << i)) {//拨码打开
                    LeftMomentAlarmChannelList << QString::number(i + 1);
                }
            }
        }

        msg = QString("LeftMomentAlarm[%1]").arg(LeftMomentAlarmChannelList.join(","));


        //2、分析左防区静态报警通道
        QStringList LeftStaticAlarmChannelList = QStringList();
        for (int i = 0; i < 8; i++) {
            //分析到底是左防区哪一位静态报警
            if (HIGH(GlobalConfig::ad_alarm_base) & (1 << i)) {//报警
                //分析拨码开关是否打开
                if (HIGH(GlobalConfig::ad_sensor_mask_LR) & (1 << i)) {//拨码打开
                    LeftStaticAlarmChannelList << QString::number(i + 1);
                }
            }
        }

        msg = msg + QString(" ") +
                QString("LeftStaticAlarm[%1]").arg(LeftStaticAlarmChannelList.join(","));


        //3、分析门磁是否报警
        if (GlobalConfig::doorkeep_state) {//发生报警
            msg = msg + QString(" ") + QString("BreakAlarm[1]");
        } else {//没有报警
            msg = msg + QString(" ") + QString("BreakAlarm[0]");
        }

        DeviceAlarmImage.setAttribute("msg", msg);
    } else if (id == 1) {//右防区报警详细信息
        QString msg = QString("");

        //1、分析右防区瞬间张力报警通道
        QStringList RightMomentAlarmChannelList = QStringList();
        for (int i = 0; i < 8; i++) {
            //分析到底是右防区哪一位瞬间张力报警
            if (LOW(GlobalConfig::ad_alarm_exts) & (1 << i)) {//报警
                //分析拨码开关是否打开
                if (LOW(GlobalConfig::ad_sensor_mask_LR) & (1 << i)) {//拨码打开
                    RightMomentAlarmChannelList << QString::number(i + 1);
                }
            }
        }

        msg = QString("RightMomentAlarm[%1]").arg(RightMomentAlarmChannelList.join(","));


        //2、分析右防区静态报警通道
        QStringList RightStaticAlarmChannelList = QStringList();
        for (int i = 0; i < 8; i++) {
            //分析到底是右防区哪一位静态报警
            if (LOW(GlobalConfig::ad_alarm_base) & (1 << i)) {//报警
                //分析拨码开关是否打开
                if (LOW(GlobalConfig::ad_sensor_mask_LR) & (1 << i)) {//拨码打开
                    RightStaticAlarmChannelList << QString::number(i + 1);
                }
            }
        }

        msg = msg + QString(" ") +
                QString("RightStaticAlarm[%1]").arg(RightStaticAlarmChannelList.join(","));


        //3、分析门磁是否报警
        if (GlobalConfig::doorkeep_state) {//发生报警
            msg = msg + QString(" ") + QString("BreakAlarm[1]");
        } else {//没有报警
            msg = msg + QString(" ") + QString("BreakAlarm[0]");
        }

        DeviceAlarmImage.setAttribute("msg", msg);
    }

    if (id == 0) {//左防区图像
        if (MainStream::TriggerTimeBuffer.size() > 0) {
            if (MainStream::TriggerTimeBuffer.size() < index) {
                DeviceAlarmImage.setAttribute("triggertime", MainStream::TriggerTimeBuffer.at(0));
            } else {
                DeviceAlarmImage.setAttribute("triggertime", MainStream::TriggerTimeBuffer.at(index));
            }
        } else {
            DeviceAlarmImage.setAttribute("triggertime", QString(""));
        }

        QString MainImageBase64;
        if (MainStream::MainImageBuffer.size() > 0) {
//            QImage MainImageRgb888;

//            if (MainStream::MainImageBuffer.size() < index) {
//                MainImageRgb888 = MainStream::MainImageBuffer.at(0);
//            } else {
//                MainImageRgb888 = MainStream::MainImageBuffer.at(index);
//            }

//            QByteArray tempData;
//            QBuffer tempBuffer(&tempData);
//            MainImageRgb888.save(&tempBuffer,"JPG");//按照JPG解码保存数据
//            MainImageBase64 = QString(tempData.toBase64());

            if (MainStream::MainImageBuffer.size() < index) {
                MainImageBase64 = MainStream::MainImageBuffer.at(0);
            } else {
                MainImageBase64 = MainStream::MainImageBuffer.at(index);
            }
        } else {
            MainImageBase64 = QString("");
        }

        QDomText mainStreamText = AckDom.createTextNode(MainImageBase64);
        DeviceAlarmImage.appendChild(mainStreamText);
    } else if (id == 1) {//右防区图像
        if (SubStream::TriggerTimeBuffer.size() > 0) {
            if (SubStream::TriggerTimeBuffer.size() < index) {
                DeviceAlarmImage.setAttribute("triggertime", SubStream::TriggerTimeBuffer.at(0));
            } else {
                DeviceAlarmImage.setAttribute("triggertime", SubStream::TriggerTimeBuffer.at(index));
            }
        } else {
            DeviceAlarmImage.setAttribute("triggertime", QString(""));
        }

        QString SubImageBase64;
        if (SubStream::SubImageBuffer.size() > 0) {
//            QImage SubImageRgb888;

//            if (SubStream::SubImageBuffer.size() < index) {
//                SubImageRgb888 = SubStream::SubImageBuffer.at(0);
//            } else {
//                SubImageRgb888 = SubStream::SubImageBuffer.at(index);
//                SubImageRgb888.save(QString("/mnt/right_%1.jpg").arg(index),"JPG");
//            }

//            QByteArray tempData;
//            QBuffer tempBuffer(&tempData);
//            SubImageRgb888.save(&tempBuffer,"JPG");//按照JPG解码保存数据
//            SubImageBase64 = QString(tempData.toBase64());


            if (SubStream::SubImageBuffer.size() < index) {
                SubImageBase64 = SubStream::SubImageBuffer.at(0);
            } else {
                SubImageBase64 = SubStream::SubImageBuffer.at(index);
            }
        } else {
            SubImageBase64 = QString("");
        }

        QDomText subStreamText = AckDom.createTextNode(SubImageBase64);
        DeviceAlarmImage.appendChild(subStreamText);
    }

    RootElement.appendChild(DeviceAlarmImage);

    QTextStream Out(&MessageMerge);
    AckDom.save(Out,4);

    int length = MessageMerge.size();
    MessageMerge = QString("IALARM:") + QString("%1").arg(length,13,10,QLatin1Char('0')) + MessageMerge;

    GlobalConfig::SendAlarmMsgToAlarmHostBuffer.append(MessageMerge.toAscii());
}

void UploadAlarmMsgToAlarmHostUsingTcp::CameraStatusInfo(int id, QString CameraStatusInfo)
{
    QString MessageMerge;
    QDomDocument AckDom;

    //xml声明
    QString XmlHeader("version=\"1.0\" encoding=\"UTF-8\"");
    AckDom.appendChild(AckDom.createProcessingInstruction("xml", XmlHeader));

    //创建根元素
    QDomElement RootElement = AckDom.createElement("Device");

    RootElement.setAttribute("DeviceIP", GlobalConfig::LocalHostIP);

    if (id == 0) {//左防区
        RootElement.setAttribute("DefenceID", GlobalConfig::MainDefenceID);
    } else if (id == 1) {//右防区
        RootElement.setAttribute("DefenceID", GlobalConfig::SubDefenceID);
    }

    AckDom.appendChild(RootElement);

    //创建DeviceError元素
    QDomElement DeviceError = AckDom.createElement("DeviceError");
    QDomText DeviceErrorText = AckDom.createTextNode(CameraStatusInfo);
    DeviceError.appendChild(DeviceErrorText);

    RootElement.appendChild(DeviceError);

    QTextStream Out(&MessageMerge);
    AckDom.save(Out,4);

    int length = MessageMerge.size();
    MessageMerge = QString("IALARM:") + QString("%1").arg(length,13,10,QLatin1Char('0')) + MessageMerge;

    GlobalConfig::SendAlarmMsgToAlarmHostBuffer.append(MessageMerge.toAscii());
}

QString UploadAlarmMsgToAlarmHostUsingTcp::GetAlarmDetailInfo()
{
    QStringList StatusInfoList = QStringList();
    QByteArray data = QByteArray();

    //1、保存配置信息
    data[0] = 0x1E;
    data[1] = 0xE8;
    data[2] = 0x1C;
    data[3] = 0x08;

    data[4] = HIGH(GlobalConfig::ad_sensor_mask_LR);//张力mask左
    data[5] = LOW(GlobalConfig::ad_sensor_mask_LR); //张力mask右

    data[6] = HIGH(GlobalConfig::ad_still_dn);      //静态张力允许下限高字节
    data[7] = LOW(GlobalConfig::ad_still_dn);       //静态张力允许下限低字节

    data[8] = HIGH(GlobalConfig::ad_still_up);      //静态张力允许上限高字节
    data[9] = LOW(GlobalConfig::ad_still_up);       //静态张力允许上限低字节

    data[10] = GlobalConfig::system_status;

    for (int i = 0; i < 6; i++) {                   //左1~6的报警阀值
        data[11 + i] = GlobalConfig::ad_still_Dup[i];
    }

    //左开关量
    data[17] = 0;

    //杆自身
    data[18] = GlobalConfig::ad_still_Dup[12];

    for (int i = 0; i < 6; i++) {                   //右1~6的报警阀值
        data[19 + i] = GlobalConfig::ad_still_Dup[6 + i];
    }

    //右开关量
    data[25] = 0;

    //杆自身
    data[26] = GlobalConfig::ad_still_Dup[12];

    data[27] = 0;
    data[28] = GlobalConfig::ip_addr;
    data[29] = GlobalConfig::beep_during_temp * SCHEDULER_TICK / 1000;
    data[30] = GlobalConfig::gl_chnn_index;
    StatusInfoList << CommonCode(data);

    data.clear();

    //2、保存报警信息
    //返回：16 01 设备地址 0A E8 08 1A  [张力掩码左] [张力掩码右] [外力报警左]  [外力报警右]
    //[静态张力报警左] [静态张力报警右] [门磁]  校验和
    data[0] = 0x0A;
    data[1] = 0xE8;
    data[2] = 0x08;
    data[3] = 0x1A;

    data[4] = HIGH(GlobalConfig::ad_sensor_mask_LR);//张力掩码左
    data[5] = LOW(GlobalConfig::ad_sensor_mask_LR); //张力掩码右

    data[6] = HIGH(GlobalConfig::AlarmDetailInfo.ExternalAlarm);    //外力报警左
    data[7] = LOW(GlobalConfig::AlarmDetailInfo.ExternalAlarm);     //外力报警右

    data[8] = HIGH(GlobalConfig::AlarmDetailInfo.StaticAlarm);      //静态张力报警左
    data[9] = LOW(GlobalConfig::AlarmDetailInfo.StaticAlarm);       //静态张力报警右

    data[10] = GlobalConfig::AlarmDetailInfo.DoorKeepAlarm;         //门磁
    StatusInfoList << CommonCode(data);

    data.clear();

    //3、保存瞬间张力
    //返回：16 01 设备地址 23 E8 21 1C 左1~8 右1~8 校验和
    data[0] = 0x23;
    data[1] = 0xE8;
    data[2] = 0x21;
    data[3] = 0x1C;

    for (int i = 0; i < 6; i++) {//左1~6的瞬间张力
        data[4 + (i << 1)] = HIGH(GlobalConfig::AlarmDetailInfo.InstantSampleValue[i]);
        data[5 + (i << 1)] = LOW(GlobalConfig::AlarmDetailInfo.InstantSampleValue[i]);
    }

    //左开关量
    data[16] = 0;
    data[17] = 0;

    //杆自身
    data[18] = HIGH(GlobalConfig::AlarmDetailInfo.InstantSampleValue[12]);
    data[19] = LOW(GlobalConfig::AlarmDetailInfo.InstantSampleValue[12]);


    for (int i = 0; i < 6; i++) {//右1~6的瞬间张力
        data[20 + (i << 1)] = HIGH(GlobalConfig::AlarmDetailInfo.InstantSampleValue[6 + i]);
        data[21 + (i << 1)] = LOW(GlobalConfig::AlarmDetailInfo.InstantSampleValue[6 + i]);
    }

    //右开关量
    data[32] = 0;
    data[33] = 0;

    //杆自身
    data[34] = HIGH(GlobalConfig::AlarmDetailInfo.InstantSampleValue[12]);
    data[35] = LOW(GlobalConfig::AlarmDetailInfo.InstantSampleValue[12]);

    StatusInfoList << CommonCode(data);

    data.clear();

    //4、存静态基准
    //返回：16 01 设备地址 23 E8 21 1D 左1~8 右1~8 校验和
    data[0] = 0x23;
    data[1] = 0xE8;
    data[2] = 0x21;
    data[3] = 0x1D;

    for (int i = 0; i < 6; i++) {//左1~6的瞬间张力
        data[4 + (i << 1)] = HIGH(GlobalConfig::AlarmDetailInfo.StaticBaseValue[i]);
        data[5 + (i << 1)] = LOW(GlobalConfig::AlarmDetailInfo.StaticBaseValue[i]);
    }

    //左开关量
    data[16] = 0;
    data[17] = 0;

    //杆自身
    data[18] = HIGH(GlobalConfig::AlarmDetailInfo.StaticBaseValue[12]);
    data[19] = LOW(GlobalConfig::AlarmDetailInfo.StaticBaseValue[12]);


    for (int i = 0; i < 6; i++) {//右1~6的瞬间张力
        data[20 + (i << 1)] = HIGH(GlobalConfig::AlarmDetailInfo.StaticBaseValue[6 + i]);
        data[21 + (i << 1)] = LOW(GlobalConfig::AlarmDetailInfo.StaticBaseValue[6 + i]);
    }

    //右开关量
    data[32] = 0;
    data[33] = 0;

    //杆自身
    data[34] = HIGH(GlobalConfig::AlarmDetailInfo.StaticBaseValue[12]);
    data[35] = LOW(GlobalConfig::AlarmDetailInfo.StaticBaseValue[12]);

    StatusInfoList << CommonCode(data);

    data.clear();

    //5、钢丝是否剪断
    //返回：16 01 设备地址 06 E8 04 1E [钢丝剪断左] [钢丝剪断右] [是否处于调整钢丝模式] 校验和
    data[0] = 0x06;
    data[1] = 0xE8;
    data[2] = 0x04;
    data[3] = 0x1E;
    data[4] = HIGH(GlobalConfig::ad_chnn_wire_cut);
    data[5] = LOW(GlobalConfig::ad_chnn_wire_cut);
    data[6] = GlobalConfig::gl_motor_adjust_flag;
    StatusInfoList << CommonCode(data);

    return StatusInfoList.join(" ");
}

QString UploadAlarmMsgToAlarmHostUsingTcp::CommonCode(QByteArray data)
{
    QByteArray cmd = QByteArray();

    cmd[0] = 0x16;
    cmd[1] = 0x01;
    cmd[2] = GlobalConfig::ip_addr;

    int size = data.size();
    for (int i = 0; i < size; i++) {
        cmd[i + 3] = data[i];
    }

    //计算校验和
    quint8 chk_sum = 0;
    for (int i = 1; i < (size + 3) ; i++) {
        chk_sum += cmd[i];
    }

    cmd[size + 3] = chk_sum;

    QStringList str = QStringList();
    for (int i = 0; i < (size + 4); i++) {
        str << QString("%1").arg(cmd[i],2,16,QLatin1Char('0'));
    }

    return str.join(" ");
}

void UploadAlarmMsgToAlarmHostUsingTcp::UploadStressAlarmMsg(int id, int index)
{
    //1、打包报警信息
    QString MessageMerge;
    QDomDocument AckDom;

    //xml声明
    QString XmlHeader("version=\"1.0\" encoding=\"UTF-8\"");
    AckDom.appendChild(AckDom.createProcessingInstruction("xml", XmlHeader));

    //创建根元素
    QDomElement RootElement = AckDom.createElement("Device");

    RootElement.setAttribute("DeviceIP", GlobalConfig::LocalHostIP);

    if (id == 0) {//左防区
        RootElement.setAttribute("DefenceID", GlobalConfig::MainDefenceID);
    } else if (id == 1) {//右防区
        RootElement.setAttribute("DefenceID", GlobalConfig::SubDefenceID);
    }

    AckDom.appendChild(RootElement);

    //创建DeviceAlarmImage元素
    QDomElement DeviceAlarmImage = AckDom.createElement("DeviceAlarmImage");

    DeviceAlarmImage.setAttribute("id", QString::number(id));
    DeviceAlarmImage.setAttribute("status", "");
    DeviceAlarmImage.setAttribute("msg", "");

    if (id == 0) {//左防区图像
        if (MainStream::TriggerTimeBuffer.size() > 0) {
            if (MainStream::TriggerTimeBuffer.size() < index) {
                DeviceAlarmImage.setAttribute("triggertime", MainStream::TriggerTimeBuffer.at(0));
            } else {
                DeviceAlarmImage.setAttribute("triggertime", MainStream::TriggerTimeBuffer.at(index));
            }
        } else {
            DeviceAlarmImage.setAttribute("triggertime", QString(""));
        }

        QString MainImageBase64;
        if (MainStream::MainImageBuffer.size() > 0) {
            if (MainStream::MainImageBuffer.size() < index) {
                MainImageBase64 = MainStream::MainImageBuffer.at(0);
            } else {
                MainImageBase64 = MainStream::MainImageBuffer.at(index);
            }
        } else {
            MainImageBase64 = QString("");
        }

        QDomText mainStreamText = AckDom.createTextNode(MainImageBase64);
        DeviceAlarmImage.appendChild(mainStreamText);
    } else if (id == 1) {//右防区图像
        if (SubStream::TriggerTimeBuffer.size() > 0) {
            if (SubStream::TriggerTimeBuffer.size() < index) {
                DeviceAlarmImage.setAttribute("triggertime", SubStream::TriggerTimeBuffer.at(0));
            } else {
                DeviceAlarmImage.setAttribute("triggertime", SubStream::TriggerTimeBuffer.at(index));
            }
        } else {
            DeviceAlarmImage.setAttribute("triggertime", QString(""));
        }

        QString SubImageBase64;
        if (SubStream::SubImageBuffer.size() > 0) {
            if (SubStream::SubImageBuffer.size() < index) {
                SubImageBase64 = SubStream::SubImageBuffer.at(0);
            } else {
                SubImageBase64 = SubStream::SubImageBuffer.at(index);
            }
        } else {
            SubImageBase64 = QString("");
        }

        QDomText subStreamText = AckDom.createTextNode(SubImageBase64);
        DeviceAlarmImage.appendChild(subStreamText);
    }

    RootElement.appendChild(DeviceAlarmImage);

    QTextStream Out(&MessageMerge);
    AckDom.save(Out,4);

    int length = MessageMerge.size();
    MessageMerge = QString("IALARM:") + QString("%1").arg(length,13,10,QLatin1Char('0')) + MessageMerge;

    GlobalConfig::SendAlarmMsgToAlarmHostBuffer.append(MessageMerge.toAscii());
}
