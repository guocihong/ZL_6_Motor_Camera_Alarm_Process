#include "devicecamera.h"

DeviceCamera::DeviceCamera(QString VideoName, quint32 width, quint32 height, quint32 BPP, quint32 pixelformat, QObject *parent) :
    QObject(parent)
{
    this->VideoName = VideoName;
    this->width = width;
    this->height = height;
    this->pixelformat = pixelformat;
    yuyv_buff = (quint8 *)malloc(width * height * BPP / 8);

    img_buffers = NULL;
}

bool DeviceCamera::OpenCamera()
{
    fd = open(VideoName.toAscii().data(), O_RDWR  | O_NONBLOCK, 0);
    if(fd < 0){
        fprintf(stderr, "Cannot open '%s',errno = %d,error info = %s\n",VideoName.toAscii().data(), errno, strerror(errno));
        return false;
    }

    return true;
}

bool DeviceCamera::InitCameraDevice()
{
    //查询设备属性
    struct v4l2_capability cap;
    if(ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0){
        qDebug() << "Error in VIDIOC_QUERYCAP";
        close(fd);
        return false;
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
        qDebug() << "it is not a video capture device";
        close(fd);
        return false;
    }

    if(!(cap.capabilities & V4L2_CAP_STREAMING)){
        qDebug() << "It can not streaming";
        close(fd);
        return false;
    }

    qDebug() << "it is a video capture device";
    qDebug() << "It can streaming";
    qDebug() << "driver:" << cap.driver;
    qDebug() << "card:" << cap.card;
    qDebug() << "bus_info:" << cap.bus_info;
    qDebug() << "version:" << cap.version;

    if(cap.capabilities == 0x4000001){
        qDebug() << "capabilities:" << "V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING";
    }

    //设置视频输入源
    int input = 0;
    if(ioctl(fd, VIDIOC_S_INPUT, &input) < 0){
        qDebug() << "Error in VIDIOC_S_INPUT";
        close(fd);
        return false;
    }

    //查询并显示所有支持的格式
//    struct v4l2_fmtdesc fmtdesc;
//    fmtdesc.index = 0;
//    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//    printf("Support format:\n");
//    while(ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1){
//        printf("\tpixelformat = %c%c%c%c, description = %s\n",
//               fmtdesc.pixelformat & 0xFF,
//               (fmtdesc.pixelformat >> 8) & 0xFF,
//               (fmtdesc.pixelformat >> 16) & 0xFF,
//               (fmtdesc.pixelformat >> 24) & 0xFF,
//               fmtdesc.description);
//        fmtdesc.index++;
//    }

    //设置图片格式和分辨率
    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.pixelformat = pixelformat;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;

    if(ioctl(fd, VIDIOC_S_FMT, &fmt) < 0){
        printf("Error in VIDIOC_S_FMT,not support format pixelformat = %c%c%c%c",
               pixelformat & 0xFF,
               (pixelformat >> 8) & 0xFF,
               (pixelformat >> 16) & 0xFF,
               (pixelformat >> 24) & 0xFF);
        close(fd);
        return false;
    }

    //查看图片格式和分辨率,判断是否设置成功
    if(ioctl(fd, VIDIOC_G_FMT, &fmt) < 0){
        qDebug() << "Error in VIDIOC_G_FMT";
        close(fd);
        return false;
    }
    qDebug() << "width = " << fmt.fmt.pix.width;
    qDebug() << "height = " << fmt.fmt.pix.height;
    printf("pixelformat = %c%c%c%c\n",
           fmt.fmt.pix.pixelformat & 0xFF,
           (fmt.fmt.pix.pixelformat >> 8) & 0xFF,
           (fmt.fmt.pix.pixelformat >> 16) & 0xFF,
           (fmt.fmt.pix.pixelformat >> 24) & 0xFF);

    //设置帧格式
    struct v4l2_streamparm parm;
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1; // 分子。 例：1
    parm.parm.capture.timeperframe.denominator = 25; //分母。 例：30
    parm.parm.capture.capturemode = 0;

    if(ioctl(fd, VIDIOC_S_PARM, &parm) < 0){
        qDebug() << "Error in VIDIOC_S_PARM";
        close(fd);
        return false;
    }

    if(ioctl(fd, VIDIOC_G_PARM, &parm) < 0){
        qDebug() << "Error in VIDIOC_G_PARM";
        close(fd);
        return false;
    }
    qDebug() << "streamparm:\n\tnumerator =" << parm.parm.capture.timeperframe.numerator << "denominator =" << parm.parm.capture.timeperframe.denominator << "capturemode =" <<           parm.parm.capture.capturemode;

    //申请和管理缓冲区
    struct v4l2_requestbuffers reqbuf;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = BUFFER_NUMBER;

    if(ioctl(fd, VIDIOC_REQBUFS, &reqbuf) < 0){
        qDebug() << "Error in VIDIOC_REQBUFS";
        close(fd);
        return false;
    }

    img_buffers = (ImgBuffer *)calloc(BUFFER_NUMBER,sizeof(ImgBuffer));
    if(img_buffers == NULL){
        qDebug() << "Error in calloc";
        close(fd);
        return false;
    }

    struct v4l2_buffer buffer;
    for(int numBufs = 0; numBufs < BUFFER_NUMBER; numBufs++){
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = numBufs;

        if(ioctl(fd, VIDIOC_QUERYBUF, &buffer) < 0){

            qDebug() << "Error in VIDIOC_QUERYBUF";
            free(img_buffers);
            close(fd);
            img_buffers = NULL;
            return false;
        }
        qDebug() << "buffer.length =" << buffer.length;
        qDebug() << "buffer.bytesused =" << buffer.bytesused;
        img_buffers[numBufs].length = buffer.length;
        img_buffers[numBufs].start = (quint8 *)mmap (NULL,buffer.length,
PROT_READ | PROT_WRITE,MAP_SHARED,fd,buffer.m.offset);
        if(MAP_FAILED == img_buffers[numBufs].start){
            qDebug() << "Error in mmap";
            free(img_buffers);
            close(fd);
            img_buffers = NULL;
            return false;
        }

        //把缓冲帧放入队列
        if(ioctl(fd, VIDIOC_QBUF, &buffer) < 0){
            qDebug() << "Error in VIDIOC_QBUF";
            for(int i = 0; i <= numBufs; i++){
                munmap(img_buffers[i].start, img_buffers[i].length);
            }
            free(img_buffers);
            close(fd);
            img_buffers = NULL;
            return false;
        }
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd, VIDIOC_STREAMON, &type) < 0){
        qDebug() << "Error in VIDIOC_STREAMON";
        for(int i = 0; i < BUFFER_NUMBER; i++){
            munmap(img_buffers[i].start, img_buffers[i].length);
        }
        free(img_buffers);
        close(fd);
        img_buffers = NULL;
        return false;
    }

    return true;
}

