#include "scheduler.h"
#include "globalconfig.h"

Scheduler *Scheduler::instance = NULL;

Scheduler::Scheduler(QObject *parent) :
    QObject(parent)
{
}

void Scheduler::Start(void)
{
    TaskSchedulerTimer = new QTimer(this);
    TaskSchedulerTimer->setInterval(SCHEDULER_TICK);
    connect(TaskSchedulerTimer,SIGNAL(timeout()),this,SLOT(slotTaskScheduler()));
    TaskSchedulerTimer->start();
}

void Scheduler::slotTaskScheduler()
{
    if (GlobalConfig::gl_delay_tick > 0) {//系统计时
        GlobalConfig::gl_delay_tick--;

        if (GlobalConfig::gl_delay_tick == 0) {
            GlobalConfig::system_status = GlobalConfig::SYS_SAMP_BASE;
        }
    }

    for (int i = 0; i < 13; i++) {
        if (GlobalConfig::ad_alarm_tick[i] > 0) {
            GlobalConfig::ad_alarm_tick[i]--;
        }
    }

//    /* beep & alarm out */
//    if (GlobalConfig::beep_flag) {
//        //正在beep
//        if (GlobalConfig::beep_timer > 0) {
//            GlobalConfig::beep_timer--;
//        }

//        if (GlobalConfig::beep_timer == 0) {
//            //蜂鸣时间到，静音
//            quint8 beep_ctrl = 0;
//            ioctl(GlobalConfig::fd, SET_BEEP_STATE, &beep_ctrl);
//            GlobalConfig::beep_flag = 0;
//        }
//    }
}
