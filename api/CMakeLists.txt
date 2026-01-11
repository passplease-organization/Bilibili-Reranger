cmake_minimum_required(VERSION 3.27)
project(BiliBili_Reranger_API VERSION 2.0)

set(CMAKE_CXX_STANDARD 23)
set(DCMAKE_VERBOSE_MAKEFILE ON)# 输出构建日志
set(CMAKE_CXX_EXTENSIONS OFF)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_options(/utf-8)
endif ()

set(API ${PROJECT_NAME} PARENT_SCOPE)

add_library(${PROJECT_NAME} SHARED
        utils/Util.h
        pluginInterface.h
        APIStatus.h
        pluginInterface.cpp
        utils/config.h
        bilibiliAPIs.h
        utils/BilibiliInterface.h
        ai/CallAI.h
        ai/CallAI.cpp
        utils/utils.cpp
        utils/BilibiliInterface.cpp
        utils/mainConfig.cpp
        webAPIs/socialAPI.h
        webAPIs/socialAPI.cpp
        webAPIs/crawler.h
        webAPIs/platforms.h
        webAPIs/postgres.cpp
        webAPIs/postgres.h
        webAPIs/browse.h
        utils/configUtil.h
        utils/configUtil.cpp
)
add_library(BiliBiliRerangerAPI::API ALIAS ${PROJECT_NAME})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_target_properties(${PROJECT_NAME} PROPERTIES
            INSTALL_RPATH "$ORIGIN"
            BUILD_WITH_INSTALL_RPATH ON
    )
endif ()
set_target_properties(${PROJECT_NAME} PROPERTIES EXPORT_NAME API)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
find_package(OpenSSL CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto)
find_package(unofficial-sodium CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC unofficial-sodium::sodium)
find_package(CURL REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC CURL::libcurl)
find_package(cpr CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC cpr::cpr)
find_package(libpqxx CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC libpqxx::pqxx)
find_package(toml11 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC toml11::toml11)

set_property(TARGET ${PROJECT_NAME} PROPERTY ENABLE_EXPORTS ON)# Allow export

target_compile_definitions(${PROJECT_NAME} PRIVATE API_DLL)
target_include_directories(${PROJECT_NAME}
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/BiliBili_Reranger>
)

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

install(
        TARGETS ${PROJECT_NAME}
        EXPORT BiliBiliRerangerAPITargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
) # generate .so, .dll ...

install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/BiliBili_Reranger
        FILES_MATCHING PATTERN "*.h"
)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/BiliBiliRerangerAPIConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/BiliBiliRerangerAPIConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/BiliBiliRerangerAPIConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BiliBiliRerangerAPI
) # generate BiliBiliRerangerAPIConfig.cmake (entrypoint)

install(
        EXPORT BiliBiliRerangerAPITargets
        NAMESPACE BiliBiliRerangerAPI::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BiliBiliRerangerAPI
) # generate BiliBiliRerangerAPITargets.cmake (define project)

install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/BiliBiliRerangerAPIConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/BiliBiliRerangerAPIConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BiliBiliRerangerAPI
)
