#include "scheduler.h"
#include "globalconfig.h"
#include "devicecontrolutil.h"
#include "CommonSetting.h"

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

    /* beep & alarm out */
    if (GlobalConfig::beep_flag) {
        //正在beep
        if (GlobalConfig::beep_timer > 0) {
            GlobalConfig::beep_timer--;
        }

        if (GlobalConfig::beep_timer == 0) {
            //蜂鸣时间到，静音
            DeviceControlUtil::DisableBeep();
            GlobalConfig::beep_flag = 0;
        }
    }

    if (GlobalConfig::check_sample_clear_tick > 0) {
        GlobalConfig::check_sample_clear_tick--;

        if (GlobalConfig::check_sample_clear_tick == 0) {
            GlobalConfig::is_sample_clear = 1;

            CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                         "AppGlobalConfig/is_sample_clear",
                                         QString::number(GlobalConfig::is_sample_clear));
        }
    }
}
