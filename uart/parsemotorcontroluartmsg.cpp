#include "parsemotorcontroluartmsg.h"
#include "devicecontrolutil.h"

/*
 * 本类专门用来解析电机控制杆串口RS232发送过来的数据包
 */

ParseMotorControlUartMsg *ParseMotorControlUartMsg::instance = NULL;

//报警点数为4,5，6,7,8时，用来判断是否发生报警
quint8 alarm_matrix[5] = {0x0F,0x1F,0x3F,0x7F,0xFF};

//0-11分别代表：左1 右1 左2 右2 左3 右3 左4 右4 左5 右5 左6 右6
quint8 matrix_index[12] = {0, 6, 1, 7, 2, 8, 3, 9, 4, 10, 5, 11};

//左1、右1、左2、右2、左3、右3、左4、右4、左5、右5
quint8 gl_5_motor_control_code[12] = {0x02,0x01,0x01,0x02,0x02,0x01,0x01,0x02,0x01,0x02,0x01,0x02};

//左1、右1、左2、右2、左3、右3、左4、右4、左5、右5、左6、右6
quint8 gl_6_motor_control_code[12] = {0x02,0x01,0x01,0x02,0x02,0x01,0x01,0x02,0x02,0x01,0x01,0x02};


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
                GlobalConfig::gl_chnn_index = 0;

                qDebug() << "GlobalConfig::system_status = GlobalConfig::SYS_CHECK";
            }
            break;

        case GlobalConfig::SYS_SELF_CHECK1://开机自检阶段1-->左1 右1 左2 右2 ... 左6 右6
            if((GlobalConfig::is_motor_add_link == 1) && (GlobalConfig::is_sample_clear == 0)){
                //1、加级联的情况下，控制杆上电不会自动收紧钢丝，需要在现场通过报警主机先进行采样值恢复，然后执行采样值清零，才会自动收紧钢丝
                //2、不加级联的情况下，控制杆一上电，就会自动收紧钢丝

                //立即同步更新静态基准值
                for (int i = 0; i < 12; i++) {
                    GlobalConfig::ad_chn_base[i].base = GlobalConfig::ad_chn_sample[i];
                    quint16 val_temp = GlobalConfig::ad_chn_base[i].base;
                    GlobalConfig::ad_chn_base[i].base_down = (val_temp >> 1) - (val_temp >> 3) - (val_temp >> 4);
                    if ((1023 - GlobalConfig::ad_chn_base[i].base) > GlobalConfig::ad_still_Dup[i]) {
                        GlobalConfig::ad_chn_base[i].base_up = GlobalConfig::ad_chn_base[i].base + GlobalConfig::ad_still_Dup[i];
                    } else {
                        GlobalConfig::ad_chn_base[i].base_up = 1023;
                    }

                    //检查静态张力是否在允许范围内
                    //只有左6道钢丝和右6道钢丝才需要判断是否静态报警,杆自身不存在静态报警,杆自身只有外力报警
                    check_still_stress(i);
                }

                GlobalConfig::gl_chnn_index = 0;

                //更新led状态
                update_led_status();

                //更新组合报警标志
                update_alarm_status();

                return;
            }

        {
            //gl_chnn_index取值范围为0~11，分别依次对应为：左1 右1 左2 右2 ... 左6 右6
            //ad_chn_sample/ad_chn_base的0~5对应左1~左6,6~11对应右1~右6，所以需要转换一下索引

            //索引转换
            //channel_index:0-11分别代表左1~左6，右1～6
            quint8 channel_index = matrix_index[GlobalConfig::gl_chnn_index];

            //判断拨码开关是否打开
            //ad_sensor_mask的bit11~bit0分别代表：右6~右1,左6~左1
            if ((GlobalConfig::ad_sensor_mask >> channel_index) & 0x0001) {//拨码打开
                if (GlobalConfig::ad_chn_sample[channel_index] < TARGET_SAMPLE_VALUE) {//没有达到预定目标
                    GlobalConfig::system_status = GlobalConfig::SYS_SELF_CHECK2;
                    motor_start(GlobalConfig::gl_chnn_index);
                } else {//满足预定目标
                    //本钢丝调整完成
                    gl_motor_adjust_end[channel_index] = 1;

                    //操作下一根钢丝
                    GlobalConfig::gl_chnn_index++;
                }
            } else {//拨码关闭
                //本钢丝调整完成
                gl_motor_adjust_end[channel_index] = 1;

                //操作下一根钢丝
                GlobalConfig::gl_chnn_index++;
            }

            if (GlobalConfig::gl_chnn_index == 12) {
                GlobalConfig::gl_chnn_index = 0;
            }

            //判断所有钢丝是否调整完成
            quint8 val_sum = 0;
            for (int i = 0; i < 12; i++) {
                val_sum += gl_motor_adjust_end[i];
            }

            if (val_sum == 12) {//所有钢丝调整完成
                GlobalConfig::system_status = GlobalConfig::SYS_CHECK;
                GlobalConfig::gl_chnn_index = 0;
            }
        }

            break;

        case GlobalConfig::SYS_SELF_CHECK2://开机自检阶段2
        {
            //索引转换
            //channel_index:0-11分别代表左1~左6，右1～6
            quint8 channel_index = matrix_index[GlobalConfig::gl_chnn_index];

            if (GlobalConfig::gl_motor_adjust_flag == 1) {//电机正在运转
                //立即同步更新静态基准值
                GlobalConfig::ad_chn_base[channel_index].base = GlobalConfig::ad_chn_sample[channel_index];
                quint16 val_temp = GlobalConfig::ad_chn_base[channel_index].base;
                GlobalConfig::ad_chn_base[channel_index].base_down = (val_temp >> 1) - (val_temp >> 3) - (val_temp >> 4);
                if ((1023 - GlobalConfig::ad_chn_base[channel_index].base) > GlobalConfig::ad_still_Dup[channel_index]) {
                    GlobalConfig::ad_chn_base[channel_index].base_up = GlobalConfig::ad_chn_base[channel_index].base + GlobalConfig::ad_still_Dup[channel_index];
                } else {
                    GlobalConfig::ad_chn_base[channel_index].base_up = 1023;
                }

                if (GlobalConfig::ad_chn_sample[channel_index] >= TARGET_SAMPLE_VALUE) {
                    //满足预定目标,停止电机
                    motor_stop();

                    //本道钢丝调整结束
                    gl_motor_adjust_end[channel_index] = 1;

                    //操作下一根钢丝
                    GlobalConfig::gl_chnn_index++;
                    GlobalConfig::system_status = GlobalConfig::SYS_SELF_CHECK1;
                }
            }

            //检查静态张力是否在允许范围内
            //只有左6道钢丝和右6道钢丝才需要判断是否静态报警,杆自身不存在静态报警,杆自身只有外力报警
            check_still_stress(channel_index);

            //更新led状态
            update_led_status();

            //更新组合报警标志
            update_alarm_status();

            if (gl_motor_overcur_flag == 1) {//电机堵转
                gl_motor_overcur_point[channel_index]++;
                if (gl_motor_overcur_point[channel_index] >= 3) {
                    gl_motor_adjust_end[channel_index] = 1;//本道钢丝调整结束
                }

                //操作下一根钢丝
                GlobalConfig::gl_chnn_index++;
                GlobalConfig::system_status = GlobalConfig::SYS_SELF_CHECK1;
            }

            if (is_timeout == 1) {//时间用完
                //操作下一根钢丝
                GlobalConfig::gl_chnn_index++;
                GlobalConfig::system_status = GlobalConfig::SYS_SELF_CHECK1;
            }

            if (GlobalConfig::gl_chnn_index == 12) {
                GlobalConfig::gl_chnn_index = 0;
            }

            //判断所有钢丝是否调整完成
            quint8 val_sum = 0;
            for (int i = 0; i < 12; i++) {
                val_sum += gl_motor_adjust_end[i];
            }

            if (val_sum == 12) {//所有钢丝调整完成
                GlobalConfig::system_status = GlobalConfig::SYS_CHECK;
                GlobalConfig::gl_chnn_index = 0;
            }
        }
            break;

        case GlobalConfig::SYS_CHECK:
            //1、分析钢丝是否被剪断
            for (int i = 0; i < 12; i++) {//0-12分别代表：左1~6、右1~6
                //ad_sensor_mask的bit11~bit0分别代表：右6~右1,左6~左1
                if ((GlobalConfig::ad_sensor_mask >> i) & 0x0001) {//拨码打开
                    if (GlobalConfig::ad_chn_sample[i] < WIRE_CUT_SAMPLE_VALUE) {
                        if (GlobalConfig::isEnterManualAdjustMotorMode) {
                            //当前处于手动调整钢丝模式，这种情况下，即使瞬间张力采样值小与10，也不认为钢丝被剪断
                            //do nothing
                        } else {//钢丝被剪断
                            if ((i >= 0) && (i <= 5)) {//左1~6
                                //ad_chnn_wire_cut的bit15-bit0：X X 左6~左1、X X 右6~右1
                                GlobalConfig::ad_chnn_wire_cut |= (1 << (i + 8));
                            } else if ((i >= 6) && (i <= 11)) {//右1~6
                                //ad_chnn_wire_cut的bit15-bit0：X X 左6~左1、X X 右6~右1
                                GlobalConfig::ad_chnn_wire_cut |= (1 << (i - 6));
                            }
                        }
                    }
                }
            }

            //2、分析钢丝的松紧程度
            if (!GlobalConfig::isEnterAutoAdjustMotorMode) {//没有进入自动调账钢丝模式
                for (int i = 0; i < 12; i++) {
                    ad_chnn_state[i] += GlobalConfig::ad_chn_sample[i];
                }
                ad_samp_pnum++;

                //SAMPLE_POINT为10的情况下，控制杆平均值点数为4，大约耗时2.6s，控制杆平均值点数为8，大约耗时5.2s
                if (ad_samp_pnum == SAMPLE_POINT) {
                    ad_samp_pnum = 0;

                    for (int i = 0; i < 12; i++) {
                        quint16 sample_value = ad_chnn_state[i] / SAMPLE_POINT;
                        if (sample_value < MIN_SAMPLE_VALUE) {//钢丝比较松，收紧钢丝
                            GlobalConfig::isEnterAutoAdjustMotorMode = true;//进入自动调账钢丝模式
                            GlobalConfig::adjust_status = 1;
                            GlobalConfig::gl_chnn_index = 0;  //从左1钢丝开始
                            break;
                        }
                    }

                    //复位，清零
                    for (int i = 0; i < 12; i++) {
                        ad_chnn_state[i] = 0;
                    }
                }
            }

            if (GlobalConfig::isEnterAutoAdjustMotorMode) {//进入自动调整钢丝模式
                //索引转换
                //channel_index:0-11分别代表左1~左6，右1～6
                quint8 channel_index = matrix_index[GlobalConfig::gl_chnn_index];

                switch(GlobalConfig::adjust_status) {
                case 1://分析钢丝拨码开关是否打开
                    //ad_sensor_mask的bit11~bit0分别代表：右6~右1,左6~左1
                    if ((GlobalConfig::ad_sensor_mask >> channel_index) & 0x0001) {//拨码打开
                        if ((channel_index >= 0) && (channel_index <= 5)) {//左1~左6
                            //ad_chnn_wire_cut的bit15-bit0：X X 左6~左1、X X 右6~右1
                            if (((GlobalConfig::ad_chnn_wire_cut >> (channel_index + 8)) & 0x0001) == 0) {//钢丝没有被剪断
                                quint16 sample_value = GlobalConfig::ad_chn_sample[channel_index];
                                if (sample_value < MIN_SAMPLE_VALUE) {//钢丝比较松，收紧钢丝
                                    motor_start(GlobalConfig::gl_chnn_index);
                                    GlobalConfig::adjust_status = 2;
                                } else {//瞬间张力符合要求，操作下一根钢丝
                                    GlobalConfig::gl_chnn_index++;
                                }
                            }
                        } else if ((channel_index >= 6) && (channel_index <= 11)) {//右1~右6
                            //ad_chnn_wire_cut的bit15-bit0：X X 左6~左1、X X 右6~右1
                            if (((GlobalConfig::ad_chnn_wire_cut >> (channel_index - 6)) & 0x0001) == 0) {//钢丝没有被剪断
                                quint16 sample_value = GlobalConfig::ad_chn_sample[channel_index];
                                if (sample_value < MIN_SAMPLE_VALUE) {//钢丝比较松，收紧钢丝
                                    motor_start(GlobalConfig::gl_chnn_index);
                                    GlobalConfig::adjust_status = 2;
                                } else {//瞬间张力符合要求，操作下一根钢丝
                                    GlobalConfig::gl_chnn_index++;
                                }
                            }
                        }
                    } else {//拨码开关没有打开，操作下一根钢丝
                        GlobalConfig::gl_chnn_index++;
                    }

                    if (GlobalConfig::gl_chnn_index == 12) {//所有钢丝调整完成，退出自动调整钢丝模式
                        GlobalConfig::isEnterAutoAdjustMotorMode = false;
                    }
                    break;

                case 2://同步更新静态基准值
                    if (GlobalConfig::gl_motor_adjust_flag == 1) {//电机正在运转
                        //立即同步更新静态基准值
                        GlobalConfig::ad_chn_base[channel_index].base = GlobalConfig::ad_chn_sample[channel_index];
                        quint16 val_temp = GlobalConfig::ad_chn_base[channel_index].base;
                        GlobalConfig::ad_chn_base[channel_index].base_down = (val_temp >> 1) - (val_temp >> 3) - (val_temp >> 4);
                        if ((1023 - GlobalConfig::ad_chn_base[channel_index].base) >
                                GlobalConfig::ad_still_Dup[channel_index]) {
                            GlobalConfig::ad_chn_base[channel_index].base_up =
                                    GlobalConfig::ad_chn_base[channel_index].base + GlobalConfig::ad_still_Dup[channel_index];
                        } else {
                            GlobalConfig::ad_chn_base[channel_index].base_up = 1023;
                        }

                        if (GlobalConfig::ad_chn_sample[channel_index] >= TARGET_SAMPLE_VALUE) {//达到预定目标
                            if (!GlobalConfig::isEnterManualAdjustMotorMode) {
                                //停止电机
                                motor_stop();

                                GlobalConfig::adjust_status = 1;

                                //操作下一根钢丝
                                GlobalConfig::gl_chnn_index++;
                            }

                        }
                    }

                    if (gl_motor_overcur_flag == 1) {//电机发生堵转，操作一根钢丝
                        GlobalConfig::adjust_status = 1;

                        //操作下一根钢丝
                        GlobalConfig::gl_chnn_index++;
                    }

                    if (is_timeout == 1) {//时间用完，操作下一根钢丝
                        GlobalConfig::adjust_status = 1;

                        //操作下一根钢丝
                        GlobalConfig::gl_chnn_index++;
                    }

                    if (GlobalConfig::gl_chnn_index == 12) {//所有钢丝调整完成，退出自动调整钢丝模式
                        GlobalConfig::isEnterAutoAdjustMotorMode = false;
                    }

                    /**********************************************************/
                    //通过报警主机发生命令控制电机运转，进入手动调整钢丝模式
                    //当进入手动调整钢丝时，即使瞬间张力采样值小于10时，也不认为钢丝被剪断
                    //但是当所有钢丝的瞬间张力采样值大于等于30时，退出手动调整钢丝模式
                    //退出手动调整钢丝模式后，如果钢丝瞬间张力采样值小于10，就认为钢丝被剪断
                    quint8 count = 0;
                    for (int k = 0; k < 12; k++) {
                        if (GlobalConfig::ad_chn_sample[k] >= 30) {
                            count++;
                        }
                    }

                    if (count == 12) {//退出手动调整钢丝模式
                        GlobalConfig::isEnterManualAdjustMotorMode = false;
                    }
                    /**********************************************************/
                    break;
                }
            }

            //3、解析12道钢丝是否外力报警以及静态报警-->左1~左6、右1~右6
            //解析控制杆是否外力报警，控制杆不存在静态报警
            for (int i = 0; i < 13; i++) {
                ad_chn_over[i] = ad_chn_over[i] << 1;   //Bit0填0，因此缺省在允许范围内
                quint16 val = GlobalConfig::ad_chn_sample[i];
                if (val <= GlobalConfig::ad_chn_base[i].base_up) {//在张力上/下限允许范围内
                    //a. 清标志(缺省)
                    //b. 计入跟踪基准值求和中
                    ad_samp_sum[i].sum += val;
                    ad_samp_sum[i].point++;

                    if (ad_samp_sum[i].point == 2) {
                        //满2点(约需0.6秒)
                        //b.0 计算这2点均值
                        quint16 val_temp = ad_samp_sum[i].sum >> 1;   //除于2, 得到这2点的均值
                        //b.1 更新基准值
                        if (GlobalConfig::ad_chn_base[i].base > (val_temp + 1)) {
                            //至少小2, 在缓慢松弛
                            //ZZX: 立即跟踪, 跟踪差值的 1/2
                            val_temp = (GlobalConfig::ad_chn_base[i].base - val_temp) >> 1;
                            if (GlobalConfig::ad_chn_base[i].base >= val_temp) {

                                GlobalConfig::ad_chn_base[i].base -= val_temp;
                                //同步更新上下限
                                val_temp = GlobalConfig::ad_chn_base[i].base;
                                GlobalConfig::ad_chn_base[i].base_down = (val_temp >> 1) - (val_temp >> 3) - (val_temp >> 4);
                                if ((1023 - GlobalConfig::ad_chn_base[i].base) > GlobalConfig::ad_still_Dup[i]) {
                                    GlobalConfig::ad_chn_base[i].base_up = GlobalConfig::ad_chn_base[i].base + GlobalConfig::ad_still_Dup[i];
                                } else {
                                    GlobalConfig::ad_chn_base[i].base_up = 1023;
                                }

                            }

                            //清缓慢张紧跟踪变量
                            md_point[i] = 0;
                        } else if (val_temp > (GlobalConfig::ad_chn_base[i].base + 1)) {
                            // 至少大2, 缓慢张紧
                            md_point[i]++;
                            if (md_point[i] >= DEF_ModiBASE_PT) {
                                // 已满缓慢张紧时的连续计量点数, 进行一次跟踪
                                // 1. 跟踪基准值
                                if (GlobalConfig::ad_chn_base[i].base < 1023) {
                                    //可以递增1
                                    GlobalConfig::ad_chn_base[i].base++;
                                    // 同步更新上下限
                                    val_temp = GlobalConfig::ad_chn_base[i].base;
                                    GlobalConfig::ad_chn_base[i].base_down = (val_temp >> 1) - (val_temp >> 3) - (val_temp >> 4);
                                    if (GlobalConfig::ad_chn_base[i].base_up < 1023) {
                                        GlobalConfig::ad_chn_base[i].base_up++;
                                    }
                                }
                                // 2. 清缓慢张紧跟踪变量
                                md_point[i] = 0;
                            }
                        }

                        //b.2 复位阶段和变量 - 用于4点4点平均的求和结构
                        ad_samp_sum[i].sum   = 0;
                        ad_samp_sum[i].point = 0;
                    }
                } else {
                    //外力报警, 置标志
                    ad_chn_over[i] |= 0x01;
                }

                //连续6点超范围，此通道有外力报警
                if ((ad_chn_over[i] & alarm_matrix[GlobalConfig::alarm_point_num - 4]) ==
                        alarm_matrix[GlobalConfig::alarm_point_num - 4]) {
                    //先保存报警详细信息，然后再更新静态基准值，这样就可以很好的分析出为什么报警
                    for (int j = 0; j < 13; j++) {
                        GlobalConfig::AlarmDetailInfo.StaticBaseValue[j] = GlobalConfig::ad_chn_base[j].base;
                        GlobalConfig::AlarmDetailInfo.InstantSampleValue[j] = GlobalConfig::ad_chn_sample[j];
                    }


                    if ((i >= 0) && (i <= 5)) {//左1~左6
                        //超出允许范围，置标志
                        GlobalConfig::ad_alarm_exts |= ((quint16)0x01 << (i + 8));
                    } else if ((i >= 6) && (i <= 11)) {//右1~右6
                        //超出允许范围，置标志
                        GlobalConfig::ad_alarm_exts |= ((quint16)0x01 << (i - 6));
                    }

                    if (i == 12) {//杆自身
                        GlobalConfig::ad_alarm_exts |= ((quint16)0x01 << (i + 3));
                        GlobalConfig::ad_alarm_exts |= ((quint16)0x01 << (i - 5));
                    }

                    //报警计时tick
                    GlobalConfig::ad_alarm_tick[i] = ALARM_TEMPO;

                    //立即更新静态基准值
                    GlobalConfig::ad_chn_base[i].base = val;
                    int val_temp = GlobalConfig::ad_chn_base[i].base;
                    GlobalConfig::ad_chn_base[i].base_down = (val_temp >> 1) - (val_temp >> 3) - (val_temp >> 4);
                    if ((1023 - GlobalConfig::ad_chn_base[i].base) > GlobalConfig::ad_still_Dup[i]) {
                        GlobalConfig::ad_chn_base[i].base_up = GlobalConfig::ad_chn_base[i].base + GlobalConfig::ad_still_Dup[i];
                    } else {
                        GlobalConfig::ad_chn_base[i].base_up = 1023;
                    }

                    //复位阶段和变量 - 用于4点4点平均的求和结构
                    ad_samp_sum[i].sum = 0;
                    ad_samp_sum[i].point = 0;

                    //清缓慢张紧跟踪变量
                    md_point[i] = 0;   //用于基准值跟踪的计量点数
                } else if ((ad_chn_over[i] & alarm_matrix[GlobalConfig::alarm_point_num - 4]) == 0x00) {//无外力报警
                    if (GlobalConfig::ad_alarm_tick[i] == 0) {//检查报警时间是否已到
                        //报警已经到最大报警时间, 停止报警
                        if ((i >= 0) && (i <= 5)) {//左1~左6
                            //在允许范围内, 清标志
                            GlobalConfig::ad_alarm_exts &= (~((quint16)0x01 << (i + 8)));
                        } else if ((i >= 6) && (i <= 11)) {//右1~右6
                            //在允许范围内, 清标志
                            GlobalConfig::ad_alarm_exts &= (~((quint16)0x01 << (i - 6)));
                        }

                        if (i == 12) {//杆自身
                            GlobalConfig::ad_alarm_exts &= (~((quint16)0x01 << (i + 3)));
                            GlobalConfig::ad_alarm_exts &= (~((quint16)0x01 << (i - 5)));
                        }
                    }
                }

                //检查静态张力是否在允许范围内
                //只有左6道钢丝和右6道钢丝才需要判断是否静态报警
                if ((i >= 0) && (i <= 11)) {
                    check_still_stress(i);
                }
            }

            //解析左开关量，右开关量是否外力报警，不存在静态报警
            //读取左开关量的值
            quint8 left_switch_alarm_state = DeviceControlUtil::ReadLeftSwitchAlarmState();
            if (left_switch_alarm_state == 0) {//报警
                GlobalConfig::ad_alarm_exts |= (1 << 14);
            } else {//正常
                GlobalConfig::ad_alarm_exts &= (~(1 << 14));
            }

            //读取右开关量的值
            quint8 right_switch_alarm_state = DeviceControlUtil::ReadRightSwitchAlarmState();
            if (right_switch_alarm_state == 0) {//报警
                GlobalConfig::ad_alarm_exts |= (1 << 6);
            } else {//正常
                GlobalConfig::ad_alarm_exts &= (~(1 << 6));
            }

            //读取本地门磁的状态
            quint8 gl_local_dk_status = DeviceControlUtil::ReadDoorkeepAlarmState();
            GlobalConfig::doorkeep_state = gl_local_dk_status || !gl_control_dk_status;

            //更新led状态
            update_led_status();

            //更新组合报警标志
            update_alarm_status();

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

    //1、左开关量、左6~1报警标志
    //过滤杆自身攀爬报警，杆自身攀爬报警不算在左防区报警内，单独开辟一个攀爬报警
    temp = HIGH((GlobalConfig::ad_alarm_exts | GlobalConfig::ad_alarm_base)) & HIGH(GlobalConfig::ad_sensor_mask_LR) & 0x7F;
    //判左侧组合报警
    if (temp == 0) {
        GlobalConfig::adl_alarm_flag = 0;   //无报警
    } else {
        GlobalConfig::adl_alarm_flag = 1;     //有报警
    }

    //2、右开关量、右6~1报警标志
    //过滤杆自身攀爬报警，杆自身攀爬报警不算在右防区报警内，单独开辟一个攀爬报警
    temp = LOW((GlobalConfig::ad_alarm_exts | GlobalConfig::ad_alarm_base)) & LOW(GlobalConfig::ad_sensor_mask_LR) & 0x7F;
    //判右侧组合报警
    if (temp == 0) {
        GlobalConfig::adr_alarm_flag = 0;   //无报警
    } else {
        GlobalConfig::adr_alarm_flag = 1;   //有报警
    }

    //3、杆自身攀爬报警
    temp = HIGH(GlobalConfig::ad_alarm_exts) & HIGH(GlobalConfig::ad_sensor_mask_LR) & 0x80;
    if (temp == 0) {
        GlobalConfig::zs_climb_alarm_flag = 0;//无报警
    } else {
        GlobalConfig::zs_climb_alarm_flag = 1;//有报警
    }

    //4、根据报警状态更新alarm_out_flag的值
    if (GlobalConfig::doorkeep_state) {//门磁报警
        GlobalConfig::alarm_out_flag |= ((1 << 3) | (1 << 4));
    } else {//门磁不报警
        if (GlobalConfig::adl_alarm_flag) {//左防区报警
            GlobalConfig::alarm_out_flag |= (1 << 3);
        } else {//左防区不报警
            GlobalConfig::alarm_out_flag &= ~(1 << 3);
        }

        if (GlobalConfig::adr_alarm_flag) {//右防区报警
            GlobalConfig::alarm_out_flag |= (1 << 4);
        } else {//右防区不报警
            GlobalConfig::alarm_out_flag &= ~(1 << 4);
        }
    }

    if (GlobalConfig::zs_climb_alarm_flag) {//杆自身攀爬报警
        GlobalConfig::alarm_out_flag |= (1 << 5);
    } else {//杆自身攀爬不报警
        GlobalConfig::alarm_out_flag &= ~(1 << 5);
    }

    //5、警号
    if (GlobalConfig::alarm_out_flag & 0x38) {//报警，警号需要鸣叫
        if (GlobalConfig::beep_flag == 0) {
            if (GlobalConfig::beep_during_temp > 0) {
                DeviceControlUtil::EnableBeep();
                GlobalConfig::beep_flag = 1;
                GlobalConfig::beep_timer = GlobalConfig::beep_during_temp;
            }
        }
    }

    //6、保存最后一次报警详细信息
    static quint16 pre_alarm_out_flag = 0;
    if (GlobalConfig::alarm_out_flag & 0x38) {//报警，需要保存最后一次报警详细信息
        if (GlobalConfig::alarm_out_flag > pre_alarm_out_flag) {
            pre_alarm_out_flag = GlobalConfig::alarm_out_flag;

            if (GlobalConfig::doorkeep_state) {//门磁报警
                for (int i = 0; i < 13; i++) {
                    GlobalConfig::AlarmDetailInfo.StaticBaseValue[i] = GlobalConfig::ad_chn_base[i].base;
                    GlobalConfig::AlarmDetailInfo.InstantSampleValue[i] = GlobalConfig::ad_chn_sample[i];
                }
            }

            if (HIGH(GlobalConfig::ad_alarm_exts) & HIGH(GlobalConfig::ad_sensor_mask_LR) & 0x40) {//左开关量报警
                for (int i = 0; i < 13; i++) {
                    GlobalConfig::AlarmDetailInfo.StaticBaseValue[i] = GlobalConfig::ad_chn_base[i].base;
                    GlobalConfig::AlarmDetailInfo.InstantSampleValue[i] = GlobalConfig::ad_chn_sample[i];
                }
            }

            if (LOW(GlobalConfig::ad_alarm_exts) & LOW(GlobalConfig::ad_sensor_mask_LR) & 0x40) {//右开关量报警
                for (int i = 0; i < 13; i++) {
                    GlobalConfig::AlarmDetailInfo.StaticBaseValue[i] = GlobalConfig::ad_chn_base[i].base;
                    GlobalConfig::AlarmDetailInfo.InstantSampleValue[i] = GlobalConfig::ad_chn_sample[i];
                }
            }

            GlobalConfig::AlarmDetailInfo.DoorKeepAlarm = GlobalConfig::doorkeep_state;
            GlobalConfig::AlarmDetailInfo.StaticAlarm   = GlobalConfig::ad_alarm_base;
            GlobalConfig::AlarmDetailInfo.ExternalAlarm = GlobalConfig::ad_alarm_exts;
        }
    } else {//不报警
        pre_alarm_out_flag = GlobalConfig::alarm_out_flag;
    }
}


