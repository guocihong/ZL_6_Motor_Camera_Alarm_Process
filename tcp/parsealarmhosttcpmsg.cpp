#include "parsealarmhosttcpmsg.h"
#include "CommonSetting.h"
#include "camera/mainstream.h"
#include "camera/substream.h"

ParseAlarmHostTcpMsg *ParseAlarmHostTcpMsg::instance = NULL;

ParseAlarmHostTcpMsg::ParseAlarmHostTcpMsg(QObject *parent) :
    QObject(parent)
{
}

void ParseAlarmHostTcpMsg::Parse(void)
{
    ParseMsgFromAlarmHostTimer = new QTimer(this);
    ParseMsgFromAlarmHostTimer->setInterval(200);
    connect(ParseMsgFromAlarmHostTimer,SIGNAL(timeout()),this,SLOT(slotParseMsgFromAlarmHost()));
    ParseMsgFromAlarmHostTimer->start();

    UpdateDateTimeTimer = new QTimer(this);
    UpdateDateTimeTimer->setInterval(30 * 1000 * 60);
    connect(UpdateDateTimeTimer,SIGNAL(timeout()),this,SLOT(slotUpdateDateTime()));
    UpdateDateTimeTimer->start();
}

void ParseAlarmHostTcpMsg::slotParseMsgFromAlarmHost()
{
    foreach (TcpHelper *tcpHelper, GlobalConfig::TcpHelperBuffer) {
        if (tcpHelper->RecvVaildCompleteMsgBuffer.size() == 0) {
            continue;
        }

        while(tcpHelper->RecvVaildCompleteMsgBuffer.size() > 0) {
            QByteArray data = tcpHelper->RecvVaildCompleteMsgBuffer.takeFirst();

            qDebug() << data;

            QDomDocument dom;
            QString errorMsg;
            int errorLine, errorColumn;

            bool isReturnOK = false;//是否需要发送ok信息给报警主机
            bool isUpdateNetworkConfig = false;//是否需要更新网络配置
            bool isReturnConfigInfo = false;//是否把本设备的配置信息返回给报警主机
            bool isReturnDoubleAreaPic = false;//是否返回双防区实时图像
            bool isReturnLeftAreaPic = false;//是否返回左防区实时图像
            bool isReturnRightAreaPic = false;//是否返回右防区实时图像
            bool isSystemReboot = false;

            if (!dom.setContent(data, &errorMsg, &errorLine, &errorColumn)) {
                qDebug() << QString("tcp xml msg parse error from alarm host:") + errorMsg;
                continue;
            }

            QDomElement RootElement = dom.documentElement();//获取根元素
            if (RootElement.tagName() == "Server") {//根元素名称
                if (RootElement.hasAttribute("TargetIP")) {//判断根元素是否有这个属性
                    //获得这个属性对应的值
                    QString TargetIP = RootElement.attributeNode("TargetIP").value();

                    if (TargetIP == GlobalConfig::LocalHostIP) {
                        if (RootElement.hasAttribute("NowTime")) {//判断根元素是否有这个属性
                            //获得这个属性对应的值
                            NowTime = RootElement.attributeNode("NowTime").value();

                            static bool isUpdateDataTime = true;
                            if (isUpdateDataTime) {
                                CommonSetting::SettingSystemDateTime(NowTime);
                                isUpdateDataTime = false;
                            }
                        }

                        QDomNode firstChildNode = RootElement.firstChild();//第一个子节点
                        while (!firstChildNode.isNull()) {
                            //1、报警主机启动/暂停服务
                            if (firstChildNode.nodeName() == "StartServices") {
                                //启动视频采集
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

                                //切换工作模式为tcp模式
                                GlobalConfig::system_mode = GlobalConfig::TcpMode;

                                isReturnOK = true;
                            }

                            if (firstChildNode.nodeName() == "StopServices") {
                                //停止视频采集
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

                                //切换工作模式为RS485模式
                                GlobalConfig::RecvVaildCompletePackageFromAlarmHostBuffer.clear();
                                GlobalConfig::SendMsgToAlarmHostBuffer.clear();
                                GlobalConfig::system_mode = GlobalConfig::RS485Mode;

                                isReturnOK = true;
                            }

                            //2、报警主机下发网络配置信息
                            if (firstChildNode.nodeName() == "ServerIP") {
                                QString temp = firstChildNode.toElement().text();

                                if (!temp.isEmpty()) {
                                    GlobalConfig::ServerIP = firstChildNode.toElement().text();
                                    CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                                                 "AppGlobalConfig/ServerIP",
                                                                 GlobalConfig::ServerIP);
                                }

                                isReturnOK = true;
                            }

                            if (firstChildNode.nodeName() == "ServerPort") {
                                QString temp = firstChildNode.toElement().text();

                                if (!temp.isEmpty()) {
                                    GlobalConfig::ServerPort = firstChildNode.toElement().text().toUShort();
                                    CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                                                 "AppGlobalConfig/ServerPort",
                                                                 QString::number(GlobalConfig::ServerPort));
                                }

                                isReturnOK = true;
                            }

                            if (firstChildNode.nodeName() == "UseMainCamera") {
                                QString temp = firstChildNode.toElement().text();

                                if (!temp.isEmpty()) {
                                    quint8 flag = firstChildNode.toElement().text().toUShort();
                                    if (flag != GlobalConfig::UseMainCamera) {
                                        isSystemReboot = true;
                                    }

                                    GlobalConfig::UseMainCamera = flag;
                                    CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                                                 "AppGlobalConfig/UseMainCamera",
                                                                 QString::number(GlobalConfig::UseMainCamera));
                                }

                                isReturnOK = true;
                            }

                            if (firstChildNode.nodeName() == "MainDefenceID") {
                                QString temp = firstChildNode.toElement().text();

                                if (!temp.isEmpty()) {
                                    GlobalConfig::MainDefenceID = firstChildNode.toElement().text();
                                    CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                                                 "AppGlobalConfig/MainDefenceID",
                                                                 GlobalConfig::MainDefenceID);
                                }

                                isReturnOK = true;
                            }

                            if (firstChildNode.nodeName() == "MainCameraSleepTime") {
                                QString temp = firstChildNode.toElement().text();

                                if (!temp.isEmpty()) {
                                    GlobalConfig::MainCameraSleepTime =
                                            firstChildNode.toElement().text().toUShort();
                                    CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                                                 "AppGlobalConfig/MainCameraSleepTime",
                                                                 QString::number(GlobalConfig::MainCameraSleepTime));
                                }

                                isReturnOK = true;
                            }

                            if (firstChildNode.nodeName() == "UseSubCamera") {
                                QString temp = firstChildNode.toElement().text();

                                if (!temp.isEmpty()) {
                                    quint8 flag = firstChildNode.toElement().text().toUShort();
                                    if (flag != GlobalConfig::UseSubCamera) {
                                        isSystemReboot = true;
                                    }

                                    GlobalConfig::UseSubCamera = flag;
                                    CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                                                 "AppGlobalConfig/UseSubCamera",
                                                                 QString::number(GlobalConfig::UseSubCamera));
                                }

                                isReturnOK = true;
                            }

                            if (firstChildNode.nodeName() == "SubDefenceID") {
                                QString temp = firstChildNode.toElement().text();

                                if (!temp.isEmpty()) {
                                    GlobalConfig::SubDefenceID =
                                            firstChildNode.toElement().text();
                                    CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                                                 "AppGlobalConfig/SubDefenceID",
                                                                 GlobalConfig::SubDefenceID);
                                }

                                isReturnOK = true;
                            }

                            if (firstChildNode.nodeName() == "SubCameraSleepTime") {
                                QString temp = firstChildNode.toElement().text();

                                if (!temp.isEmpty()) {
                                    GlobalConfig::SubCameraSleepTime =
                                            firstChildNode.toElement().text().toUShort();
                                    CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                                                 "AppGlobalConfig/SubCameraSleepTime",
                                                                 QString::number(GlobalConfig::SubCameraSleepTime));
                                }

                                isReturnOK = true;
                            }

                            if (firstChildNode.nodeName() == "DeviceMacAddr") {
                                QString temp = firstChildNode.toElement().text();

                                if (!temp.isEmpty()) {
                                    GlobalConfig::DeviceMacAddr =
                                            firstChildNode.toElement().text();
                                    CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                                                 "AppGlobalConfig/DeviceMacAddr",
                                                                 GlobalConfig::DeviceMacAddr);
                                }

                                isReturnOK = true;
                                isUpdateNetworkConfig = true;
                            }

                            if (firstChildNode.nodeName() == "DeviceIPAddrPrefix") {
                                QString temp = firstChildNode.toElement().text();

                                if (!temp.isEmpty()) {
                                    GlobalConfig::DeviceIPAddrPrefix =
                                            firstChildNode.toElement().text();
                                    CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                                                 "AppGlobalConfig/DeviceIPAddrPrefix",
                                                                 GlobalConfig::DeviceIPAddrPrefix);
                                }

                                isReturnOK = true;
                                isUpdateNetworkConfig = true;
                            }

                            //3、报警主机获取网络配置信息
                            if (firstChildNode.nodeName() == "GetDeviceConfig") {
                                isReturnConfigInfo = true;
                            }

                            //4、报警主机获取前端控制杆左右防区实时图像
                            if (firstChildNode.nodeName() == "GetBasicPic") {
                                isReturnDoubleAreaPic = true;
                            }

                            //5、报警主机获取前端控制杆左防区实时图像
                            if (firstChildNode.nodeName() == "GetMainStream") {
                                isReturnLeftAreaPic = true;
                            }

                            //6、报警主机获取前端控制杆右防区实时图像
                            if (firstChildNode.nodeName() == "GetSubStream") {
                                isReturnRightAreaPic = true;
                            }

                            //7、获取电机张力控制杆的相关信息
                            if (firstChildNode.nodeName() == "SendDeviceData") {
                                QString cmd = firstChildNode.toElement().text();
                                ParseMotorControlCmdFromAlarmHost(cmd,tcpHelper);
                            }

                            //8、报警主机重启前端设备
                            if (firstChildNode.nodeName() == "SystemReboot") {
                                isReturnOK = true;
                                isSystemReboot = true;
                            }
                            firstChildNode = firstChildNode.nextSibling();//下一个节点
                        }

                        if (isReturnOK) {
                            SendOKAckToAlarmHost(tcpHelper);
                        }

                        if (isUpdateNetworkConfig) {
                            CommonSetting::Sleep(300);//等待ok信息发送完成
                            GlobalConfig::UpdateIPAddr();
                        }

                        if (isReturnConfigInfo) {
                            SendConfigInfoToAlarmHost(tcpHelper);
                        }

                        if (isReturnDoubleAreaPic) {
                            SendDoubleImageToAlarmHost(tcpHelper);
                        }

                        if (isReturnLeftAreaPic) {
                            SendLeftAreaImageToAlarmHost(tcpHelper);
                        }

                        if (isReturnRightAreaPic) {
                            SendRightAreaImageToAlarmHost(tcpHelper);
                        }

                        if (isSystemReboot) {
                            CommonSetting::Sleep(300);//等待ok信息发送完成
                            system("reboot");
                        }
                    }
                }
            }
        }
    }
}

