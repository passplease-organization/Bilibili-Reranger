cmake_minimum_required(VERSION 3.27)
project(ExamplePlugin)
set(CMAKE_CXX_STANDARD 23)
set(DCMAKE_VERBOSE_MAKEFILE ON)# 输出构建日志
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_options(/utf-8)
endif ()

add_library(${PROJECT_NAME} SHARED main.cpp
        interface.h
        tasks.h)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${API})
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")
endif ()

# Plugin Interface (For Test)
set(DLL_DIR "${CMAKE_BINARY_DIR}/plugins/${PROJECT_NAME}" PARENT_SCOPE)
set(DLL_NAME ${PROJECT_NAME} PARENT_SCOPE)

# Copy .dll and .pdb under /plugins, act after each build task
add_custom_target(
        "${PROJECT_NAME}_COPY_DLL"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_PATH}/plugins"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> "${OUTPUT_PATH}/plugins" # copy
        COMMENT "dealing with plugin dll files ..."
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_custom_target(
        "${PROJECT_NAME}_COPY_PDB"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PDB_FILE:${PROJECT_NAME}> "${OUTPUT_PATH}/plugins" # copy
        COMMENT "dealing with plugin pdb files ..."
    )# .pdb file for debug set break point
endif ()

add_dependencies("${PROJECT_NAME}_COPY_DLL" ${PROJECT_NAME})
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_dependencies("${PROJECT_NAME}_COPY_PDB" "${PROJECT_NAME}_COPY_DLL")
endif ()
set_property(GLOBAL APPEND PROPERTY COPY_ALL_DLLS "${PROJECT_NAME}_COPY_DLL")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_property(GLOBAL APPEND PROPERTY COPY_ALL_PDBS "${PROJECT_NAME}_COPY_PDB")
endif ()