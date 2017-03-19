cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
project(fujinami)

if(UNIX AND NOT APPLE)
    option(ENABLE_CODECOVERAGE "Enable code coverage")
elseif(MSVC)
endif()

find_package(Lua REQUIRED)

set(FUJINAMI_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(BOOST_PREPROCESSOR_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libs/boost/preprocessor/include)
set(GSL_LITE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libs/gsl-lite/include)
set(SPDLOG_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libs/spdlog/include)
set(SOL2_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libs/sol2)
set(CATCH_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libs/catch/include)

add_subdirectory(src)

enable_testing()
add_subdirectory(test)