void ParseAlarmHostTcpMsg::SendOKAckToAlarmHost(TcpHelper *tcpHelper)
{
    QString MessageMerge;
    QDomDocument AckDom;

    //xml声明
    QString XmlHeader("version=\"1.0\" encoding=\"UTF-8\"");
    AckDom.appendChild(AckDom.createProcessingInstruction("xml", XmlHeader));

    //创建根元素
    QDomElement RootElement = AckDom.createElement("Device");

    RootElement.setAttribute("DeviceIP", GlobalConfig::LocalHostIP);
    RootElement.setAttribute("DefenceID", GlobalConfig::MainDefenceID + "|" + GlobalConfig::SubDefenceID);

    QDomText RootElementText = AckDom.createTextNode("OK");
    RootElement.appendChild(RootElementText);

    AckDom.appendChild(RootElement);

    QTextStream Out(&MessageMerge);
    AckDom.save(Out,4);

    int length = MessageMerge.size();
    MessageMerge = QString("IALARM:") + QString("%1").arg(length,13,10,QLatin1Char('0')) + MessageMerge;

    tcpHelper->SendMsgBuffer.append(MessageMerge.toAscii());
}

void ParseAlarmHostTcpMsg::SendConfigInfoToAlarmHost(TcpHelper *tcpHelper)
{
    QString MessageMerge;
    QDomDocument AckDom;

    //xml声明
    QString XmlHeader("version=\"1.0\" encoding=\"UTF-8\"");
    AckDom.appendChild(AckDom.createProcessingInstruction("xml", XmlHeader));

    //创建根元素
    QDomElement RootElement = AckDom.createElement("Device");

    RootElement.setAttribute("DeviceIP",GlobalConfig::LocalHostIP);
    RootElement.setAttribute("DefenceID",GlobalConfig::MainDefenceID + "|" + GlobalConfig::SubDefenceID);
    AckDom.appendChild(RootElement);

    //创建ServerIP元素
    QDomElement ServerIP = AckDom.createElement("ServerIP");
    QDomText ServerIPText = AckDom.createTextNode(GlobalConfig::ServerIP);
    ServerIP.appendChild(ServerIPText);
    RootElement.appendChild(ServerIP);

    //创建ServerPort元素
    QDomElement ServerPort = AckDom.createElement("ServerPort");
    QDomText ServerPortText =
            AckDom.createTextNode(QString::number(GlobalConfig::ServerPort));
    ServerPort.appendChild(ServerPortText);
    RootElement.appendChild(ServerPort);

    //创建UseMainCamera元素
    QDomElement UseMainCamera = AckDom.createElement("UseMainCamera");
    QDomText UseMainCameraText =
            AckDom.createTextNode(QString::number(GlobalConfig::UseMainCamera));
    UseMainCamera.appendChild(UseMainCameraText);
    RootElement.appendChild(UseMainCamera);

    //创建MainDefenceID元素
    QDomElement MainDefenceID = AckDom.createElement("MainDefenceID");
    QDomText MainDefenceIDText =
            AckDom.createTextNode(GlobalConfig::MainDefenceID);
    MainDefenceID.appendChild(MainDefenceIDText);
    RootElement.appendChild(MainDefenceID);

    //创建MainCameraSleepTime元素
    QDomElement MainCameraSleepTime = AckDom.createElement("MainCameraSleepTime");
    QDomText MainCameraSleepTimeText =
            AckDom.createTextNode(QString::number(GlobalConfig::MainCameraSleepTime));
    MainCameraSleepTime.appendChild(MainCameraSleepTimeText);
    RootElement.appendChild(MainCameraSleepTime);

    //创建UseSubCamera元素
    QDomElement UseSubCamera = AckDom.createElement("UseSubCamera");
    QDomText UseSubCameraText =
            AckDom.createTextNode(QString::number(GlobalConfig::UseSubCamera));
    UseSubCamera.appendChild(UseSubCameraText);
    RootElement.appendChild(UseSubCamera);

    //创建SubDefenceID元素
    QDomElement SubDefenceID = AckDom.createElement("SubDefenceID");
    QDomText SubDefenceIDText =
            AckDom.createTextNode(GlobalConfig::SubDefenceID);
    SubDefenceID.appendChild(SubDefenceIDText);
    RootElement.appendChild(SubDefenceID);

    //创建SubCameraSleepTime元素
    QDomElement SubCameraSleepTime = AckDom.createElement("SubCameraSleepTime");
    QDomText SubCameraSleepTimeText =
            AckDom.createTextNode(QString::number(GlobalConfig::SubCameraSleepTime));
    SubCameraSleepTime.appendChild(SubCameraSleepTimeText);
    RootElement.appendChild(SubCameraSleepTime);

    //创建DeviceMacAddr元素
    QDomElement DeviceMacAddr = AckDom.createElement("DeviceMacAddr");
    QDomText DeviceMacAddrText =
            AckDom.createTextNode(GlobalConfig::DeviceMacAddr);
    DeviceMacAddr.appendChild(DeviceMacAddrText);
    RootElement.appendChild(DeviceMacAddr);

    //创建DeviceIPAddrPrefix元素
    QDomElement DeviceIPAddrPrefix = AckDom.createElement("DeviceIPAddrPrefix");
    QDomText DeviceIPAddrPrefixText =
            AckDom.createTextNode(GlobalConfig::DeviceIPAddrPrefix);
    DeviceIPAddrPrefix.appendChild(DeviceIPAddrPrefixText);
    RootElement.appendChild(DeviceIPAddrPrefix);

    QTextStream Out(&MessageMerge);
    AckDom.save(Out,4);

    int length = MessageMerge.size();
    MessageMerge = QString("IALARM:") + QString("%1").arg(length,13,10,QLatin1Char('0')) +
            MessageMerge;

    tcpHelper->SendMsgBuffer.append(MessageMerge.toAscii());
}

