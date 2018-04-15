
cmake_minimum_required(VERSION 2.8)
project(nvva)

set(CMAKE_BUILD_TYPE debug)

#set(CUDA_TOOLKIT_ROOT_DIR "/usr/local/cuda-7.5")
include(FindCUDA)

find_package(CUDA REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(/usr/local/stow/ffmpeg/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

set(SOURCE_FILES src/decode.cpp  src/decode.h  src/encode.cpp  src/encode.h  src/yuv2bgr.cu  src/yuv2bgr.h)
cuda_add_executable(nvva main.cpp ${SOURCE_FILES})

LIST(APPEND CMAKE_CXX_FLAGS "-std=c++11 -O3 -ffast-math -Wall")
LIST(APPEND CUDA_NVCC_FLAGS --compiler-options -fno-strict-aliasing -lineinfo -use_fast_math -Xptxas -dlcm=cg)
LIST(APPEND CUDA_NVCC_FLAGS -gencode arch=compute_52,code=sm_52)

target_link_libraries(nvva
        ${CUDA_LIBRARIES}
        ${CUDA_CUBLAS_LIBRARIES}
        /usr/local/stow/ffmpeg/lib/libavcodec.so
        /usr/local/stow/ffmpeg/lib/libavdevice.so
        /usr/local/stow/ffmpeg/lib/libavfilter.so
        /usr/local/stow/ffmpeg/lib/libavformat.so
        /usr/local/stow/ffmpeg/lib/libavutil.so
        /usr/local/stow/ffmpeg/lib/libpostproc.so
        /usr/local/stow/ffmpeg/lib/libswresample.so
        /usr/local/stow/ffmpeg/lib/libswscale.so
        ${OpenCV_LIBS})

