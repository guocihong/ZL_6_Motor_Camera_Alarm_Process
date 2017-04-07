#include "parsealarmhostuartmsg.h"
#include "uartutil.h"
#include "globalconfig.h"
#include "CommonSetting.h"

/*
 * 本类专门用来解析报警主机RS485发送过来的数据包
 */

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

        switch(cmd[4]) {
        case 0xE0://询问状态
            //返回：16 01 设备地址 01 00 校验和
            data[0] = 0x01;
            data[1] = 0x00;
            SendDataToAlarmHost(data);
            break;
        case 0xE1://询问地址
            //返回：16 01 设备地址 01 F1 校验和
            data[0] = 0x01;
            data[1] = 0xF1;
            SendDataToAlarmHost(data);
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

                data[27] = 0;
                data[28] = GlobalConfig::ip_addr;
                data[29] = GlobalConfig::beep_during_temp * SCHEDULER_TICK / 1000;
                data[30] = 0;
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

            case 0xF0://张力杆电机控制(主机->设备)
                //接收:16 FF 01 06 E8 04 F0 电机号(0-11) 正转(1)/反转(2)/停止(0) 运转时间(0-255) 校验和
                //需要转发给电机控制杆
                GlobalConfig::SendMsgToMotorControlBuffer.append(cmd);
                break;

            case 0xF1://采样值清零---->消除电路上的误差
                //需要转发给电机控制杆
                GlobalConfig::SendMsgToMotorControlBuffer.append(cmd);
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
                SendDataToAlarmHost(data);
                break;

            case 0xF8://读取详细报警信息
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
                SendDataToAlarmHost(data);
                break;
            }

            break;
        }
    }

    //启动延时返回数据包给报警主机
    UartUtil::newInstance()->Start();
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
