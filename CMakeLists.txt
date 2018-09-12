cmake_minimum_required(VERSION 3.1)
project(CaptureCatch)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CUDA_USE_STATIC_CUDA_RUNTIME OFF) #禁用CUDA
set(SOURCE_FILES main.cpp Camara.cpp)


find_package(OpenCV REQUIRED) #查找opencv包

add_executable(CaptureCatch ${SOURCE_FILES})

target_link_libraries(CaptureCatch
        ${OpenCV_LIBS}
        gxiapi
        dximageproc
        pthread
        GCBase_gcc40_v2_3
        GenApi_gcc40_v2_3
        log4cpp_gcc40_v2_3
        Log_gcc40_v2_3
        MathParser_gcc40_v2_3

) #加入opencv lib