#ifndef COMMONSETTING_H
#define COMMONSETTING_H

#include <QObject>
#include <QTextCodec>
#include <QSettings>
#include <QProcess>
#include <QTime>
#include <QThread>
#include <QTcpSocket>
#include <QTcpServer>
#include <QTimer>
#include <QDomDocument>
#include <QBuffer>
#include <QDebug>
#include <QCoreApplication>
#include <QNetworkInterface>
#include <QFile>

class CommonSetting : public QObject
{
public:
    CommonSetting();
    ~CommonSetting();

    //设置编码为UTF8
    static void SetUTF8Code()
    {
        QTextCodec *codec=
                QTextCodec::codecForName("UTF-8");
        QTextCodec::setCodecForLocale(codec);
        QTextCodec::setCodecForCStrings(codec);
        QTextCodec::setCodecForTr(codec);
    }

    //文件是否存在
    static bool FileExists(QString strFile)
    {
        QFile tempFile(strFile);
        if (tempFile.exists()){
            return true;
        }
        return false;
    }

    //写数据到文件
    static void WriteCommonFile(QString fileName,QString data)
    {
        QFile file(fileName);
        if(file.open(QFile::ReadWrite | QFile::Append)){
            file.write(data.toLocal8Bit().data());
            file.close();
        }
    }

    static QString GetCurrentDateTimeNoSpace()
    {
        QDateTime time = QDateTime::currentDateTime();
        return time.toString("yyyy-MM-dd hh:mm:ss");
    }

   //QSetting应用
    static void WriteSettings(const QString &ConfigFile,
                              const QString &key,
                              const QString value)
    {

        QSettings settings(ConfigFile,QSettings::IniFormat);
        settings.setValue(key,value);
    }

    static QString ReadSettings(const QString &ConfigFile,
                                const QString &key)
    {
        QSettings settings(ConfigFile,QSettings::IniFormat);
        settings.setIniCodec("UTF-8");
        return settings.value(key).toString();
    }

    static void SettingSystemDateTime(QString SystemDate)
    {
        //设置系统时间
        QString year = SystemDate.mid(0,4);
        QString month = SystemDate.mid(5,2);
        QString day = SystemDate.mid(8,2);
        QString hour = SystemDate.mid(11,2);
        QString minute = SystemDate.mid(14,2);
        QString second = SystemDate.mid(17,2);
        QProcess *process = new QProcess;
        process->start(tr("date %1%2%3%4%5.%6").arg(month).arg(day)
                       .arg(hour).arg(minute)
                       .arg(year).arg(second));
        process->waitForFinished(200);
        process->start("hwclock -w");
        process->waitForFinished(200);
        delete process;
    }

    static void Sleep(quint16 msec)
    {
        QTime dieTime = QTime::currentTime().addMSecs(msec);
        while(QTime::currentTime() < dieTime)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }

    static QString GetLocalHostIP()
    {
        QList<QNetworkInterface> list =
                QNetworkInterface::allInterfaces();
        foreach(const QNetworkInterface &interface,list){
            if(interface.name() == "eth0"){
                QList<QNetworkAddressEntry> entrylist = interface.addressEntries();
                foreach (QNetworkAddressEntry entry, entrylist) {
                    return entry.ip().toString();
                }
            }
        }
    }

    static QString GetMask()
    {
        QList<QNetworkInterface> list =
                QNetworkInterface::allInterfaces();
        foreach(const QNetworkInterface &interface,list){
            if(interface.name() == "eth0"){
                QList<QNetworkAddressEntry> entrylist = interface.addressEntries();
                foreach (QNetworkAddressEntry entry, entrylist) {
                    return entry.netmask().toString();
                }
            }
        }
    }

    static QString GetGateway()
    {
        system("rm -rf gw.txt");
        system("route -n | grep 'UG' | awk '{print $2}' > gw.txt");

        QString gw;

        QFile file("gw.txt");
        if(file.open(QFile::ReadOnly)){
            gw = QString(file.readAll()).trimmed().simplified();
            file.close();
        }

        return gw;
    }

    static QString ReadMacAddress()
    {
        QList<QNetworkInterface> list =
                QNetworkInterface::allInterfaces();
        foreach(const QNetworkInterface &interface,list){
            if(interface.name() == "eth0"){
                QString MacAddress = interface.hardwareAddress();
                return MacAddress;
            }
        }
    }
};

#endif // COMMONSETTING_H
