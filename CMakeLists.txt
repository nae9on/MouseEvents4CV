# Use at-least 3.0 for Modern CMake
cmake_minimum_required(VERSION 3.16)

# Sets the name of the project and stores it in the PROJECT_NAME variable
project(target_MouseEvents4CV)

# Add sub-directories corresponding to other targets that needs to be build first
# The CMake instance will first build the MyLib sub-directory using its own CMakeLists.txt
# add_subdirectory(MyLib)

# Specify the C++ standard when compiling targets from the current directory and below
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add an executable to the project using the specified source files.
FILE(GLOB allcpp ./*.cpp)
FILE(GLOB TinyXmlcpp ./TinyXml/*.cpp)
add_executable(
"${PROJECT_NAME}"
MouseEvents.h
${allcpp}
${TinyXmlcpp}
)

# Following flags will be used when compiling the current target
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Using Clang")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Using GNU GCC")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -O3)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    message(STATUS "Using Intel C++")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "Using Visual Studio C++")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /analyze)
endif()

message(STATUS "Using CXX compiler version " ${CMAKE_CXX_COMPILER_VERSION})

if (WIN32)
    set(OpenCV_DIR "C:/Users/ahkad/opencv/gnu_build/install")
elseif (UNIX)
    set(OpenCV_DIR "/usr/local/lib/cmake/opencv4")
endif()

find_package(OpenCV REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)

target_include_directories("${PROJECT_NAME}" PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries("${PROJECT_NAME}" PRIVATE ${OpenCV_LIBS} PRIVATE ${Boost_LIBRARIES})

message(STATUS "OpenCV_DIR ${OpenCV_DIR}")
message(STATUS "OpenCV_INCLUDE_DIRS ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIBS ${OpenCV_LIBS}")

message(STATUS "Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARIES ${Boost_LIBRARIES}")
