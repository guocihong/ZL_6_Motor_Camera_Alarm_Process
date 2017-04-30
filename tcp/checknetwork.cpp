#include "checknetwork.h"
#include "globalconfig.h"
#include "CommonSetting.h"

CheckNetWork *CheckNetWork::instance = NULL;

CheckNetWork::CheckNetWork(QObject *parent) :
    QObject(parent)
{
}

void CheckNetWork::Listen()
{
    UploadOkMsgToAlarmHostTimer = new QTimer(this);
    UploadOkMsgToAlarmHostTimer->setInterval(5000);
    connect(UploadOkMsgToAlarmHostTimer,SIGNAL(timeout()),this,SLOT(slotCheckNetworkState()));
    UploadOkMsgToAlarmHostTimer->start();
}

void CheckNetWork::slotCheckNetworkState()
{
    QDateTime now = QDateTime::currentDateTime();

    int value = GlobalConfig::RecvAlarmHostLastMsgTime.secsTo(now);

    if (value > 5) {//5s没有收到报警主机的消息了，有可能是报警主机没有启动服务，或者网络不通
        SendOKAckToAlarmHost();
    }
}

void CheckNetWork::SendOKAckToAlarmHost()
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

    GlobalConfig::SendOkMsgToAlarmHostBuffer.append(MessageMerge.toAscii());
}
