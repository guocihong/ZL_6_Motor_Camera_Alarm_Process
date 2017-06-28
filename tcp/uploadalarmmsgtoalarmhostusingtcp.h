#ifndef UPLOADALARMMSGTOALARMHOSTUSINGTCP_H
#define UPLOADALARMMSGTOALARMHOSTUSINGTCP_H

#include <QObject>
#include <QMutex>
#include <QTimer>

class UploadAlarmMsgToAlarmHostUsingTcp : public QObject
{
    Q_OBJECT
public:
    explicit UploadAlarmMsgToAlarmHostUsingTcp(QObject *parent = 0);

    static UploadAlarmMsgToAlarmHostUsingTcp *newInstance() {
        static QMutex mutex;

        if (!instance) {
            QMutexLocker locker(&mutex);

            if (!instance) {
                instance = new UploadAlarmMsgToAlarmHostUsingTcp();
            }
        }

        return instance;
    }

    void Listen();

    void AlarmMsg(int id, int index);
    void CameraStatusInfo(int id, QString CameraStatusInfo);
    QString GetAlarmDetailInfo();

    QString CommonCode(QByteArray data);

    void UploadStressAlarmMsg(int id, int index);//上传受力杆报警信息

public slots:
    void slotUploadAlarmMsgToAlarmHost();

private:
    static UploadAlarmMsgToAlarmHostUsingTcp *instance;

    QTimer *UploadAlarmMsgToAlarmHostTimer;
};

#endif // UPLOADALARMMSGTOALARMHOSTUSINGTCP_H