//index：0-11分别对应为左1 右1 左2 右2 ... 左6 右6
void ParseMotorControlUartMsg::motor_start(quint8 index)
{
    QByteArray MotorRunCmd = QByteArray();
    MotorRunCmd[0] = 0x16;
    MotorRunCmd[1] = 0xFF;
    MotorRunCmd[2] = 0x01;
    MotorRunCmd[3] = 0x06;
    MotorRunCmd[4] = 0xE8;
    MotorRunCmd[5] = 0x04;
    MotorRunCmd[6] = 0xF0;
    MotorRunCmd[7] = index;      //电机号

    if ((GlobalConfig::gl_motor_channel_number == 4) ||
            (GlobalConfig::gl_motor_channel_number == 5)) {
        MotorRunCmd[8] = gl_5_motor_control_code[index];          //正转
    } else if (GlobalConfig::gl_motor_channel_number == 6) {
        MotorRunCmd[8] = gl_6_motor_control_code[index];          //正转
    }
    MotorRunCmd[9] = MOTOR_RUN_TIME;         //运转时间10s

    //计算校验和
    quint8 check_parity_sum = 0;
    for (int i = 1; i < 10; i++) {
        check_parity_sum += MotorRunCmd[i];
    }

    MotorRunCmd[10] = check_parity_sum; //校验和

    //发送给电机控制杆
    GlobalConfig::SendMsgToMotorControlBuffer.append(MotorRunCmd);
}

//index：0-11分别对应为左1 右1 左2 右2 ... 左6 右6
void ParseMotorControlUartMsg::motor_stop()
{
    QByteArray MotorRunCmd = QByteArray();
    MotorRunCmd[0] = 0x16;
    MotorRunCmd[1] = 0xFF;
    MotorRunCmd[2] = 0x01;
    MotorRunCmd[3] = 0x06;
    MotorRunCmd[4] = 0xE8;
    MotorRunCmd[5] = 0x04;
    MotorRunCmd[6] = 0xF0;
    MotorRunCmd[7] = 0;           //电机号可以随便填,因为同一时间只会有一个电机工作

    MotorRunCmd[8] = 0;           //停止
    MotorRunCmd[9] = 0;           //运转时间0s

    //计算校验和
    quint8 check_parity_sum = 0;
    for (int i = 1; i < 10; i++) {
        check_parity_sum += MotorRunCmd[i];
    }

    MotorRunCmd[10] = check_parity_sum; //校验和

    //发送给电机控制杆
    GlobalConfig::SendMsgToMotorControlBuffer.append(MotorRunCmd);
}
