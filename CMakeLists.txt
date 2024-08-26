cmake_minimum_required(VERSION 3.10)

include(FetchContent)

set(TARGET pdf_info)
project(${TARGET})

FetchContent_Declare(
  PDFHummus
  GIT_REPOSITORY https://github.com/galkahana/PDF-Writer.git
  GIT_TAG        v4.6.6
  FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(PDFHummus)


include_directories(${CMAKE_SOURCE_DIR})

list(APPEND SRCS "pdf_info.cpp")

set(COMPILER_FLAGS "")
set(COMPILER_FLAGS "${COMPILER_FLAGS} -ggdb")
set(COMPILER_FLAGS "${COMPILER_FLAGS} -O2")

set(COMPILER_FLAGS "${COMPILER_FLAGS} -g2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMPILER_FLAGS}")

add_executable(${TARGET} ${SRCS})
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_OBJCOPY} ARGS --only-keep-debug ${${TARGET}_BINARY_DIR}/${PROJECT_NAME} ${${TARGET}_BINARY_DIR}/${PROJECT_NAME}.debug)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_STRIP} ${${TARGET}_BINARY_DIR}/${PROJECT_NAME})
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_OBJCOPY} ARGS --add-gnu-debuglink=${${TARGET}_BINARY_DIR}/${PROJECT_NAME}.debug ${${TARGET}_BINARY_DIR}/${PROJECT_NAME})

add_dependencies(${TARGET} PDFHummus::PDFWriter)

target_link_libraries (${TARGET} PDFHummus::PDFWriter)
