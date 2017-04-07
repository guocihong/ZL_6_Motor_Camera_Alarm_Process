#include "tcphelper.h"

TcpHelper::TcpHelper(QObject *parent) :
    QObject(parent)
{
    this->Socket = NULL;
    this->RecvOriginalMsgBuffer.clear();
    this->RecvVaildCompleteMsgBuffer.clear();
    this->SendMsgBuffer.clear();
}
