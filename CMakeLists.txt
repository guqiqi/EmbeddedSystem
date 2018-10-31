cmake_minimum_required(VERSION 3.10)
project(EmbeddedSystem)

set(CMAKE_CXX_STANDARD 11)

include_directories(lib "/usr/local/Cellar/opencv/3.4.0_1/include/")

#link_libraries(“libGPIO.a”)

add_executable(EmbeddedSystem
#        "Basic Principle/GPIO/DCmotors.cpp"
#        "Basic Principle/GPIO/decoderISR.cpp"
#        "Basic Principle/GPIO/servo.cpp"
#        examples/GPIO/DCmotors.cpp
#        examples/GPIO/decoderISR.cpp
#        examples/GPIO/servo.cpp
        examples/OpenCV/main.cpp
        lib/GPIOlib.h
        CMakeLists.txt "Basic Principle/test.cpp" examples/run/demo.cpp)
