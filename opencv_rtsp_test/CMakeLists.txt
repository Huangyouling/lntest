cmake_minimum_required(VERSION 3.5)
project(opencv_rtsp_test LANGUAGES CXX)

find_package(OpenCV REQUIRED)

link_directories(${OpenCV_LIBRARY_DIRS})
add_executable(opencv_rtsp_test main.cpp)
target_link_libraries(opencv_rtsp_test 
                    -lgstrtspserver-1.0
                    -lgstreamer-1.0
                    -lgobject-2.0
                    -lpthread
                    ${OpenCV_LIBS}
                    )
include_directories("/usr/include/gstreamer-1.0"
                    "/usr/include/glib-2.0"
                    "/usr/local/include/gstreamer-1.0/gst/rtsp-server"
                    "/usr/local/include/gstreamer-1.0/gst"
                    ${OpenCV_INCLUDE_DIRS}
)

set(CPAKC_PROJECT_NAME ${PROJECT_NAME})
set(CPAKC_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

