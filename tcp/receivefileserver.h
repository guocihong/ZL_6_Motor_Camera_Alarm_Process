#ifndef RECEIVEFILESERVER_H
#define RECEIVEFILESERVER_H

#include <QTcpServer>

class ReceiveFileServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit ReceiveFileServer(QObject *parent = 0);
    ~ReceiveFileServer();

    bool startServer(quint16 port);
    void stopServer();

private slots:
        void updateReceiveStatus(QString msg);

signals:
    void receiveFileName(QString name);
    void receiveFileSize(qint64 size);
    void finished();
    void message(QString msg);
    void receiveData(qint64 size);

protected:
    void incomingConnection(int handle);

};

#endif // RECEIVEFILESERVER_H
