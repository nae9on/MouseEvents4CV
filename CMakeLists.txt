cmake_minimum_required(VERSION 3.16)

project(project_MouseEvents4CV)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Using Clang")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Using GCC")
    add_compile_options(-Wall -Wextra -Wpedantic -O3)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    message(STATUS "Using Intel C++")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "Using Visual Studio C++")
    add_compile_options(/W4 /analyze)
endif()

if (WIN32)
    # this overrides the OpenCV_DIR env variable (if set previously)
    set(OpenCV_DIR "C:/Users/ahkad/opencv/gnu_build/install")
endif (WIN32)
find_package(OpenCV REQUIRED)

find_package(Boost COMPONENTS system filesystem REQUIRED)

FILE(GLOB allcpp ./*.cpp)

FILE(GLOB TinyXmlcpp ./TinyXml/*.cpp)

add_executable(
"${PROJECT_NAME}"
MouseEvents.h
${allcpp}
${TinyXmlcpp}
)

target_link_libraries("${PROJECT_NAME}" ${OpenCV_LIBS})
target_link_libraries("${PROJECT_NAME}" ${Boost_LIBRARIES})

message(STATUS "OpenCV_DIR ${OpenCV_DIR}")
message(STATUS "OpenCV_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIBS ${OpenCV_LIBS}")
