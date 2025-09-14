cmake_minimum_required(VERSION 3.19)
project(RakNet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(RAKNET_BUILD_SHARED "Build RakNet as a shared library" OFF)

if(RAKNET_BUILD_SHARED)
    add_library(${PROJECT_NAME} SHARED)
else()
    add_library(${PROJECT_NAME} STATIC)
endif()

file(GLOB ALL_HEADER_SRCS CONFIGURE_DEPENDS "${RakNet_SOURCE_DIR}/include/*.h")
file(GLOB ALL_CPP_SRCS    CONFIGURE_DEPENDS "${RakNet_SOURCE_DIR}/Source/*.cpp")

target_sources(${PROJECT_NAME} PRIVATE ${ALL_CPP_SRCS} ${ALL_HEADER_SRCS})

target_include_directories(${PROJECT_NAME} PRIVATE ${RAKNET_INTERNAL_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} PRIVATE
    $<$<PLATFORM_ID:Windows>:ws2_32.lib>
    $<$<NOT:$<PLATFORM_ID:Windows>>:pthread>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<PLATFORM_ID:Windows>:WIN32>
    _RAKNET_LIB
    _CRT_NONSTDC_NO_DEPRECATE
    _CRT_SECURE_NO_DEPRECATE
)

target_link_options(${PROJECT_NAME} PRIVATE
    $<$<AND:$<PLATFORM_ID:Windows>,$<CXX_COMPILER_ID:MSVC>>:/NODEFAULTLIB:LIBCD.lib>
    $<$<AND:$<PLATFORM_ID:Windows>,$<CXX_COMPILER_ID:MSVC>>:/NODEFAULTLIB:LIBCMTD.lib>
    $<$<AND:$<PLATFORM_ID:Windows>,$<CXX_COMPILER_ID:MSVC>>:/NODEFAULTLIB:MSVCRT.lib>
)
