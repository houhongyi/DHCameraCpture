#include <opencv2/opencv.hpp>
#include "Camera.h"
using namespace cv;

int main() {
    int photoflag = 0;
    Camera Camera_L, Camera_R;
    if (Camera_L.Init("1") < 0 || Camera_R.Init("2") < 0) {
        Camera_L.Close();
        Camera_R.Close();
        return 0;
    }

    Camera_L.Start();
    Camera_R.Start();
    Camera_L.Triger();
    Camera_R.Triger();
    char fileName[30] = {0};

    int frame_n = 0;
    Mat img_L;

    while (1) {
        Camera_L.GetImg();
        Camera_R.GetImg();
        Camera_L.Triger();
        Camera_R.Triger();

        char c = waitKey(30);
        if (c == 27)break;
        if (c == 'p')photoflag = 1;


        imshow("CameraL", Camera_L.frame_);
        imshow("CameraR", Camera_R.frame_);

        if (photoflag) {
            sprintf(fileName, "./IMG/ImgL%d.jpg", frame_n);
            imwrite(fileName, Camera_L.frame_);
            sprintf(fileName, "./IMG/ImgR%d.jpg", frame_n);
            imwrite(fileName, Camera_R.frame_);
            frame_n++;
            printf("%s\r\n", fileName);
        }

    }
    Camera_L.Close();
    Camera_R.Close();
    return 0;

}