#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <QObject>
#include <QTimer>
#include <QMutexLocker>

class Scheduler : public QObject
{
    Q_OBJECT
public:
    explicit Scheduler(QObject *parent = 0);

    static Scheduler *newInstance() {
        static QMutex mutex;

        if (!instance) {
            QMutexLocker locker(&mutex);

            if (!instance) {
                instance = new Scheduler();
            }
        }

        return instance;
    }

    void Start(void);

private slots:
    void slotTaskScheduler();

private:
    static Scheduler *instance;

    QTimer *TaskSchedulerTimer;
};

#endif // SCHEDULER_H
