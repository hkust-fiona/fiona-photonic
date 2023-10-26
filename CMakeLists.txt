cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0074 NEW)

project(FionaPhotonic_Spike)
project(FionaPhotonic_Verilator)

option(USE_EIGEN "enable spike interface add-ons with Eigen3 library" OFF)

set(CMAKE_C_STANDARD 11)
set(THREADS_PREFER_PTHREAD_FLAG ON)

if (USE_EIGEN)
  add_definitions(-DUSE_EIGEN)
  find_package(Eigen3 3.3 REQUIRED)
  message(STATUS "Eigen3 is enabled for Spike-Simulator.")
endif ()

find_package(Python 3.6 REQUIRED Development)
find_package(verilator HINTS $ENV{VERILATOR_ROOT})
find_package(Threads REQUIRED)

if (NOT verilator_FOUND)
  message(FATAL_ERROR "Verilator was not found. Either install it, or set the VERILATOR_ROOT environment variable.")
endif()

if (USE_EIGEN)
  include_directories(${EIGEN3_INCLUDE_DIR})
endif ()
include_directories(${Python_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR} bridge)

# spike
add_executable(FionaPhotonic_Spike test/spike/main.cpp)
target_include_directories(FionaPhotonic_Spike PRIVATE bridge/spike)
target_link_libraries(FionaPhotonic_Spike PRIVATE ${Python_LIBRARIES} ${CMAKE_DL_LIBS})

# verilator
add_executable(FionaPhotonic_Verilator bridge/verilator/engine.cc test/verilator/main.cpp)
target_include_directories(FionaPhotonic_Verilator PRIVATE bridge/verilator)
target_link_libraries(FionaPhotonic_Verilator PRIVATE ${Python_LIBRARIES})
target_link_libraries(FionaPhotonic_Verilator PRIVATE Threads::Threads)
set(VERILATOR_TOP_MODULE)
verilate(${PROJECT_NAME}
  INCLUDE_DIRS "bridge/verilator/"
  SOURCES test/verilator/top.sv
)

file(COPY "${PROJECT_SOURCE_DIR}/pyfunc/" DESTINATION "${PROJECT_BINARY_DIR}/pyfunc/")

