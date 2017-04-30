#-------------------------------------------------
#
# Project created by QtCreator 2017-03-27T12:45:59
# 电机张力视频复合主程序
#-------------------------------------------------

QT       += core gui xml network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ZL_6_Motor_Camera_Alarm_Process
TEMPLATE = app


SOURCES += main.cpp \
    globalconfig.cpp \
    devicecontrolutil.cpp \
    udp/udputil.cpp \
    uart/uartutil.cpp \
    uart/QextSerialPort/qextserialport_unix.cpp \
    uart/QextSerialPort/qextserialport.cpp \
    uart/parsealarmhostuartmsg.cpp \
    uart/parsemotorcontroluartmsg.cpp \
    tcp/tcputil.cpp \
    tcp/tcphelper.cpp \
    tcp/parsealarmhosttcpmsg.cpp \
    tcp/receivefileserver.cpp \
    tcp/receivefilethread.cpp \
    tcp/uploadalarmmsgtoalarmhostusingtcp.cpp \
    camera/devicecamera.cpp \
    camera/devicecamerathread.cpp \
    camera/getcamerainfo.cpp \
    camera/mainstream.cpp \
    camera/substream.cpp \
    scheduler.cpp \
    uart/rs485msgthread.cpp \
    tcp/checknetwork.cpp

HEADERS += \
    globalconfig.h \
    devicecontrolutil.h \
    udp/udputil.h \
    uart/uartutil.h \
    uart/QextSerialPort/qextserialport_global.h \
    uart/QextSerialPort/qextserialport_p.h \
    uart/QextSerialPort/qextserialport.h \
    uart/parsealarmhostuartmsg.h \
    uart/parsemotorcontroluartmsg.h \
    tcp/tcputil.h \
    tcp/tcphelper.h \
    tcp/parsealarmhosttcpmsg.h \
    tcp/receivefileserver.h \
    tcp/receivefilethread.h \
    tcp/uploadalarmmsgtoalarmhostusingtcp.h \
    camera/devicecamera.h \
    camera/devicecamerathread.h \
    camera/getcamerainfo.h \
    camera/mainstream.h \
    camera/substream.h \
    scheduler.h \
    uart/rs485msgthread.h \
    tcp/checknetwork.h


INCLUDEPATH += /usr/local/arm_ffmpeg/include
LIBS += -L/usr/local/arm_ffmpeg/lib -lavcodec \
        -L/usr/local/arm_ffmpeg/lib -lavfilter\
        -L/usr/local/arm_ffmpeg/lib -lavformat\
        -L/usr/local/arm_ffmpeg/lib -lswscale\
        -L/usr/local/arm_ffmpeg/lib -lavutil \
        -L/usr/local/arm_ffmpeg/lib -lswresample \
        -L/usr/local/arm_ffmpeg/lib -lpostproc

DESTDIR=bin
MOC_DIR=temp/moc
RCC_DIR=temp/rcc
UI_DIR=temp/ui
OBJECTS_DIR=temp/obj