void ParseAlarmHostTcpMsg::SendDoubleImageToAlarmHost(TcpHelper *tcpHelper)
{
    SaveMotorLastestStatusInfo();

    QString MessageMerge;
    QDomDocument AckDom;

    //xml声明
    QString XmlHeader("version=\"1.0\" encoding=\"UTF-8\"");
    AckDom.appendChild(AckDom.createProcessingInstruction("xml", XmlHeader));

    //创建根元素
    QDomElement RootElement = AckDom.createElement("Device");

    RootElement.setAttribute("DeviceIP", GlobalConfig::LocalHostIP);
    RootElement.setAttribute("DefenceID", GlobalConfig::MainDefenceID + "|" + GlobalConfig::SubDefenceID);
    AckDom.appendChild(RootElement);

    //创建MainStream元素
    QDomElement mainStream = AckDom.createElement("MainStream");
    mainStream.setAttribute("status", GlobalConfig::status);

    QString MainImageBase64;
    if (MainStream::MainImageBuffer.size() > 0) {
        QImage MainImageRgb888;

        if (MainStream::MainImageBuffer.size() == 1) {
            MainImageRgb888 = MainStream::MainImageBuffer.at(0);
        } else {
            MainImageRgb888 = MainStream::MainImageBuffer.takeFirst();
        }

        QByteArray tempData;
        QBuffer tempBuffer(&tempData);
        MainImageRgb888.save(&tempBuffer,"JPG");//按照JPG解码保存数据
        MainImageBase64 = QString(tempData.toBase64());
    } else {
        MainImageBase64 = QString("");
    }

    QDomText mainStreamText = AckDom.createTextNode(MainImageBase64);
    mainStream.appendChild(mainStreamText);
    RootElement.appendChild(mainStream);

    //创建SubStream元素
    QDomElement subStream = AckDom.createElement("SubStream");
    subStream.setAttribute("status", GlobalConfig::status);

    QString SubImageBase64;
    if (SubStream::SubImageBuffer.size() > 0) {
        QImage SubImageRgb888;

        if (SubStream::SubImageBuffer.size() == 1) {
            SubImageRgb888 = SubStream::SubImageBuffer.at(0);
        } else {
            SubImageRgb888 = SubStream::SubImageBuffer.takeFirst();
        }

        QByteArray tempData;
        QBuffer tempBuffer(&tempData);
        SubImageRgb888.save(&tempBuffer,"JPG");//按照JPG解码保存数据
        SubImageBase64 = QString(tempData.toBase64());
    } else {
        SubImageBase64 = QString("");
    }

    QDomText subStreamText = AckDom.createTextNode(SubImageBase64);
    subStream.appendChild(subStreamText);
    RootElement.appendChild(subStream);

    QTextStream Out(&MessageMerge);
    AckDom.save(Out,4);

    int length = MessageMerge.size();
    MessageMerge = QString("IALARM:") + QString("%1").arg(length,13,10,QLatin1Char('0')) + MessageMerge;

    tcpHelper->SendMsgBuffer.append(MessageMerge.toAscii());
}

