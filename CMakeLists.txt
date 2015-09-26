cmake_minimum_required(VERSION 3.3)
project(LifeTheGame)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lpthread")

set(SOURCE_FILES master.cpp field.cpp field.cpp worker.cpp)
add_executable(LifeTheGame ${SOURCE_FILES})

target_link_libraries(LifeTheGame -lpthread)