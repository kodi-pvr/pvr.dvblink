project(pvr.dvblink)

cmake_minimum_required(VERSION 2.6)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(Kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(p8-platform REQUIRED)
find_package(TinyXML2 REQUIRED)

include_directories(${kodiplatform_INCLUDE_DIRS}
                    ${p8-platform_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}
                    ${TINYXML2_INCLUDE_DIRS}
                    ${PROJECT_SOURCE_DIR}/lib)

add_definitions(-D__STDC_FORMAT_MACROS)

set(DVBLINK_SOURCES src/base64.cpp
                    src/client.cpp
                    src/DVBLinkClient.cpp
                    src/HttpPostClient.cpp
                    src/RecordingStreamer.cpp
                    src/TimeShiftBuffer.cpp)

add_subdirectory(lib/libdvblinkremote)
set(DEPLIBS ${p8-platform_LIBRARIES} dvblinkremote ${TINYXML2_LIBRARIES})
build_addon(pvr.dvblink DVBLINK DEPLIBS)

include(CPack)