void DeviceCamera::CleanupCaptureDevice()
{
    if(img_buffers != NULL){
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(ioctl(fd, VIDIOC_STREAMOFF, &type) < 0){
            qDebug() << "Error in VIDIOC_STREAMOFF";
        }

        for(int i = 0; i < BUFFER_NUMBER; i++){
            munmap(img_buffers[i].start, img_buffers[i].length);
        }
        close(fd);
        free(img_buffers);
        img_buffers = NULL;
        qDebug() << "CleanupCaptureDevice";
    }
}

bool DeviceCamera::ReadFrame()
{
    //等待摄像头采集到一桢数据
    for(;;){
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;

/*
1、设置了timeout的值之后，select在没有文件描述符可用的情况下，会等待这个timeout的时间，时间到了select返回0
2、如果timeout超时之前有文件描述符可用，则返回可用的数量，这时候的timeout则会依然计数，
因此如果想要每次都超时一定的时间那么在slelect返回>0的值之后要重新装填timeout的值一次
3、如果tv_sec和tv_usec都是0，那么就是超时时间为0，select就会立刻返回了。
4、如果timeout这里是个NULL，那么超时就未被启用，会一直阻塞在监视文件描述符的地方
*/

        int r = select(fd + 1,&fds,NULL,NULL,&tv);//超时读
//        int r = select(fd + 1,&fds,NULL,NULL,NULL);//阻塞读
        if(-1 == r){
            if(EINTR == errno)
                continue;
            qDebug() << "select error";
            return false;
        }else if(0 == r) {
            qDebug() << "select timeout";
            return false;
        }else{//采集到一张图片
//            qDebug() << "capture frame";
            break;
        }
    }

    //从缓冲区取出一个缓冲帧
    struct v4l2_buffer buffer;
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    if(ioctl(fd, VIDIOC_DQBUF, &buffer) < 0){
        qDebug() << "Error in VIDIOC_DQBUF";
        return false;
    }

    memcpy(yuyv_buff,(quint8 *)img_buffers[buffer.index].start,img_buffers[buffer.index].length);

    //将取出的缓冲帧放回缓冲区
    if(ioctl(fd, VIDIOC_QBUF, &buffer) < 0){
        qDebug() << "Error in VIDIOC_QBUF";
        return false;
    }

    return true;
}