void ParseAlarmHostTcpMsg::SendLeftAreaImageToAlarmHost(TcpHelper *tcpHelper)
{
    SaveMotorLastestStatusInfo();

    QString MessageMerge;
    QDomDocument AckDom;

    //xml声明
    QString XmlHeader("version=\"1.0\" encoding=\"UTF-8\"");
    AckDom.appendChild(AckDom.createProcessingInstruction("xml", XmlHeader));

    //创建根元素
    QDomElement RootElement = AckDom.createElement("Device");

    RootElement.setAttribute("DeviceIP", GlobalConfig::LocalHostIP);
    RootElement.setAttribute("DefenceID", GlobalConfig::MainDefenceID);
    AckDom.appendChild(RootElement);

    //创建MainStream元素
    QDomElement mainStream = AckDom.createElement("MainStream");
    mainStream.setAttribute("status", GlobalConfig::status);

    QString MainImageBase64;
    if (MainStream::MainImageBuffer.size() > 0) {
        QImage MainImageRgb888;

        if (MainStream::MainImageBuffer.size() == 1) {
            MainImageRgb888 = MainStream::MainImageBuffer.at(0);
        } else {
            MainImageRgb888 = MainStream::MainImageBuffer.takeFirst();
        }

        QByteArray tempData;
        QBuffer tempBuffer(&tempData);
        MainImageRgb888.save(&tempBuffer,"JPG");//按照JPG解码保存数据
        MainImageBase64 = QString(tempData.toBase64());
    } else {
        MainImageBase64 = QString("");
    }

    QDomText mainStreamText = AckDom.createTextNode(MainImageBase64);
    mainStream.appendChild(mainStreamText);
    RootElement.appendChild(mainStream);

    QTextStream Out(&MessageMerge);
    AckDom.save(Out,4);

    int length = MessageMerge.size();
    MessageMerge = QString("IALARM:") + QString("%1").arg(length,13,10,QLatin1Char('0')) + MessageMerge;

    tcpHelper->SendMsgBuffer.append(MessageMerge.toAscii());
}

void ParseAlarmHostTcpMsg::SendRightAreaImageToAlarmHost(TcpHelper *tcpHelper)
{
    SaveMotorLastestStatusInfo();

    QString MessageMerge;
    QDomDocument AckDom;

    //xml声明
    QString XmlHeader("version=\"1.0\" encoding=\"UTF-8\"");
    AckDom.appendChild(AckDom.createProcessingInstruction("xml", XmlHeader));

    //创建根元素
    QDomElement RootElement = AckDom.createElement("Device");

    RootElement.setAttribute("DeviceIP", GlobalConfig::LocalHostIP);
    RootElement.setAttribute("DefenceID", GlobalConfig::SubDefenceID);
    AckDom.appendChild(RootElement);

    //创建SubStream元素
    QDomElement subStream = AckDom.createElement("SubStream");
    subStream.setAttribute("status", GlobalConfig::status);

    QString SubImageBase64;
    if (SubStream::SubImageBuffer.size() > 0) {
        QImage SubImageRgb888;

        if (SubStream::SubImageBuffer.size() == 1) {
            SubImageRgb888 = SubStream::SubImageBuffer.at(0);
        } else {
            SubImageRgb888 = SubStream::SubImageBuffer.takeFirst();
        }

        QByteArray tempData;
        QBuffer tempBuffer(&tempData);
        SubImageRgb888.save(&tempBuffer,"JPG");//按照JPG解码保存数据
        SubImageBase64 = QString(tempData.toBase64());
    } else {
        SubImageBase64 = QString("");
    }

    QDomText subStreamText = AckDom.createTextNode(SubImageBase64);
    subStream.appendChild(subStreamText);
    RootElement.appendChild(subStream);

    QTextStream Out(&MessageMerge);
    AckDom.save(Out,4);

    int length = MessageMerge.size();
    MessageMerge = QString("IALARM:") + QString("%1").arg(length,13,10,QLatin1Char('0')) + MessageMerge;

    tcpHelper->SendMsgBuffer.append(MessageMerge.toAscii());
}

