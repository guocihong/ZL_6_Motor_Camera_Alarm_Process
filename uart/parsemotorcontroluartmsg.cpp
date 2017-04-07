#include "parsemotorcontroluartmsg.h"
#include "devicecontrolutil.h"

/*
 * 本类专门用来解析电机控制杆串口RS232发送过来的数据包
 */

ParseMotorControlUartMsg *ParseMotorControlUartMsg::instance = NULL;

ParseMotorControlUartMsg::ParseMotorControlUartMsg(QObject *parent) :
    QObject(parent)
{
}


void ParseMotorControlUartMsg::Parse(void)
{
    //相关变量初始化
    ad_samp_pnum                               = 0;        //采样点数(计算静态基准值时总采样点数)
    for (int i = 0; i < 13; i++) {
        GlobalConfig::ad_chn_sample[i]         = 0;        //最新一轮采样值
        ad_samp_sum[i].sum                     = 0;        //阶段求和
        ad_samp_sum[i].point                   = 0;
        GlobalConfig::ad_chn_base[i].base      = 0;        //各通道静态基准值/上下限阀值
        GlobalConfig::ad_chn_base[i].base_down = 0;
        GlobalConfig::ad_chn_base[i].base_up   = 0;
        ad_chn_over[i]                         = 0;        //各通道连续采样点(均衡后)的阀值判定：均在范围内
        md_point[i]                            = 0;        //用于基准值跟踪的计量点数
        GlobalConfig::ad_alarm_tick[i]         = 0;
    }

    for (int i = 0; i < 12; i++) {
        gl_motor_overcur_point[i] = 0;
        gl_motor_adjust_end[i]    = 0;
        ad_chnn_state[i]          = 0;
    }

    GlobalConfig::zs_climb_alarm_flag    = 0;
    GlobalConfig::right_climb_alarm_flag = 0;
    GlobalConfig::left_climb_alarm_flag  = 0;

    GlobalConfig::ad_alarm_exts    = 0;                    //外力报警标志（无mask）: 无
    GlobalConfig::ad_alarm_base    = 0;                    //静态张力报警标志（无mask）：允许范围内
    GlobalConfig::gl_chnn_index    = 0;                    //从左1开始
    GlobalConfig::ad_chnn_wire_cut = 0;

    ParseMotorControlUartMsgTimer = new QTimer(this);
    ParseMotorControlUartMsgTimer->setInterval(200);
    connect(ParseMotorControlUartMsgTimer,SIGNAL(timeout()),this,SLOT(slotParseMotorControltUartMsg()));
    ParseMotorControlUartMsgTimer->start();
}

void ParseMotorControlUartMsg::slotParseMotorControltUartMsg(void)
{
    while(GlobalConfig::RecvVaildCompletePackageFromMotorControlBuffer.size() > 0) {
        //1、提取数据
        QByteArray cmd =
                GlobalConfig::RecvVaildCompletePackageFromMotorControlBuffer.takeFirst();

        //左1~6,右1~6,杆自身的瞬间张力值
        for (int i = 0; i < 13; i++) {
            GlobalConfig::ad_chn_sample[i] = (cmd[7 + (i << 1)] << 8) + cmd[8 + (i << 1)];
        }

        //控制杆的门磁状态
        gl_control_dk_status               = cmd[33];//0-报警;1-正常

        //电机状态
        GlobalConfig::gl_motor_adjust_flag = cmd[34];//0-停止;1-运转

        //电机堵转状态
        gl_motor_overcur_flag              = cmd[35];//0-正常;1-堵转

        //时间是否用完
        is_timeout                         = cmd[36];//0-没有;1-时间用完

        //2、数据解析
        switch (GlobalConfig::system_status) {
        case GlobalConfig::SYS_SAMP_BASE://初始上电时的静态基准值采样
            for (int i = 0; i < 13; i++) {
                ad_samp_sum[i].sum += GlobalConfig::ad_chn_sample[i];
            }
            ad_samp_pnum++;

            if (ad_samp_pnum == 32) {//已经满基准值采样点数（每通道32点，均衡后, 耗时约10秒)
                ad_samp_pnum = 0;

                //计算均值和上下限
                for (int i = 0; i < 13; i++) {
                    //基准
                    GlobalConfig::ad_chn_base[i].base = ad_samp_sum[i].sum >> 5;   //除于32

                    //下限 = 基准 * （1 / 3）
                    quint16 val_temp = GlobalConfig::ad_chn_base[i].base;
                    GlobalConfig::ad_chn_base[i].base_down =
                            (val_temp >> 1) - (val_temp >> 3) - (val_temp >> 4);

                    //上限
                    if ((1023 - GlobalConfig::ad_chn_base[i].base) >
                            GlobalConfig::ad_still_Dup[i]) {
                        GlobalConfig::ad_chn_base[i].base_up =
                                GlobalConfig::ad_chn_base[i].base + GlobalConfig:: ad_still_Dup[i];
                    } else {
                        GlobalConfig::ad_chn_base[i].base_up = 1023;
                    }

                    //检查静态张力是否在允许范围内
                    //只有左6道钢丝和右6道钢丝才需要判断是否静态报警,杆自身不存在静态报警,杆自身只有外力报警
                    if ((i >= 0) && (i <= 11)) {
                        check_still_stress(i);
                    }

                    //复位阶段和变量，准备用于自适应阀值跟踪
                    ad_samp_sum[i].sum   = 0;
                    ad_samp_sum[i].point = 0;
                }

                //更新led状态
                update_led_status();

                //更新组合报警标志
                update_alarm_status();

                //状态->开机自检
                GlobalConfig::system_status = GlobalConfig::SYS_SELF_CHECK1;
            }
            break;

        case GlobalConfig::SYS_SELF_CHECK1:

            break;
        case GlobalConfig::SYS_SELF_CHECK2:
            break;
        case GlobalConfig::SYS_CHECK:
            break;
        }
    }
}

