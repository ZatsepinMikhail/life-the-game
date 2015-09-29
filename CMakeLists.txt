cmake_minimum_required(VERSION 2.8)
project(LifeTheGame)

find_package(MPI REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lpthread")

set(SOURCE_FILES main.cpp field.cpp field.cpp worker.cpp master.cpp)

add_executable(LifeTheGame ${SOURCE_FILES})

include_directories(${MPI_INCLUDE_PATH})
target_link_libraries(LifeTheGame ${MPI_LIBRARIES})
target_link_libraries(LifeTheGame -lpthread)

if(MPI_COMPILE_FLAGS)
    set_target_properties(LifeTheGame PROPERTIES
            COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(LifeTheGame PROPERTIES
            LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()
