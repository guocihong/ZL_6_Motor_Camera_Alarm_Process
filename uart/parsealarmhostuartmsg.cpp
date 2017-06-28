#include "parsealarmhostuartmsg.h"
#include "rs485msgthread.h"
#include "globalconfig.h"
#include "CommonSetting.h"
#include "tcp/checknetwork.h"

ParseAlarmHostUartMsg *ParseAlarmHostUartMsg::instance = NULL;

ParseAlarmHostUartMsg::ParseAlarmHostUartMsg(QObject *parent) :
    QObject(parent)
{
}

void ParseAlarmHostUartMsg::Parse(void)
{
    ParseAlarmHostUartMsgTimer = new QTimer(this);
    ParseAlarmHostUartMsgTimer->setInterval(200);
    connect(ParseAlarmHostUartMsgTimer,SIGNAL(timeout()),this,SLOT(slotParseAlarmHostUartMsg()));
    ParseAlarmHostUartMsgTimer->start();
}

void ParseAlarmHostUartMsg::slotParseAlarmHostUartMsg(void)
{
    while(GlobalConfig::RecvVaildCompletePackageFromAlarmHostBuffer.size() == 0) {//没有数据等待处理
        return;
    }

    while(GlobalConfig::RecvVaildCompletePackageFromAlarmHostBuffer.size() > 0) {
        QByteArray cmd = GlobalConfig::RecvVaildCompletePackageFromAlarmHostBuffer.takeFirst();
        QByteArray data = QByteArray();
        QStringList DupList = QStringList();

        /*
         * 方法一：接收到报警主机的RS485广播命令，主动通过tcp发送一个OK数据包给报警主机，用来测试网络的好坏
         * 这种方法存在一个问题，如果报警主机RS485没有发送广播命令，那么就不会去检测网络的好坏，就会出现网络不通的情况
         *比如，报警主机停止服务，过了一段时间后，如果有设备网络不通，那么报警主机发出广播搜索命令，就会出现设备返回不全的问题
         *为了解决这个问题，现在改成主动通过tcp发送ok数据包给报警主机，如果设备网络不通，就会自动重启网卡，可以完美解决网络不通的情况
         *
        //判断是否是广播命令，主要用来测试网络是否正常，需要主动给报警主机发送一个ok包
        //如果发送失败，则认为网络不通，则将工作模式切换为RS485
        if (cmd[1] == CMD_ADDR_BC) {//广播命令
            //不管处于什么模式，都需要返回一个OK数据包，不然可能会碰到一上电网络不好的情况，这样就会导致网络永远不通
            //if (GlobalConfig::system_mode == GlobalConfig::TcpMode) {//只有处于tcp模式才需要测试网络的连通性
                CheckNetWork::newInstance()->SendOKAckToAlarmHost();
            //}
        }
        */

        switch(cmd[4]) {
        case 0xE0://询问状态
            //返回1：16 01 设备地址 02 F0 01/02/03 校验和
            //返回2：16 01 设备地址 01 00 校验和（该地址模块所有防区正常）
            if (GlobalConfig::ip_addr == CMD_ADDR_UNSOLV) {//本设备无有效地址,只回参数应答
                data[0] = 1;
                data[1] = 0xF1;
            } else {//有有效地址,回防区状态
                if ((GlobalConfig::alarm_out_flag & 0x38) == 0x00) {//2个防区均无报警
                    data[0] = 1;
                    data[1] = 0x00;
                } else {//有报警
                    data[0] = 2;
                    data[1] = 0xF0;
                    data[2] = (GlobalConfig::alarm_out_flag & 0x38) >> 3;
                }
            }

            GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = false;
            GlobalConfig::isDelayResponseAlarmHostRS485UnicastCmd = false;

            if (cmd[1] == GlobalConfig::ip_addr) {//本条命令只有单播才会返回，并且不需要延时返回，广播不会返回
                SendDataToAlarmHost(data);
            }
            break;

        case 0xE1://询问地址
            //返回：16 01 设备地址 01 F1 校验和
            data[0] = 0x01;
            data[1] = 0xF1;

            GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = false;
            GlobalConfig::isDelayResponseAlarmHostRS485UnicastCmd = false;

            if (cmd[1] == GlobalConfig::ip_addr) {//本条命令只有单播才会返回，并且不需要延时返回，广播不会返回
                SendDataToAlarmHost(data);
            }
            break;

        case 0xE3://设置设备返回延时时间
            //接收：16 FF 01 02 E3 延时时间 校验和
            GlobalConfig::gl_reply_tick = cmd[5];
            CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                         "AppGlobalConfig/gl_reply_tick",
                                         QString::number(GlobalConfig::gl_reply_tick));
            break;

        case 0xE4://读取设备返回延时时间
            //返回：16 01 设备地址 02 F4 延时时间 校验和
            data[0] = 0x02;
            data[1] = 0xF4;
            data[2] = GlobalConfig::gl_reply_tick;

            GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = true;
            GlobalConfig::isDelayResponseAlarmHostRS485UnicastCmd = false;

            SendDataToAlarmHost(data);
            break;

        case 0xE8:
            switch(cmd[6]) {
            case 0x10://读配置信息
                //返回：16 01 设备地址 1E E8 1C 08 张力mask左   张力mask右
                //静态张力允许下限高/低字节  静态张力允许上限高/低字节 报警阀值下浮比例
                //报警阀值允许上浮左1~8 报警阀值允许上浮右1~8	双/单防区(0-双  1-单)
                //码地址 声光报警时间  联动输出时间    校验和
                data[0] = 0x1E;
                data[1] = 0xE8;
                data[2] = 0x1C;
                data[3] = 0x08;

                data[4] = HIGH(GlobalConfig::ad_sensor_mask_LR);//张力mask左
                data[5] = LOW(GlobalConfig::ad_sensor_mask_LR); //张力mask右

                data[6] = HIGH(GlobalConfig::ad_still_dn);      //静态张力允许下限高字节
                data[7] = LOW(GlobalConfig::ad_still_dn);       //静态张力允许下限低字节

                data[8] = HIGH(GlobalConfig::ad_still_up);      //静态张力允许上限高字节
                data[9] = LOW(GlobalConfig::ad_still_up);       //静态张力允许上限低字节

                data[10] = GlobalConfig::system_status;

                for (int i = 0; i < 6; i++) {                   //左1~6的报警阀值
                    data[11 + i] = GlobalConfig::ad_still_Dup[i];
                }

                //左开关量
                data[17] = 0;

                //杆自身
                data[18] = GlobalConfig::ad_still_Dup[12];

                for (int i = 0; i < 6; i++) {                   //右1~6的报警阀值
                    data[19 + i] = GlobalConfig::ad_still_Dup[6 + i];
                }

                //右开关量
                data[25] = 0;

                //杆自身
                data[26] = GlobalConfig::ad_still_Dup[12];

                //双/单防区(0-双  1-单)
                data[27] = 0;

                //拨码地址
                data[28] = GlobalConfig::ip_addr;

                //声光报警时间
                data[29] = GlobalConfig::beep_during_temp * SCHEDULER_TICK / 1000;

                //当前正在调整钢丝的索引
                data[30] = GlobalConfig::gl_chnn_index;

                //本条命令不管广播还是单播都需要返回
                //因为生产车间测试工具发送的所有命令都是广播，但是报警主机本条命令是单播
                //本条命令返回需要延时100ms，因为报警主机/生产车间是好几条命令一起发下来，不延时返回会出问题
                GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = false;
                GlobalConfig::isDelayResponseAlarmHostRS485UnicastCmd = true;

                SendDataToAlarmHost(data);

                break;

            case 0x12://读报警信息
                //返回：16 01 设备地址 0A E8 08 1A  [张力掩码左] [张力掩码右] [外力报警左]  [外力报警右]
                //[静态张力报警左] [静态张力报警右] [门磁]  校验和
                data[0] = 0x0A;
                data[1] = 0xE8;
                data[2] = 0x08;
                data[3] = 0x1A;

                data[4] = HIGH(GlobalConfig::ad_sensor_mask_LR);//张力掩码左
                data[5] = LOW(GlobalConfig::ad_sensor_mask_LR); //张力掩码右

                data[6] = HIGH(GlobalConfig::ad_alarm_exts);    //外力报警左
                data[7] = LOW(GlobalConfig::ad_alarm_exts);     //外力报警右

                data[8] = HIGH(GlobalConfig::ad_alarm_base);    //静态张力报警左
                data[9] = LOW(GlobalConfig::ad_alarm_base);     //静态张力报警右

                data[10] = GlobalConfig::doorkeep_state;        //门磁

                //本条命令不管广播还是单播都需要返回
                //因为生产车间测试工具发送的所有命令都是广播，但是报警主机本条命令是单播
                //本条命令返回需要延时100ms，因为报警主机/生产车间是好几条命令一起发下来，不延时返回会出问题
                GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = false;
                GlobalConfig::isDelayResponseAlarmHostRS485UnicastCmd = true;

                SendDataToAlarmHost(data);

                break;

            case 0x14://读瞬间张力
                //返回：16 01 设备地址 23 E8 21 1C 左1~8 右1~8 校验和
                data[0] = 0x23;
                data[1] = 0xE8;
                data[2] = 0x21;
                data[3] = 0x1C;

                for (int i = 0; i < 6; i++) {//左1~6的瞬间张力
                    data[4 + (i << 1)] = HIGH(GlobalConfig::ad_chn_sample[i]);
                    data[5 + (i << 1)] = LOW(GlobalConfig::ad_chn_sample[i]);
                }

                //左开关量
                data[16] = 0;
                data[17] = 0;

                //杆自身
                data[18] = HIGH(GlobalConfig::ad_chn_sample[12]);
                data[19] = LOW(GlobalConfig::ad_chn_sample[12]);


                for (int i = 0; i < 6; i++) {//右1~6的瞬间张力
                    data[20 + (i << 1)] = HIGH(GlobalConfig::ad_chn_sample[6 + i]);
                    data[21 + (i << 1)] = LOW(GlobalConfig::ad_chn_sample[6 + i]);
                }

                //右开关量
                data[32] = 0;
                data[33] = 0;

                //杆自身
                data[34] = HIGH(GlobalConfig::ad_chn_sample[12]);
                data[35] = LOW(GlobalConfig::ad_chn_sample[12]);

                //本条命令不管广播还是单播都需要返回
                //因为生产车间测试工具发送的所有命令都是广播，但是报警主机本条命令是单播
                //本条命令返回需要延时100ms，因为报警主机/生产车间是好几条命令一起发下来，不延时返回会出问题
                GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = false;
                GlobalConfig::isDelayResponseAlarmHostRS485UnicastCmd = true;

                SendDataToAlarmHost(data);

                break;

            case 0x15://读静态张力基准
                //返回：16 01 设备地址 23 E8 21 1D 左1~8 右1~8 校验和
                data[0] = 0x23;
                data[1] = 0xE8;
                data[2] = 0x21;
                data[3] = 0x1D;

                for (int i = 0; i < 6; i++) {//左1~6的瞬间张力
                    data[4 + (i << 1)] = HIGH(GlobalConfig::ad_chn_base[i].base);
                    data[5 + (i << 1)] = LOW(GlobalConfig::ad_chn_base[i].base);
                }

                //左开关量
                data[16] = 0;
                data[17] = 0;

                //杆自身
                data[18] = HIGH(GlobalConfig::ad_chn_base[12].base);
                data[19] = LOW(GlobalConfig::ad_chn_base[12].base);


                for (int i = 0; i < 6; i++) {//右1~6的瞬间张力
                    data[20 + (i << 1)] = HIGH(GlobalConfig::ad_chn_base[6 + i].base);
                    data[21 + (i << 1)] = LOW(GlobalConfig::ad_chn_base[6 + i].base);
                }

                //右开关量
                data[32] = 0;
                data[33] = 0;

                //杆自身
                data[34] = HIGH(GlobalConfig::ad_chn_base[12].base);
                data[35] = LOW(GlobalConfig::ad_chn_base[12].base);

                //本条命令不管广播还是单播都需要返回
                //因为生产车间测试工具发送的所有命令都是广播，但是报警主机本条命令是单播
                //本条命令返回需要延时100ms，因为报警主机/生产车间是好几条命令一起发下来，不延时返回会出问题
                GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = false;
                GlobalConfig::isDelayResponseAlarmHostRS485UnicastCmd = true;

                SendDataToAlarmHost(data);

                break;

            case 0x40://设置静态张力值范围
                //接收：16 FF 01 07 E8 05 40 下限值高/低字节  上限值高/低字节 校验和
                GlobalConfig::ad_still_dn = (cmd[7] << 8) + cmd[8];
                GlobalConfig::ad_still_up = (cmd[9] << 8) + cmd[10];
                CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                             "AppGlobalConfig/ad_still_dn",
                                             QString::number(GlobalConfig::ad_still_dn));

                CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                             "AppGlobalConfig/ad_still_up",
                                             QString::number(GlobalConfig::ad_still_up));
                break;

            case 0x50://设置报警阀值
                //接收：16 FF 01 14 E8 12 50 42 左1~8    右1~8   校验和
                //左1~6
                for (int i = 0; i < 6; i++) {
                    GlobalConfig::ad_still_Dup[i] = cmd[8 + i];
                }

                //右1~6
                for (int i = 0; i < 6; i++) {
                    GlobalConfig::ad_still_Dup[6 + i] = cmd[16 + i];
                }

                //杆自身
                GlobalConfig::ad_still_Dup[12] = cmd[23];


                for (int i = 0; i < 13; i++) {
                    DupList << QString::number(GlobalConfig::ad_still_Dup[i]);
                }

                CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                             "AppGlobalConfig/ad_still_Dup",
                                             DupList.join("|"));
                break;

            case 0x60://设置声光报警时间
                //接收：16 FF 01 04 E8 02 60 设置值 校验和
                GlobalConfig::beep_during_temp = cmd[7] * 1000 / SCHEDULER_TICK;

                CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                             "AppGlobalConfig/beep_during_temp",
                                             QString::number(cmd[7]));
                break;
            case 0x70://设置联动时间
                //do nothing
                break;

            case 0xF0://控制电机正反转
                //接收:16 FF 01 06 E8 04 F0 电机号(0-11) 正转(1)/反转(2)/停止(0) 运转时间(0-255) 校验和
                //需要转发给电机控制杆
                GlobalConfig::SendMsgToMotorControlBuffer.append(cmd);

                //这里必须，不然手动松紧钢丝时，静态基准和瞬间张力不能同步更新
                GlobalConfig::gl_chnn_index = cmd[7];//电机编号

                //这几个变量只有进入实时检测阶段才会用到
                if (GlobalConfig::system_status == GlobalConfig::SYS_CHECK) {
                    GlobalConfig::isEnterAutoAdjustMotorMode = false;
                    GlobalConfig::isEnterManualAdjustMotorMode = true;

                    //手动调整模式下，不检测钢丝是否被剪断
                    GlobalConfig::isCheckWireCut = false;

                    qDebug() << "enter manual adjust motor mode";
                    qDebug() << "disable check wire cut";
                }

                break;

            case 0xF1://采样值清零---->消除电路上的误差
                //需要转发给电机控制杆
                GlobalConfig::SendMsgToMotorControlBuffer.append(cmd);

                /******************************************************************/
                //1、在加级联的情况下，需要在现场通过报警主机先进行采样值恢复，然后在执行采样值清零
                //2、生产车间进行采样值恢复和采样值清零都是通过广播模式设置
                //3、现场通过报警主机进行采样值恢复和采样值清零是通过单播模式设置

                //只有报警主机发出的采样值清零命令才生效
                if (cmd[1] == GlobalConfig::ip_addr) {//单播模式
                    quint16 sample_value_sum = 0;
                    for (int i = 0; i < 32; i++) {
                        sample_value_sum += cmd[7 + i];
                    }

                    if (sample_value_sum == 0) {//本条命令是进行采样值恢复
                        GlobalConfig::is_sample_clear = 0;

                        CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                                     "AppGlobalConfig/is_sample_clear",
                                                     QString::number(GlobalConfig::is_sample_clear));
                    } else {//本条命令是进行采样值清零
                        GlobalConfig::check_sample_clear_tick = 3000 / SCHEDULER_TICK;
                    }
                } else if (cmd[1] == CMD_ADDR_BC) {//广播模式
                    //生产车间发出的采样值清零命令不生效
                    //什么都不做
                }

                /******************************************************************/
                break;

            case 0xF2://设置采样点数---->连续采样多少个点以确定钢丝是否比较松
                break;
            case 0xF3://读钢丝是否剪断以及是否处于调整钢丝模式
                //返回：16 01 设备地址 06 E8 04 1E [钢丝剪断左] [钢丝剪断右] [是否处于调整钢丝模式] 校验和
                data[0] = 0x06;
                data[1] = 0xE8;
                data[2] = 0x04;
                data[3] = 0x1E;
                data[4] = HIGH(GlobalConfig::ad_chnn_wire_cut);
                data[5] = LOW(GlobalConfig::ad_chnn_wire_cut);
                data[6] = GlobalConfig::gl_motor_adjust_flag;

                //本条命令不管广播还是单播都需要返回
                //因为生产车间测试工具发送的所有命令都是广播，但是报警主机本条命令是单播
                //本条命令返回需要延时100ms，因为报警主机/生产车间是好几条命令一起发下来，不延时返回会出问题
                GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = false;
                GlobalConfig::isDelayResponseAlarmHostRS485UnicastCmd = true;

                SendDataToAlarmHost(data);

                break;

            case 0xF4://设置电机张力通道数：4道电机张力、5道电机张力、6道电机张力
                //接收：16 FF 01 04 E8 02 F4 通道数(4/5/6) 校验和
                GlobalConfig::gl_motor_channel_number = cmd[7];
                CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                             "AppGlobalConfig/gl_motor_channel_number",
                                             QString::number(GlobalConfig::gl_motor_channel_number));
                break;

            case 0xF5://读电机张力通道数
                //返回：16 01 设备地址 04 E8 02 1F 通道数(4/5/6) 校验和
                data[0] = 0x04;
                data[1] = 0xE8;
                data[2] = 0x02;
                data[3] = 0x1F;
                data[4] = GlobalConfig::gl_motor_channel_number;

                //本条命令不管广播还是单播都需要返回
                //因为生产车间测试工具发送的所有命令都是广播，但是报警主机本条命令是单播
                //本条命令返回需要延时100ms，因为报警主机/生产车间是好几条命令一起发下来，不延时返回会出问题
                GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = false;
                GlobalConfig::isDelayResponseAlarmHostRS485UnicastCmd = true;

                SendDataToAlarmHost(data);

                break;

            case 0xF6://设置电机张力是否添加级联
                //接收：16 FF 01 04 E8 02 F6 0/1 校验和
                GlobalConfig::is_motor_add_link = cmd[7];
                CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                             "AppGlobalConfig/is_motor_add_link",
                                             QString::number(GlobalConfig::is_motor_add_link));
                break;

            case 0xF7://读电机张力是否添加级联
                //返回：16 01 设备地址 04 E8 02 20 0/1 校验和
                data[0] = 0x04;
                data[1] = 0xE8;
                data[2] = 0x02;
                data[3] = 0x20;
                data[4] = GlobalConfig::is_motor_add_link;

                //本条命令不管广播还是单播都需要返回
                //因为生产车间测试工具发送的所有命令都是广播，但是报警主机本条命令是单播
                //本条命令返回需要延时100ms，因为报警主机/生产车间是好几条命令一起发下来，不延时返回会出问题
                GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = false;
                GlobalConfig::isDelayResponseAlarmHostRS485UnicastCmd = true;

                SendDataToAlarmHost(data);

                break;

            case 0xF8://读取详细报警信息
                GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = false;
                GlobalConfig::isDelayResponseAlarmHostRS485UnicastCmd = false;

                //本条命令肯定是单播命令，不可能是广播命令
                get_alarm_detail_info();

                //默认情况下，报警会保持15s，如果网络是正常，主动上传报警信息后，报警会继续保持3s，然后报警恢复
                //如果网络不通，报警主机通过RS485读取本设备的报警详细信息后，报警会立即恢复
                for (int i = 0; i < 13; i++) {
                    GlobalConfig::ad_alarm_tick[i] = 0;
                }
                break;

            case 0xF9://设置平均值点数-->采样多少个点求平均值
                //接收：16 FF 01 04 E8 02 F9 [平均值点数(范围为4~8)] 校验和
                //需要转发给电机张力控制杆
                GlobalConfig::SendMsgToMotorControlBuffer.append(cmd);

                //返回：16 01 设备地址 04 E8 02 21 [平均值点数] 校验和
                data[0] = 0x04;
                data[1] = 0xE8;
                data[2] = 0x02;
                data[3] = 0x21;
                data[4] = cmd[7];

                if (cmd[1] == CMD_ADDR_BC) {//广播命令需要延时返回
                    GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = true;
                } else if (cmd[1] == GlobalConfig::ip_addr){//单播命令不需要延时返回
                    GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = false;
                }

                GlobalConfig::isDelayResponseAlarmHostRS485UnicastCmd = false;

                //本条命令既可以通过广播设置，也可以通过单播设置
                //通过广播设置，需要延时返回
                //通过单播设置，不需要延时，可以立即返回
                SendDataToAlarmHost(data);
                break;

            case 0xFA://设置报警点数-->连续多少个点报警才判定为报警
                //接收：16 FF 01 04 E8 02 FA [报警点数(范围为4~8)] 校验和
                GlobalConfig::alarm_point_num = cmd[7];
                CommonSetting::WriteSettings(GlobalConfig::ConfigFileName,
                                             "AppGlobalConfig/alarm_point_num",
                                             QString::number(GlobalConfig::alarm_point_num));

                //返回：16 01 设备地址 04 E8 02 22 [报警点数] 校验和
                data[0] = 0x04;
                data[1] = 0xE8;
                data[2] = 0x02;
                data[3] = 0x22;
                data[4] = GlobalConfig::alarm_point_num;

                if (cmd[1] == CMD_ADDR_BC) {//广播命令需要延时返回
                    GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = true;
                } else if (cmd[1] == GlobalConfig::ip_addr){//单播命令不需要延时返回
                    GlobalConfig::isDelayResponseAlarmHostRS485BroadcastCmd = false;
                }

                GlobalConfig::isDelayResponseAlarmHostRS485UnicastCmd = false;

                //本条命令既可以通过广播设置，也可以通过单播设置
                //通过广播设置，需要延时返回
                //通过单播设置，不需要延时，可以立即返回
                SendDataToAlarmHost(data);
                break;
            }

            break;
        }
    }

    //启动延时返回数据包给报警主机
    RS485MsgThread::newInstance()->Start();
}