//检查指定通道的静态张力是否在允许范围内
void ParseMotorControlUartMsg::check_still_stress(quint8 index)
{
    if ((GlobalConfig::ad_chn_base[index].base >= GlobalConfig::ad_still_dn) &&
        (GlobalConfig::ad_chn_base[index].base <= GlobalConfig::ad_still_up)) {
        if ((index >= 0) && (index <= 5)) {//左1~左6
            //在允许范围内, 清标志
            GlobalConfig::ad_alarm_base &= (~((quint16)0x01 << (index + 8)));
        } else if ((index >= 6) && (index <= 11)) {//右1~右6
            //在允许范围内, 清标志
            GlobalConfig::ad_alarm_base &= (~((quint16)0x01 << (index - 6)));
        }
    } else {
        if ((index >= 0) && (index <= 5)) {//左1~左6
            //超出允许范围，置标志
            GlobalConfig::ad_alarm_base |= ((quint16)0x01 << (index + 8));
        } else if ((index >= 6) && (index <= 11)) {//右1~右6
            //超出允许范围，置标志
            GlobalConfig::ad_alarm_base |= ((quint16)0x01 << (index - 6));
        }
    }

    //杆自身、左开关量、右开关量不存在静态报警
    GlobalConfig::ad_alarm_base &= 0x3F3F;
}

void ParseMotorControlUartMsg::update_led_status(void)
{
    //杆自身报警标志,左1~6报警标志,右1~6报警标志
    quint16 alarm_state = GlobalConfig::ad_alarm_exts | GlobalConfig::ad_alarm_base;
    DeviceControlUtil::UpdateLedStatus(alarm_state);
}

void ParseMotorControlUartMsg::update_alarm_status(void)
{
    quint8 temp;

    //左开关量、左6~1报警标志
    //过滤杆自身攀爬报警，杆自身攀爬报警不算在左防区报警内，单独开辟一个攀爬报警
    temp = HIGH((GlobalConfig::ad_alarm_exts | GlobalConfig::ad_alarm_base)) & HIGH(GlobalConfig::ad_sensor_mask_LR) & 0x7F;
    //判左侧组合报警
    if (temp == 0) {
        GlobalConfig::adl_alarm_flag = 0;   //无报警
    } else {
        GlobalConfig::adl_alarm_flag = 1;     //有报警
    }

    //右开关量、右6~1报警标志
    //过滤杆自身攀爬报警，杆自身攀爬报警不算在右防区报警内，单独开辟一个攀爬报警
    temp = LOW((GlobalConfig::ad_alarm_exts | GlobalConfig::ad_alarm_base)) & LOW(GlobalConfig::ad_sensor_mask_LR) & 0x7F;
    //判右侧组合报警
    if (temp == 0) {
        GlobalConfig::adr_alarm_flag = 0;   //无报警
    } else {
        GlobalConfig::adr_alarm_flag = 1;   //有报警
    }

    //杆自身攀爬报警
    temp = HIGH(GlobalConfig::ad_alarm_exts) & HIGH(GlobalConfig::ad_sensor_mask_LR) & 0x80;
    if (temp == 0) {
        GlobalConfig::zs_climb_alarm_flag = 0;//无报警
    } else {
        GlobalConfig::zs_climb_alarm_flag = 1;//有报警
    }
}
