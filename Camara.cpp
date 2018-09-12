#include "Camera.h"

int g_LibConnect_n=0;//相机Lib引用计数  每个相机Inite会使得次值+1 close会使得此值-1 非线程安全

int Camera::Init(char* No)
{
    if(CameraState_)
    {
        printf("Warning> Camera No:%s has inited.\r\n",No);
        return -1;
    }
    GX_STATUS status = GX_STATUS_SUCCESS;
    int ret = 0;
    GX_OPEN_PARAM open_param;
    //初始化设备打开参数，默认打开序号为１的设备
    open_param.accessMode = GX_ACCESS_EXCLUSIVE;
    open_param.openMode = GX_OPEN_INDEX;
    open_param.pszContent = No;

    //如果是第一个调用lib的相机，首先链接Lib
    if(0==g_LibConnect_n)
    {
        //链接Lib
        status = GXInitLib();
        if(status != GX_STATUS_SUCCESS)
        {
            GetErrorString(status);
            printf("Erro>Inite Camera No:%s Fail.\r\n",No);
            return -1;
        }
    }

    g_LibConnect_n+=1;//连接库如果成功 全局Lib计数 加一

    uint32_t device_number = 0;
    status = GXUpdateDeviceList(&device_number, 1000);//获取设备台数

    if(status != GX_STATUS_SUCCESS)
    {
        GetErrorString(status);
        return -1;
    }

    if(device_number <= 0)
    {
        printf("Erro> No device Connected\r\n");
        return -1;
    }
    //打开摄像头
    status = GXOpenDevice(&open_param, &hdevice_);
    if(status == GX_STATUS_SUCCESS)
    {
        printf("Open Camera No:%s Success.\r\n",No);
        CameraState_=Camera_State_Inited;
    }
    else
    {
        printf("Erro>Open Camera No:%s Fail.\r\n",No);
        return -1;
    }

    while(1)
    {
    status = GXSetEnum(hdevice_, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS);//设置采集模式为连续采集
        if(status!=GX_STATUS_SUCCESS)break;
    status = GXSetEnum(hdevice_, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_ON);//设置触发开关为ON
        if(status!=GX_STATUS_SUCCESS)break;
    status = GXSetEnum(hdevice_, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_SOFTWARE);//设置触发源为软触发
        if(status!=GX_STATUS_SUCCESS)break;
    status = GXGetEnum(hdevice_, GX_ENUM_PIXEL_FORMAT, &pixel_format_);//获取相机输出数据的颜色格式
        if(status!=GX_STATUS_SUCCESS)break;
    status = GXGetEnum(hdevice_, GX_ENUM_PIXEL_COLOR_FILTER, &color_filter_);//相机采集图像为彩色还是黑白
        if(status!=GX_STATUS_SUCCESS)break;
    break;
    }
    if(status!=GX_STATUS_SUCCESS)GetErrorString(status);
    else printf("Camara Set Ok\r\n");

    //为采集做准备 为获取的图像分配存储空间
    int64_t payload_size = 0;
    status = GXGetInt(hdevice_, GX_INT_PAYLOAD_SIZE, &payload_size);

    if(!frame_data_.pImgBuf)
        frame_data_.pImgBuf = malloc(payload_size);
    if(!raw8_buffer_)//将非8位raw数据转换成8位数据的时候的中转缓冲buffer
        raw8_buffer_ = malloc(payload_size);
    if(!rgb_frame_data_)
        rgb_frame_data_ = malloc(payload_size * 3);

    status = GXGetBufferLength(hdevice_, GX_BUFFER_FRAME_INFORMATION, &frameinfo_datasize_);//获取帧信息长度并申请帧信息数据空间
    if(status!=GX_STATUS_SUCCESS)GetErrorString(status);

    if(frameinfo_datasize_ > 0)
    {
        frameinfo_data_ = malloc(frameinfo_datasize_);
    }
    if(!(frame_data_.pImgBuf && raw8_buffer_ && rgb_frame_data_ ))
    {
        printf("Erro>Failed to allocate memory\n");
        return -1;
    }

    frame_.create(1024,1280,CV_8UC3);
    //frame_=cv::Mat(frame_data_.nWidth,frame_data_.nHeight,CV_8UC3,cv::Scalar(0,0,0)).clone();

    return 0;
}
int Camera::ReInit()
{
 return 0;
}
int Camera::Start()
{
    GX_STATUS status = GX_STATUS_SUCCESS;
    status = GXSendCommand(hdevice_, GX_COMMAND_ACQUISITION_START);//发送开采命令
    if(status != GX_STATUS_SUCCESS)GetErrorString(status);
}
int Camera::Stop()
{
    GX_STATUS status = GXSendCommand(hdevice_, GX_COMMAND_ACQUISITION_STOP);//停止采集
}
int Camera::Triger()
{
    GX_STATUS status = GX_STATUS_SUCCESS;
    status = GXSendCommand(hdevice_, GX_COMMAND_TRIGGER_SOFTWARE);
    if(status != GX_STATUS_SUCCESS)GetErrorString(status);

}
int Camera::GetImg()
{
    GX_STATUS status = GXGetImage(hdevice_, &frame_data_, 100);
    if(status == GX_STATUS_SUCCESS)
    {
        ProcessData(frame_data_.pImgBuf,
                    raw8_buffer_,
                    rgb_frame_data_,
                    frame_data_.nWidth,
                    frame_data_.nHeight,
                    pixel_format_,
                    color_filter_);//将Raw数据处理成RGB数据
        if(frame_.cols!=frame_data_.nWidth || frame_.rows!=frame_data_.nHeight)
            cv::resize(frame_,frame_,cv::Size(frame_data_.nWidth,frame_data_.nHeight));

        frame_.data=(unsigned char *)rgb_frame_data_;
        cv::cvtColor(frame_,frame_,cv::COLOR_RGB2BGR);
    }
    else GetErrorString(status);
}
int Camera::SaveImg()
{
    cv::imwrite("outimg.jpg",frame_);
    return 0;
}
int Camera::Close()
{
    //释放camera资源
    GX_STATUS status = GXCloseDevice(hdevice_);
    status = GXSendCommand(hdevice_, GX_COMMAND_ACQUISITION_STOP);//停止采集

    //释放buffer
    if(frame_data_.pImgBuf != NULL)
    {
        free(frame_data_.pImgBuf);
        frame_data_.pImgBuf = NULL;
    }

    if(raw8_buffer_ != NULL)
    {
        free(raw8_buffer_);
        raw8_buffer_ = NULL;
    }

    if(rgb_frame_data_ != NULL)
    {
        free(rgb_frame_data_);
        rgb_frame_data_ = NULL;
    }

    if(frameinfo_data_ != NULL)
    {
        free(frameinfo_data_);
        frameinfo_data_ = NULL;
    }

    g_LibConnect_n-=1;
    if(0==g_LibConnect_n)//最后一个camera 调用close 释放lib
    {
        status = GXCloseLib();//关闭Lib
    }
    return 0;
}

