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

    if (GlobalConfig::check_wire_cut_tick > 0) {
        GlobalConfig::check_wire_cut_tick--;

        if (GlobalConfig::check_wire_cut_tick == 0) {
            for (int i = 0; i < 12; i++) {
                if (GlobalConfig::ad_chn_sample[i] > 10) {//钢丝没有被剪断
                    if ((i >= 0) && (i <= 5)) {//左1~6
                        //ad_chnn_wire_cut的bit15-bit0：X X 左6~左1、X X 右6~右1
                        GlobalConfig::ad_chnn_wire_cut &= ~(1 << (i + 8));
                    } else if ((i >= 6) && (i <= 11)) {//右1~6
                        //ad_chnn_wire_cut的bit15-bit0：X X 左6~左1、X X 右6~右1
                        GlobalConfig::ad_chnn_wire_cut &= ~(1 << (i - 6));
                    }
                }
            }
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