void ParseAlarmHostTcpMsg::ParseMotorControlCmdFromAlarmHost(QString Msg, TcpHelper *tcpHelper)
{
    bool ok = true;
    QByteArray data = QByteArray();
    QStringList cmd = Msg.trimmed().simplified().toUpper().split(" ");

    if (cmd[4] == "E0") {//询问状态
        //返回1：16 01 设备地址 02 F0 01/02/03 校验和
        //返回2：16 01 设备地址 01 00 校验和（该地址模块所有防区正常）
        if (GlobalConfig::ip_addr == CMD_ADDR_UNSOLV) {//本设备无有效地址,只回参数应答
            data[0] = 1;
            data[1] = 0xF1;
        } else {//有有效地址,回防区状态
            if ((GlobalConfig::alarm_out_flag & 0x38) == 0x00) {//2个防区均无报警
                data[0] = 1;
                data[1] = 0x00;
            } else {//有报警
                data[0] = 2;
                data[1] = 0xF0;
                data[2] = (GlobalConfig::alarm_out_flag & 0x38) >> 3;
            }
        }
        CommonCode(data,tcpHelper);
    } else if (cmd[4] == "E1") {//询问地址
        //返回：16 01 设备地址 01 F1 校验和
        data[0] = 0x01;
        data[1] = 0xF1;
        CommonCode(data,tcpHelper);
    } else if (cmd[4] == "E3") {//设置设备返回延时时间
        //更新变量
        GlobalConfig::gl_reply_tick = cmd[5].toUShort(&ok,16);

        //保存到配置文件
        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                     "AppGlobalConfig/gl_reply_tick",
                                     QString::number(GlobalConfig::gl_reply_tick));
    } else if (cmd[4] == "E4") {//读取设备返回延时时间
        //返回：16 01 设备地址 02 F4 延时时间 校验和
        data[0] = 0x02;
        data[1] = 0xF4;
        data[2] = GlobalConfig::gl_reply_tick;
        CommonCode(data,tcpHelper);
    } else if (cmd[4] == "E8") {
        if (cmd[6] == "10") {//读配置信息
            //返回：16 01 设备地址 1E E8 1C 08 张力mask左   张力mask右
            //静态张力允许下限高/低字节  静态张力允许上限高/低字节 报警阀值下浮比例
            //报警阀值允许上浮左1~8 报警阀值允许上浮右1~8	双/单防区(0-双  1-单)
            //码地址 声光报警时间  联动输出时间    校验和
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
            data[30] = 0;
            CommonCode(data,tcpHelper);
        } else if (cmd[6] == "12") {//读报警信息
            //返回：16 01 设备地址 0A E8 08 1A  [张力掩码左] [张力掩码右] [外力报警左]  [外力报警右]
            //[静态张力报警左] [静态张力报警右] [门磁]  校验和
            data[0] = 0x0A;
            data[1] = 0xE8;
            data[2] = 0x08;
            data[3] = 0x1A;

            data[4] = HIGH(GlobalConfig::ad_sensor_mask_LR);//张力掩码左
            data[5] = LOW(GlobalConfig::ad_sensor_mask_LR); //张力掩码右

            data[6] = HIGH(GlobalConfig::ad_alarm_exts);    //外力报警左
            data[7] = LOW(GlobalConfig::ad_alarm_exts);     //外力报警右

            data[8] = HIGH(GlobalConfig::ad_alarm_base);    //静态张力报警左
            data[9] = LOW(GlobalConfig::ad_alarm_base);     //静态张力报警右

            data[10] = GlobalConfig::doorkeep_state;        //门磁
            CommonCode(data,tcpHelper);
        } else if (cmd[6] == "14") {//读瞬间张力
            //返回：16 01 设备地址 23 E8 21 1C 左1~8 右1~8 校验和
            data[0] = 0x23;
            data[1] = 0xE8;
            data[2] = 0x21;
            data[3] = 0x1C;

            for (int i = 0; i < 6; i++) {//左1~6的瞬间张力
                data[4 + (i << 1)] = HIGH(GlobalConfig::ad_chn_sample[i]);
                data[5 + (i << 1)] = LOW(GlobalConfig::ad_chn_sample[i]);
            }

            //左开关量
            data[16] = 0;
            data[17] = 0;

            //杆自身
            data[18] = HIGH(GlobalConfig::ad_chn_sample[12]);
            data[19] = LOW(GlobalConfig::ad_chn_sample[12]);


            for (int i = 0; i < 6; i++) {//右1~6的瞬间张力
                data[20 + (i << 1)] = HIGH(GlobalConfig::ad_chn_sample[6 + i]);
                data[21 + (i << 1)] = LOW(GlobalConfig::ad_chn_sample[6 + i]);
            }

            //右开关量
            data[32] = 0;
            data[33] = 0;

            //杆自身
            data[34] = HIGH(GlobalConfig::ad_chn_sample[12]);
            data[35] = LOW(GlobalConfig::ad_chn_sample[12]);

            CommonCode(data,tcpHelper);
        } else if (cmd[6] == "15") {//读静态张力基准
            //返回：16 01 设备地址 23 E8 21 1D 左1~8 右1~8 校验和
            data[0] = 0x23;
            data[1] = 0xE8;
            data[2] = 0x21;
            data[3] = 0x1D;

            for (int i = 0; i < 6; i++) {//左1~6的瞬间张力
                data[4 + (i << 1)] = HIGH(GlobalConfig::ad_chn_base[i].base);
                data[5 + (i << 1)] = LOW(GlobalConfig::ad_chn_base[i].base);
            }

            //左开关量
            data[16] = 0;
            data[17] = 0;

            //杆自身
            data[18] = HIGH(GlobalConfig::ad_chn_base[12].base);
            data[19] = LOW(GlobalConfig::ad_chn_base[12].base);


            for (int i = 0; i < 6; i++) {//右1~6的瞬间张力
                data[20 + (i << 1)] = HIGH(GlobalConfig::ad_chn_base[6 + i].base);
                data[21 + (i << 1)] = LOW(GlobalConfig::ad_chn_base[6 + i].base);
            }

            //右开关量
            data[32] = 0;
            data[33] = 0;

            //杆自身
            data[34] = HIGH(GlobalConfig::ad_chn_base[12].base);
            data[35] = LOW(GlobalConfig::ad_chn_base[12].base);

            CommonCode(data,tcpHelper);
        } else if (cmd[6] == "40") {//设置静态张力值范围
            //更新变量
            GlobalConfig::ad_still_dn = (cmd[7].toUShort(&ok,16) << 8) + cmd[8].toUShort(&ok,16);//下限
            GlobalConfig::ad_still_up = (cmd[9].toUShort(&ok,16) << 8) + cmd[10].toUShort(&ok,16);//上限

            //保存到配置文件
            CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                         "AppGlobalConfig/ad_still_dn",
                                         QString::number(GlobalConfig::ad_still_dn));

            CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                         "AppGlobalConfig/ad_still_up",
                                         QString::number(GlobalConfig::ad_still_up));
        } else if (cmd[6] == "50") {//设置报警阀值
            //更新变量
            GlobalConfig::ad_still_Dup[0] = cmd[8].toUShort(&ok,16);  //左1
            GlobalConfig::ad_still_Dup[1] = cmd[9].toUShort(&ok,16);  //左2
            GlobalConfig::ad_still_Dup[2] = cmd[10].toUShort(&ok,16); //左3
            GlobalConfig::ad_still_Dup[3] = cmd[11].toUShort(&ok,16); //左4
            GlobalConfig::ad_still_Dup[4] = cmd[12].toUShort(&ok,16); //左5
            GlobalConfig::ad_still_Dup[5] = cmd[13].toUShort(&ok,16); //左6

            //左开关量
            //杆自身
            GlobalConfig::ad_still_Dup[6] = cmd[16].toUShort(&ok,16); //右1
            GlobalConfig::ad_still_Dup[7] = cmd[17].toUShort(&ok,16); //右2
            GlobalConfig::ad_still_Dup[8] = cmd[18].toUShort(&ok,16); //右3
            GlobalConfig::ad_still_Dup[9] = cmd[19].toUShort(&ok,16); //右4
            GlobalConfig::ad_still_Dup[10] = cmd[20].toUShort(&ok,16);//右5
            GlobalConfig::ad_still_Dup[11] = cmd[21].toUShort(&ok,16);//右6

            //右开关量

            //杆自身
            GlobalConfig::ad_still_Dup[12] = cmd[23].toUShort(&ok,16);//杆自身


            //保存到配置文件
            QStringList DupList = QStringList();
            for (int i = 0; i < 13; i++) {
                DupList << QString::number(GlobalConfig::ad_still_Dup[i]);
            }
            CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                         "AppGlobalConfig/ad_still_Dup",
                                         DupList.join("|"));
        } else if (cmd[6] == "60") {//设置声光报警时间
            //更新变量
            GlobalConfig::beep_during_temp = cmd[7].toUShort(&ok,16) * 1000 / SCHEDULER_TICK;

            //保存到配置文件
            CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                         "AppGlobalConfig/beep_during_temp",
                                         QString::number(GlobalConfig::beep_during_temp * SCHEDULER_TICK / 1000));
        } else if (cmd[6] == "70") {//设置联动时间
            //do nothing
        } else if (cmd[6] == "F0") {//张力杆电机控制(主机->设备)
            QByteArray MotorRunCmd;
            MotorRunCmd[0] = 0x16;
            MotorRunCmd[1] = 0xFF;
            MotorRunCmd[2] = 0x01;
            MotorRunCmd[3] = 0x06;
            MotorRunCmd[4] = 0xE8;
            MotorRunCmd[5] = 0x04;
            MotorRunCmd[6] = 0xF0;
            MotorRunCmd[7] = cmd[7].toUShort(&ok,16);//电机号
            MotorRunCmd[8] = cmd[8].toUShort(&ok,16);//正转、反转、停止
            MotorRunCmd[9] = cmd[9].toUShort(&ok,16);//运转时间

            //计算校验和
            quint8 check_parity_sum = 0;
            for (int i = 1; i < 10; i++) {
                check_parity_sum += MotorRunCmd[i];
            }

            MotorRunCmd[10] = check_parity_sum; //校验和

            //发送给电机控制杆
            GlobalConfig::SendMsgToMotorControlBuffer.append(MotorRunCmd);

            //这里必须，不然手动松紧钢丝时，静态基准和瞬间张力不能同步更新
            GlobalConfig::gl_chnn_index = cmd[7].toUShort(&ok,16);//电机号

            //这几个变量只有进入实时检测阶段才会用到
            if (GlobalConfig::system_status == GlobalConfig::SYS_CHECK) {
                GlobalConfig::isEnterAutoAdjustMotorMode = true;
                GlobalConfig::adjust_status = 2;

                GlobalConfig::isEnterManualAdjustMotorMode = true;
            }
        } else if (cmd[6] == "F1") {//采样值清零---->消除电路上的误差
            QByteArray MotorRunCmd;
            MotorRunCmd[0] = 0x16;
            MotorRunCmd[1] = 0xFF;
            MotorRunCmd[2] = 0x01;
            MotorRunCmd[3] = 0x23;
            MotorRunCmd[4] = 0xE8;
            MotorRunCmd[5] = 0x21;
            MotorRunCmd[6] = 0xF1;

            //左1~8、右1~8
            for (int i = 0; i < 32; i++) {
                MotorRunCmd[7 + i] = cmd[7 + i].toUShort(&ok,16);
            }

            //计算校验和
            quint8 check_parity_sum = 0;
            for (int i = 1; i < 39; i++) {
                check_parity_sum += MotorRunCmd[i];
            }

            MotorRunCmd[39] = check_parity_sum; //校验和

            //发送给电机控制杆
            GlobalConfig::SendMsgToMotorControlBuffer.append(MotorRunCmd);

            /******************************************************************/
            //1、在加级联的情况下，需要在现场通过报警主机先进行采样值恢复，然后在执行采样值清零
            //2、生产车间进行采样值恢复和采样值清零都是通过广播模式设置
            //3、现场通过报警主机进行采样值恢复和采样值清零是通过单播模式设置

            //只有报警主机发出的采样值清零命令才生效
            if (cmd[1].toUShort(&ok,16) == GlobalConfig::ip_addr) {//单播模式
                quint16 sample_value_sum = 0;
                for (int i = 0; i < 32; i++) {
                    sample_value_sum += cmd[7 + i].toUShort(&ok,16);
                }

                if (sample_value_sum == 0) {//本条命令是进行采样值恢复
                    GlobalConfig::is_sample_clear = 0;

                    CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                                 "AppGlobalConfig/is_sample_clear",
                                                 QString::number(GlobalConfig::is_sample_clear));
                } else {//本条命令是进行采样值清零
                    GlobalConfig::check_sample_clear_tick = 3000 / SCHEDULER_TICK;
                }
            } else if (cmd[1].toUShort(&ok,16) == CMD_ADDR_BC) {//广播模式
                //生产车间发出的采样值清零命令不生效
                //什么都不做
            }

            /******************************************************************/
        } else if (cmd[6] == "F3") {//读钢丝是否剪断以及是否处于调整钢丝模式
            //返回：16 01 设备地址 06 E8 04 1E [钢丝剪断左] [钢丝剪断右] [是否处于调整钢丝模式] 校验和
            data[0] = 0x06;
            data[1] = 0xE8;
            data[2] = 0x04;
            data[3] = 0x1E;
            data[4] = HIGH(GlobalConfig::ad_chnn_wire_cut);
            data[5] = LOW(GlobalConfig::ad_chnn_wire_cut);
            data[6] = GlobalConfig::gl_motor_adjust_flag;
            CommonCode(data,tcpHelper);
        } else if (cmd[6] == "F4") {//设置电机张力通道数：4道电机张力、5道电机张力、6道电机张力
            //更新变量
            GlobalConfig::gl_motor_channel_number = cmd[7].toUShort(&ok,16);

            //保存配置文件
            CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                         "AppGlobalConfig/gl_motor_channel_number",
                                         QString::number(GlobalConfig::gl_motor_channel_number));
        } else if (cmd[6] == "F5") {//读电机张力通道数
            //返回：16 01 设备地址 04 E8 02 1F 通道数(4/5/6) 校验和
            data[0] = 0x04;
            data[1] = 0xE8;
            data[2] = 0x02;
            data[3] = 0x1F;
            data[4] = GlobalConfig::gl_motor_channel_number;
            CommonCode(data,tcpHelper);
        } else if (cmd[6] == "F6") {//设置电机张力是否添加级联
            //更新变量
            GlobalConfig::is_motor_add_link = cmd[7].toUShort(&ok,16);

            //保存到配置文件
            CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                         "AppGlobalConfig/is_motor_add_link",
                                         QString::number(GlobalConfig::is_motor_add_link));
        } else if (cmd[6] == "F7") {//读电机张力是否添加级联
            //返回：16 01 设备地址 04 E8 02 20 0/1 校验和
            data[0] = 0x04;
            data[1] = 0xE8;
            data[2] = 0x02;
            data[3] = 0x20;
            data[4] = GlobalConfig::is_motor_add_link;
            CommonCode(data,tcpHelper);
        } else if (cmd[6] == "F8") {//读取详细报警信息

        } else if (cmd[6] == "F9") {//设置平均值点数-->采样多少个点求平均值
            QByteArray MotorRunCmd;
            MotorRunCmd[0] = 0x16;
            MotorRunCmd[1] = 0xFF;
            MotorRunCmd[2] = 0x01;
            MotorRunCmd[3] = 0x04;
            MotorRunCmd[4] = 0xE8;
            MotorRunCmd[5] = 0x02;
            MotorRunCmd[6] = 0xF9;

            quint8 avg_point = cmd[7].toUShort(&ok,16);
            if (avg_point < 4) {
                MotorRunCmd[7] = 4;
            } else if (avg_point > 8) {
                MotorRunCmd[7] = 8;
            } else {
                MotorRunCmd[7] = avg_point;
            }

            //计算校验和
            quint8 check_parity_sum = 0;
            for (int i = 1; i < 8; i++) {
                check_parity_sum += MotorRunCmd[i];
            }

            MotorRunCmd[8] = check_parity_sum; //校验和

            //发送给电机控制杆
            GlobalConfig::SendMsgToMotorControlBuffer.append(MotorRunCmd);

            //返回：16 01 设备地址 04 E8 02 21 [平均值点数] 校验和
            data[0] = 0x04;
            data[1] = 0xE8;
            data[2] = 0x02;
            data[3] = 0x21;
            data[4] = cmd[7].toUShort(&ok,16);
            CommonCode(data,tcpHelper);
        } else if (cmd[6] == "FA") {//设置报警点数-->连续多少个点报警才判定为报警
            //更新变量
            quint8 alarm_point = cmd[7].toUShort(&ok,16);

            if (alarm_point < 4) {
                GlobalConfig::alarm_point_num = 4;
            } else if (alarm_point > 8){
                GlobalConfig::alarm_point_num = 8;
            } else {
                GlobalConfig::alarm_point_num = alarm_point;
            }

            //保存到配置文件
            CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                         "AppGlobalConfig/alarm_point_num",
                                         QString::number(GlobalConfig::alarm_point_num));

            //返回：16 01 设备地址 04 E8 02 22 [报警点数] 校验和
            data[0] = 0x04;
            data[1] = 0xE8;
            data[2] = 0x02;
            data[3] = 0x22;
            data[4] = GlobalConfig::alarm_point_num;
            CommonCode(data,tcpHelper);
        }
    }
}