void Camera::ProcessData(void *image_buffer, void *image_raw8_buffer, void *image_rgb_buffer, int image_width, int image_height, int pixel_format, int color_filter)
{
    switch(pixel_format)
    {
        //当数据格式为12位时，位数转换为4-11
        case GX_PIXEL_FORMAT_BAYER_GR12:
        case GX_PIXEL_FORMAT_BAYER_RG12:
        case GX_PIXEL_FORMAT_BAYER_GB12:
        case GX_PIXEL_FORMAT_BAYER_BG12:
            //将12位格式的图像转换为8位格式
            DxRaw16toRaw8(image_buffer, image_raw8_buffer, image_width, image_height, DX_BIT_4_11);
            //将Raw8图像转换为RGB图像以供显示
            DxRaw8toRGB24(image_raw8_buffer, image_rgb_buffer, image_width, image_height, RAW2RGB_NEIGHBOUR,
                          DX_PIXEL_COLOR_FILTER(color_filter), false);
            break;

            //当数据格式为10位时，位数转换为2-9
        case GX_PIXEL_FORMAT_BAYER_GR10:
        case GX_PIXEL_FORMAT_BAYER_RG10:
        case GX_PIXEL_FORMAT_BAYER_GB10:
        case GX_PIXEL_FORMAT_BAYER_BG10:
            //将10位格式的图像转换为8位格式,有效位数2-9
            DxRaw16toRaw8(image_buffer, image_raw8_buffer, image_width, image_height, DX_BIT_2_9);
            //将Raw8图像转换为RGB图像以供显示
            DxRaw8toRGB24(image_raw8_buffer, image_rgb_buffer, image_width, image_height,RAW2RGB_NEIGHBOUR,
                          DX_PIXEL_COLOR_FILTER(color_filter),false);
            break;

        case GX_PIXEL_FORMAT_BAYER_GR8:
        case GX_PIXEL_FORMAT_BAYER_RG8:
        case GX_PIXEL_FORMAT_BAYER_GB8:
        case GX_PIXEL_FORMAT_BAYER_BG8:
            //将Raw8图像转换为RGB图像以供显示
            DxRaw8toRGB24(image_buffer,image_rgb_buffer, image_width, image_height,RAW2RGB_NEIGHBOUR,
                          DX_PIXEL_COLOR_FILTER(color_filter),false);
            break;

        case GX_PIXEL_FORMAT_MONO12:
            //将12位格式的图像转换为8位格式
            DxRaw16toRaw8(image_buffer, image_raw8_buffer, image_width, image_height, DX_BIT_4_11);
            //将Raw8图像转换为RGB图像以供显示
            DxRaw8toRGB24(image_raw8_buffer, image_rgb_buffer, image_width, image_height, RAW2RGB_NEIGHBOUR,
                          DX_PIXEL_COLOR_FILTER(NONE),false);
            break;

        case GX_PIXEL_FORMAT_MONO10:
            //将10位格式的图像转换为8位格式
            DxRaw16toRaw8(image_buffer, image_raw8_buffer, image_width, image_height, DX_BIT_4_11);
            //将Raw8图像转换为RGB图像以供显示
            DxRaw8toRGB24(image_raw8_buffer, image_rgb_buffer, image_width, image_height,RAW2RGB_NEIGHBOUR,
                          DX_PIXEL_COLOR_FILTER(NONE),false);
            break;

        case GX_PIXEL_FORMAT_MONO8:
            //将Raw8图像转换为RGB图像以供显示
            DxRaw8toRGB24(image_buffer, image_rgb_buffer, image_width, image_height,RAW2RGB_NEIGHBOUR,
                          DX_PIXEL_COLOR_FILTER(NONE),false);
            break;

        default:
            break;
    }
}


void Camera::GetErrorString(GX_STATUS error_status)
{
    char *error_info = NULL;
    size_t    size         = 0;
    GX_STATUS status      = GX_STATUS_SUCCESS;

    // 获取错误描述信息长度
    status = GXGetLastError(&error_status, NULL, &size);
    error_info = new char[size];
    if (error_info == NULL)
    {
        printf("<Failed to allocate memory>\n");
        return ;
    }

    // 获取错误信息描述
    status = GXGetLastError(&error_status, error_info, &size);
    if (status != GX_STATUS_SUCCESS)
    {
        printf("<GXGetLastError call fail>\n");
    }
    else
    {
        printf("%s\n", (char*)error_info);
    }

    // 释放资源
    if (error_info != NULL)
    {
        delete[]error_info;
        error_info = NULL;
    }
}