void ParseAlarmHostUartMsg::SendDataToAlarmHost(QByteArray data)
{
    QByteArray cmd = QByteArray();

    cmd[0] = 0x16;
    cmd[1] = 0x01;
    cmd[2] = GlobalConfig::ip_addr;

    int size = data.size();
    for (int i = 0; i < size; i++) {
        cmd[i + 3] = data[i];
    }

    //计算校验和
    quint8 chk_sum = 0;
    for (int i = 1; i < (size + 3) ; i++) {
        chk_sum += cmd[i];
    }

    cmd[size + 3] = chk_sum;

    GlobalConfig::SendMsgToAlarmHostBuffer.append(cmd);
}

void ParseAlarmHostUartMsg::get_alarm_detail_info()
{
    QByteArray data = QByteArray();

    //1、保存配置信息
    data[0] = 0x1E;
    data[1] = 0xE8;
    data[2] = 0x1C;
    data[3] = 0x08;

    data[4] = HIGH(GlobalConfig::ad_sensor_mask_LR);//张力mask左
    data[5] = LOW(GlobalConfig::ad_sensor_mask_LR); //张力mask右

    data[6] = HIGH(GlobalConfig::ad_still_dn);      //静态张力允许下限高字节
    data[7] = LOW(GlobalConfig::ad_still_dn);       //静态张力允许下限低字节

    data[8] = HIGH(GlobalConfig::ad_still_up);      //静态张力允许上限高字节
    data[9] = LOW(GlobalConfig::ad_still_up);       //静态张力允许上限低字节

    data[10] = GlobalConfig::system_status;

    for (int i = 0; i < 6; i++) {                   //左1~6的报警阀值
        data[11 + i] = GlobalConfig::ad_still_Dup[i];
    }

    //左开关量
    data[17] = 0;

    //杆自身
    data[18] = GlobalConfig::ad_still_Dup[12];

    for (int i = 0; i < 6; i++) {                   //右1~6的报警阀值
        data[19 + i] = GlobalConfig::ad_still_Dup[6 + i];
    }

    //右开关量
    data[25] = 0;

    //杆自身
    data[26] = GlobalConfig::ad_still_Dup[12];

    data[27] = 0;
    data[28] = GlobalConfig::ip_addr;
    data[29] = GlobalConfig::beep_during_temp * SCHEDULER_TICK / 1000;
    data[30] = GlobalConfig::gl_chnn_index;
    SendDataToAlarmHost(data);

    data.clear();

    //2、保存报警信息
    //返回：16 01 设备地址 0A E8 08 1A  [张力掩码左] [张力掩码右] [外力报警左]  [外力报警右]
    //[静态张力报警左] [静态张力报警右] [门磁]  校验和
    data[0] = 0x0A;
    data[1] = 0xE8;
    data[2] = 0x08;
    data[3] = 0x1A;

    data[4] = HIGH(GlobalConfig::ad_sensor_mask_LR);//张力掩码左
    data[5] = LOW(GlobalConfig::ad_sensor_mask_LR); //张力掩码右

    data[6] = HIGH(GlobalConfig::AlarmDetailInfo.ExternalAlarm);    //外力报警左
    data[7] = LOW(GlobalConfig::AlarmDetailInfo.ExternalAlarm);     //外力报警右

    data[8] = HIGH(GlobalConfig::AlarmDetailInfo.StaticAlarm);      //静态张力报警左
    data[9] = LOW(GlobalConfig::AlarmDetailInfo.StaticAlarm);       //静态张力报警右

    data[10] = GlobalConfig::AlarmDetailInfo.DoorKeepAlarm;         //门磁
    SendDataToAlarmHost(data);

    data.clear();

    //3、保存瞬间张力
    //返回：16 01 设备地址 23 E8 21 1C 左1~8 右1~8 校验和
    data[0] = 0x23;
    data[1] = 0xE8;
    data[2] = 0x21;
    data[3] = 0x1C;

    for (int i = 0; i < 6; i++) {//左1~6的瞬间张力
        data[4 + (i << 1)] = HIGH(GlobalConfig::AlarmDetailInfo.InstantSampleValue[i]);
        data[5 + (i << 1)] = LOW(GlobalConfig::AlarmDetailInfo.InstantSampleValue[i]);
    }

    //左开关量
    data[16] = 0;
    data[17] = 0;

    //杆自身
    data[18] = HIGH(GlobalConfig::AlarmDetailInfo.InstantSampleValue[12]);
    data[19] = LOW(GlobalConfig::AlarmDetailInfo.InstantSampleValue[12]);


    for (int i = 0; i < 6; i++) {//右1~6的瞬间张力
        data[20 + (i << 1)] = HIGH(GlobalConfig::AlarmDetailInfo.InstantSampleValue[6 + i]);
        data[21 + (i << 1)] = LOW(GlobalConfig::AlarmDetailInfo.InstantSampleValue[6 + i]);
    }

    //右开关量
    data[32] = 0;
    data[33] = 0;

    //杆自身
    data[34] = HIGH(GlobalConfig::AlarmDetailInfo.InstantSampleValue[12]);
    data[35] = LOW(GlobalConfig::AlarmDetailInfo.InstantSampleValue[12]);

    SendDataToAlarmHost(data);

    data.clear();

    //4、存静态基准
    //返回：16 01 设备地址 23 E8 21 1D 左1~8 右1~8 校验和
    data[0] = 0x23;
    data[1] = 0xE8;
    data[2] = 0x21;
    data[3] = 0x1D;

    for (int i = 0; i < 6; i++) {//左1~6的瞬间张力
        data[4 + (i << 1)] = HIGH(GlobalConfig::AlarmDetailInfo.StaticBaseValue[i]);
        data[5 + (i << 1)] = LOW(GlobalConfig::AlarmDetailInfo.StaticBaseValue[i]);
    }

    //左开关量
    data[16] = 0;
    data[17] = 0;

    //杆自身
    data[18] = HIGH(GlobalConfig::AlarmDetailInfo.StaticBaseValue[12]);
    data[19] = LOW(GlobalConfig::AlarmDetailInfo.StaticBaseValue[12]);


    for (int i = 0; i < 6; i++) {//右1~6的瞬间张力
        data[20 + (i << 1)] = HIGH(GlobalConfig::AlarmDetailInfo.StaticBaseValue[6 + i]);
        data[21 + (i << 1)] = LOW(GlobalConfig::AlarmDetailInfo.StaticBaseValue[6 + i]);
    }

    //右开关量
    data[32] = 0;
    data[33] = 0;

    //杆自身
    data[34] = HIGH(GlobalConfig::AlarmDetailInfo.StaticBaseValue[12]);
    data[35] = LOW(GlobalConfig::AlarmDetailInfo.StaticBaseValue[12]);

    SendDataToAlarmHost(data);

    data.clear();

    //5、钢丝是否剪断
    //返回：16 01 设备地址 06 E8 04 1E [钢丝剪断左] [钢丝剪断右] [是否处于调整钢丝模式] 校验和
    data[0] = 0x06;
    data[1] = 0xE8;
    data[2] = 0x04;
    data[3] = 0x1E;
    data[4] = HIGH(GlobalConfig::ad_chnn_wire_cut);
    data[5] = LOW(GlobalConfig::ad_chnn_wire_cut);
    data[6] = GlobalConfig::gl_motor_adjust_flag;
    SendDataToAlarmHost(data);
}
