cmake_minimum_required(VERSION 3.27)
project(BiliBili_Reranger)

set(CMAKE_CXX_STANDARD 23)
set(DCMAKE_VERBOSE_MAKEFILE ON)# 输出构建日志
set(DEBUG_OUTPUT_PATH "${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DEBUG_OUTPUT_PATH})# Debug模式编译文件输出目录
if(WIN32)
    add_compile_options(/utf-8)
endif ()

add_executable(${PROJECT_NAME} src/main.cpp
        src/crawler.cpp
        src/portListener.cpp
        src/pluginHandler.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Include
add_subdirectory(api)
if(DEFINED API)
    message(STATUS "Find API DLL, adding ...")
    target_link_libraries(${PROJECT_NAME} PUBLIC ${API})
    add_dependencies(${PROJECT_NAME} ${API})
else ()
    message(FATAL_ERROR "Can not find API DLL, cmake failed")
endif ()

find_package(CURL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)
find_package(Boost REQUIRED COMPONENTS system)

add_custom_target(
        COPY_ALL_DLLS
        COMMENT "Copying all plugin dlls to right path"
)
add_custom_target(
        COPY_ALL_PDBS
        COMMENT "Copying all plugin pdbs to right path (prepare for setting break point)"
)
# Subproject
add_subdirectory(plugins/ExamplePlugin)

# DLL Plugin (For Test)
message(STATUS "Finding Plugins...")
if(DEFINED DLL_DIR)
if(DEFINED DLL_NAME)
    message(STATUS "Adding Plugin: ${DLL_DIR}/${DLL_NAME}")
    include_directories(${DLL_DIR})
    link_directories(${DLL_DIR})
    #target_link_libraries(${PROJECT_NAME} PUBLIC "${DLL_NAME}.dll")加上这个意味着静态加载这个库
endif ()
endif ()

get_property(NEED_COPY GLOBAL PROPERTY COPY_ALL_DLLS)
if(NEED_COPY)
    add_dependencies(COPY_ALL_DLLS ${NEED_COPY})
endif ()

get_property(NEED_COPY_PDB GLOBAL PROPERTY COPY_ALL_PDBS)
if(NEED_COPY_PDB)
    add_dependencies(COPY_ALL_PDBS ${NEED_COPY_PDB})
else ()
    add_dependencies(COPY_ALL_PDBS COPY_ALL_DLLS)
endif ()

target_compile_definitions(
        ${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:DEVELOP=true>
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
    add_custom_command(
            POST_BUILD
            TARGET ${PROJECT_NAME}
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/test" "${CMAKE_BINARY_DIR}/Debug/testing"
            COMMENT "copying crawled data for test ..."
    )
endif ()