cmake_minimum_required(VERSION 2.8.2)
project(LifeTheGame)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")

set(SOURCE_FILES main.cpp field.cpp field.cpp worker.cpp master.cpp)
add_executable(LifeTheGame ${SOURCE_FILES})

target_link_libraries(LifeTheGame -lpthread)