void ParseAlarmHostTcpMsg::CommonCode(QByteArray data, TcpHelper *tcpHelper)
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

    //发送给报警主机
    SendMotorStatusInfoToAlarmHost(str.join(" ").toUpper(), tcpHelper);
}

//根据报警主机发送过来的命令返回电机张力相应的数据包
void ParseAlarmHostTcpMsg::SendMotorStatusInfoToAlarmHost(QString cmd,TcpHelper *tcpHelper)
{
    QString MessageMerge;
    QDomDocument AckDom;

    //xml声明
    QString XmlHeader("version=\"1.0\" encoding=\"UTF-8\"");
    AckDom.appendChild(AckDom.createProcessingInstruction("xml", XmlHeader));

    //创建根元素
    QDomElement RootElement = AckDom.createElement("Device");

    RootElement.setAttribute("DeviceIP", GlobalConfig::LocalHostIP);
    RootElement.setAttribute("DefenceID", GlobalConfig::MainDefenceID + "|" + GlobalConfig::SubDefenceID);
    AckDom.appendChild(RootElement);

    //创建DeviceData元素
    QDomElement DeviceData = AckDom.createElement("DeviceData");
    QDomText DeviceDataText = AckDom.createTextNode(cmd);
    DeviceData.appendChild(DeviceDataText);
    RootElement.appendChild(DeviceData);

    QTextStream Out(&MessageMerge);
    AckDom.save(Out,4);

    int length = MessageMerge.size();
    MessageMerge = QString("IALARM:") + QString("%1").arg(length,13,10,QLatin1Char('0')) + MessageMerge;

    tcpHelper->SendMsgBuffer.append(MessageMerge.toAscii());
}

