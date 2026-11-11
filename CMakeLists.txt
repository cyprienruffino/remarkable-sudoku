cmake_minimum_required(VERSION 3.16)
project(sudoku LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# dlib, for handwriting recognition, is compiled in since the device doesn't ship it
set(DLIB_SOURCE_DIR "" CACHE PATH "Path to an extracted dlib source tree")
if(NOT DLIB_SOURCE_DIR)
    include(FetchContent)
    FetchContent_Declare(dlib
        URL https://github.com/davisking/dlib/archive/refs/tags/v20.0.tar.gz
        URL_HASH SHA512=a4bcb2d013bd2b0000530d684c9c4b9f047f9fa6216174b3cb26d96f66c4a302d0bd1733d0ba35626d57133d9159f90114ab51a3af8fb9c493ff3e74dcc73911
    )
    FetchContent_Populate(dlib)
    set(DLIB_SOURCE_DIR ${dlib_SOURCE_DIR})
endif()
add_library(dlib STATIC ${DLIB_SOURCE_DIR}/dlib/all/source.cpp)
target_include_directories(dlib PUBLIC ${DLIB_SOURCE_DIR})
target_compile_definitions(dlib PUBLIC DLIB_NO_GUI_SUPPORT DLIB_DISABLE_ASSERTS)
find_package(Threads REQUIRED)
target_link_libraries(dlib PUBLIC Threads::Threads)

# The backend, started by AppLoad as backend/entry
add_executable(entry backend/main.cpp backend/sudoku.cpp backend/recognizer.cpp backend/eraser.cpp)
target_link_libraries(entry PRIVATE dlib -static-libstdc++ -static-libgcc)
target_include_directories(entry PRIVATE backend)

# Generator tests, on the PC only
if(NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    add_executable(test_sudoku backend/test_sudoku.cpp backend/sudoku.cpp)
    add_test(NAME sudoku COMMAND test_sudoku)

    # Trains data/digits.dat, see tools/train_digits.cpp. dlib's network code
    # is much faster with the build machine's vector instructions.
    add_library(dlib_native STATIC ${DLIB_SOURCE_DIR}/dlib/all/source.cpp)
    target_include_directories(dlib_native PUBLIC ${DLIB_SOURCE_DIR})
    target_compile_definitions(dlib_native PUBLIC DLIB_NO_GUI_SUPPORT DLIB_DISABLE_ASSERTS)
    target_compile_options(dlib_native PUBLIC -O3 -march=native)
    target_link_libraries(dlib_native PUBLIC Threads::Threads)
    add_executable(train_digits tools/train_digits.cpp)
    target_include_directories(train_digits PRIVATE backend)
    target_link_libraries(train_digits PRIVATE dlib_native)

    add_executable(test_digits backend/test_digits.cpp backend/recognizer.cpp)
    target_include_directories(test_digits PRIVATE backend)
    target_link_libraries(test_digits PRIVATE dlib)
    add_test(NAME digits COMMAND test_digits ${CMAKE_SOURCE_DIR}/data/digits.dat)
endif()

# The frontend, packed into resources.rcc
if(NOT RCC_EXECUTABLE AND DEFINED ENV{RCC_EXECUTABLE})
    set(RCC_EXECUTABLE $ENV{RCC_EXECUTABLE})
endif()
find_program(RCC_EXECUTABLE NAMES rcc REQUIRED)
file(GLOB_RECURSE QML_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/ui/*)
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/resources.rcc
    COMMAND ${RCC_EXECUTABLE} --binary -o ${CMAKE_BINARY_DIR}/resources.rcc ${CMAKE_SOURCE_DIR}/application.qrc
    DEPENDS ${CMAKE_SOURCE_DIR}/application.qrc ${QML_FILES}
)
add_custom_target(resources ALL DEPENDS ${CMAKE_BINARY_DIR}/resources.rcc)

# The AppLoad application directory: cmake --install <build> --prefix dist/sudoku
install(TARGETS entry DESTINATION backend)
install(FILES manifest.json icon.png ${CMAKE_BINARY_DIR}/resources.rcc DESTINATION .)
install(FILES data/digits.dat DESTINATION data)
