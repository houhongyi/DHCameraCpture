//
// Created by happy on 18-8-29.
//

#ifndef OPENCVTEST_CAMARA_H
#define OPENCVTEST_CAMARA_H

#include <opencv2/opencv.hpp>
#include "GxIAPI.h"
#include "DxImageProc.h"
#include "CTimeCounter.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


#define FRAMEINFOOFFSET 14
#define MEMORY_ALLOT_ERROR -1



typedef enum
{
    Camera_State_Uninite=0,
    Camera_State_Inited,
    Camera_State_Ready,
    Camera_State_Exposing,

}Camera_State;

class Camera
{
public:

    cv::Mat frame_;
    Camera_State CameraState_= Camera_State_Uninite;
    GX_DEV_HANDLE hdevice_ = NULL;                                    ///< 设备句柄
    GX_FRAME_DATA frame_data_ = { 0 };                               ///< 采集图像参数
    void *raw8_buffer_ = NULL;                                       ///< 将非8位raw数据转换成8位数据的时候的中转缓冲buffer
    void *rgb_frame_data_ = NULL;                                    ///< RAW数据转换成RGB数据后的存储空间，大小是相机输出数据大小的3倍
    int64_t pixel_format_ = GX_PIXEL_FORMAT_BAYER_GR8;               ///< 当前相机的pixelformat格式
    int64_t color_filter_ = GX_COLOR_FILTER_NONE;                    ///< bayer插值的参数
    pthread_t acquire_thread_ = 0;                                   ///< 采集线程ID
    bool get_image_ = false;                                         ///< 采集线程是否结束的标志：true 运行；false 退出
    void *frameinfo_data_ = NULL;                                    ///< 帧信息数据缓冲区
    size_t frameinfo_datasize_ = 0;                                  ///< 帧信息数据长度


    int Init(char* No);//相机初始化
    int ReInit();//相机再次初始化
    int Start();//相机启动采集
    int Stop();//相机停止采集
    int Triger();//相机触发命令
    int GetImg();//相机获取图像
    int SaveImg();//相机保存图像
    int Close();//相机关闭

    void GetErrorString(GX_STATUS error_status);
    void ProcessData(void *image_buffer, void *image_raw8_buffer, void *image_rgb_buffer, int image_width, int image_height, int pixel_format, int color_filter);

};


#endif //OPENCVTEST_CAMARA_H