//实时保存电机的最新状态信息
void ParseAlarmHostTcpMsg::SaveMotorLastestStatusInfo()
{
    //1、保存配置信息
    quint8 ConfigInfo[35] = {0x16,0x01,GlobalConfig::ip_addr,0x1E,0xE8,0x1C,0x08,
                            HIGH(GlobalConfig::ad_sensor_mask_LR),//张力掩码左
                            LOW(GlobalConfig::ad_sensor_mask_LR), //张力掩码右

                            HIGH(GlobalConfig::ad_still_dn),//静态张力允许下限高
                            LOW(GlobalConfig::ad_still_dn), //静态张力允许下限低

                            HIGH(GlobalConfig::ad_still_up),//静态张力允许上限高
                            LOW(GlobalConfig::ad_still_up), //静态张力允许上限低

                            GlobalConfig::system_status,    //报警阀值下浮比例

                            GlobalConfig::ad_still_Dup[0],  //报警阀值左1
                            GlobalConfig::ad_still_Dup[1],  //报警阀值左2
                            GlobalConfig::ad_still_Dup[2],  //报警阀值左3
                            GlobalConfig::ad_still_Dup[3],  //报警阀值左4
                            GlobalConfig::ad_still_Dup[4],  //报警阀值左5
                            GlobalConfig::ad_still_Dup[5],  //报警阀值左6
                            0,                              //左开关量
                            GlobalConfig::ad_still_Dup[12], //杆自身

                            GlobalConfig::ad_still_Dup[6],  //报警阀值右1
                            GlobalConfig::ad_still_Dup[7],  //报警阀值右2
                            GlobalConfig::ad_still_Dup[8],  //报警阀值右3
                            GlobalConfig::ad_still_Dup[9],  //报警阀值右4
                            GlobalConfig::ad_still_Dup[10], //报警阀值右5
                            GlobalConfig::ad_still_Dup[11], //报警阀值右6
                            0,                              //右开关量
                            GlobalConfig::ad_still_Dup[12], //杆自身

                            0,                              //双/单防区
                            GlobalConfig::ip_addr,          //拨码地址
                            GlobalConfig::beep_during_temp * SCHEDULER_TICK / 1000, //声光报警输出时间
                            0                               //联动输出时间
                           };

    //计算检验和
    ConfigInfo[34] = 0;
    for (int i = 1; i < 34; i++) {
        ConfigInfo[34] += ConfigInfo[i];
    }

    //把命令转换成16进制字符串
    QStringList str = QStringList();
    for (int i = 0; i < 35; i++) {
        str << QString("%1").arg(ConfigInfo[i],2,16,QLatin1Char('0'));
    }

    //2、保存报警信息
    quint8 AlarmInfo[15] = {0x16,0x01,GlobalConfig::ip_addr,0x0A,0xE8,0x08,0x1A,
                            HIGH(GlobalConfig::ad_sensor_mask_LR),//张力掩码左
                            LOW(GlobalConfig::ad_sensor_mask_LR), //张力掩码右

                            HIGH(GlobalConfig::ad_alarm_exts),//外力报警左
                            LOW(GlobalConfig::ad_alarm_exts), //外力报警右

                            HIGH(GlobalConfig::ad_alarm_base),//静态张力报警左
                            LOW(GlobalConfig::ad_alarm_base), //静态张力报警右

                            GlobalConfig::doorkeep_state      //门磁
                           };

    //计算检验和
    AlarmInfo[14] = 0;
    for (int i = 1; i < 14; i++) {
        AlarmInfo[14] += AlarmInfo[i];
    }

    //把命令转换成16进制字符串
    for (int i = 0; i < 15; i++) {
        str << QString("%1").arg(AlarmInfo[i],2,16,QLatin1Char('0'));
    }

    //3、保存瞬间张力
    quint8 InstantSampleValue[40] = {0x16,0x01,GlobalConfig::ip_addr,0x23,0xE8,0x21,0x1C};

    quint16 temp16;

    for (int j = 0; j < 6; j++) { //左1~6
        temp16 = GlobalConfig::ad_chn_sample[j];
        InstantSampleValue[7 + (j << 1)] = HIGH(temp16);
        InstantSampleValue[8 + (j << 1)] = LOW(temp16);
    }
    //左开关量
    InstantSampleValue[19] = 0;
    InstantSampleValue[20] = 0;

    //杆自身
    temp16 = GlobalConfig::ad_chn_sample[12];
    InstantSampleValue[21] = HIGH(temp16);
    InstantSampleValue[22] = LOW(temp16);

    for (int j = 0; j < 6; j++) { //右1~6
        temp16 = GlobalConfig::ad_chn_sample[6 + j];
        InstantSampleValue[23 + (j << 1)] = HIGH(temp16);
        InstantSampleValue[24 + (j << 1)] = LOW(temp16);
    }
    //右开关量
    InstantSampleValue[35] = 0;
    InstantSampleValue[36] = 0;

    //杆自身
    temp16 = GlobalConfig::ad_chn_sample[12];
    InstantSampleValue[37] = HIGH(temp16);
    InstantSampleValue[38] = LOW(temp16);

    //计算检验和
    InstantSampleValue[39] = 0;
    for (int i = 1; i < 39; i++) {
        InstantSampleValue[39] += InstantSampleValue[i];
    }

    //把命令转换成16进制字符串
    for (int i = 0; i < 40; i++) {
        str << QString("%1").arg(InstantSampleValue[i],2,16,QLatin1Char('0'));
    }

    //4、存静态基准
    quint8 StaticBaseValue[40] = {0x16,0x01,GlobalConfig::ip_addr,0x23,0xE8,0x21,0x1D};

    for (int j = 0; j < 6; j++) { //左1~6
        temp16 = GlobalConfig::ad_chn_base[j].base;
        StaticBaseValue[7 + (j << 1)] = HIGH(temp16);
        StaticBaseValue[8 + (j << 1)] = LOW(temp16);
    }
    //左开关量
    StaticBaseValue[19] = 0;
    StaticBaseValue[20] = 0;

    //杆自身
    temp16 = GlobalConfig::ad_chn_base[12].base;
    StaticBaseValue[21] = HIGH(temp16);
    StaticBaseValue[22] = LOW(temp16);

    for (int j = 0; j < 6; j++) { //右1~6
        temp16 = GlobalConfig::ad_chn_base[6 + j].base;
        StaticBaseValue[23 + (j << 1)] = HIGH(temp16);
        StaticBaseValue[24 + (j << 1)] = LOW(temp16);
    }
    //右开关量
    StaticBaseValue[35] = 0;
    StaticBaseValue[36] = 0;

    //杆自身
    temp16 = GlobalConfig::ad_chn_base[12].base;
    StaticBaseValue[37] = HIGH(temp16);
    StaticBaseValue[38] = LOW(temp16);

    //计算检验和
    StaticBaseValue[39] = 0;
    for (int i = 1; i < 39; i++) {
        StaticBaseValue[39] += StaticBaseValue[i];
    }

    //把命令转换成16进制字符串
    for (int i = 0; i < 40; i++) {
        str << QString("%1").arg(StaticBaseValue[i],2,16,QLatin1Char('0'));
    }

    //5、钢丝是否剪断
    quint8 MotorStatusInfo[11] = {0x16,0x01,GlobalConfig::ip_addr,0x06,0xE8,0x04,0x1E,
                           HIGH(GlobalConfig::ad_chnn_wire_cut),
                           LOW(GlobalConfig::ad_chnn_wire_cut),
                           GlobalConfig::gl_motor_adjust_flag};

    //计算检验和
    MotorStatusInfo[10] = 0;
    for (int i = 1; i < 10; i++) {
        MotorStatusInfo[10] += MotorStatusInfo[i];
    }

    //把命令转换成16进制字符串
    for (int i = 0; i < 11; i++) {
        str << QString("%1").arg(MotorStatusInfo[i],2,16,QLatin1Char('0'));
    }

    //保存到全局变量中
    GlobalConfig::status = str.join(" ").toUpper();
}

void ParseAlarmHostTcpMsg::slotUpdateDateTime()
{
   CommonSetting::SettingSystemDateTime(NowTime);
}
