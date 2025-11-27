cmake_minimum_required(VERSION 3.27)
project(BiliBili_Reranger_API)

set(CMAKE_CXX_STANDARD 23)
set(DCMAKE_VERBOSE_MAKEFILE ON)# 输出构建日志
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_options(/utf-8)
endif ()

set(API ${PROJECT_NAME} PARENT_SCOPE)

add_library(${PROJECT_NAME} SHARED
        Util.h
        pluginInterface.h
        APIStatus.h
        pluginInterface.cpp
        config.h
        bilibiliAPIs.h
        BilibiliInterface.h
        ai/CallAI.h
        ai/CallAI.cpp
        utils.cpp
        BilibiliInterface.cpp
        mainConfig.cpp
        #        bilibili_wbi/wbi.cpp
#        bilibili_wbi/wbi.h
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_target_properties(${PROJECT_NAME} PROPERTIES
            INSTALL_RPATH "$ORIGIN"
            BUILD_WITH_INSTALL_RPATH ON
    )
endif ()

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
#find_package(cryptopp CONFIG REQUIRED)
#target_link_libraries(${PROJECT_NAME} PRIVATE cryptopp::cryptopp)
find_package(cpr CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE cpr::cpr)

set_property(TARGET ${PROJECT_NAME} PROPERTY ENABLE_EXPORTS ON)# Allow export

target_compile_definitions(${PROJECT_NAME} PRIVATE API_DLL)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(
        "${PROJECT_NAME}_COPY_DLL"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> ${OUTPUT_PATH} # copy
        COMMENT "dealing with plugin dll files ..."
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_custom_target(
        "${PROJECT_NAME}_COPY_PDB"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_PDB_FILE:${PROJECT_NAME}> ${OUTPUT_PATH} # copy
        COMMENT "dealing with plugin pdb files ..."
    )
endif ()

add_dependencies("${PROJECT_NAME}_COPY_DLL" ${PROJECT_NAME})
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_dependencies("${PROJECT_NAME}_COPY_PDB" "${PROJECT_NAME}_COPY_DLL")
endif ()
set_property(GLOBAL APPEND PROPERTY COPY_ALL_DLLS "${PROJECT_NAME}_COPY_DLL")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set_property(GLOBAL APPEND PROPERTY COPY_ALL_PDBS "${PROJECT_NAME}_COPY_PDB")
endif ()