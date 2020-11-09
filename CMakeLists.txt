cmake_minimum_required(VERSION 3.5)
project(pvr.dvblink)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

find_package(Kodi REQUIRED)
find_package(TinyXML2 REQUIRED)

include_directories(${KODI_INCLUDE_DIR}/.. # Hack way with "/..", need bigger Kodi cmake rework to match right include ways
                    ${TINYXML2_INCLUDE_DIRS}
                    ${PROJECT_SOURCE_DIR}/lib)

add_definitions(-D__STDC_FORMAT_MACROS)

set(DVBLINK_SOURCES src/addon.cpp
                    src/base64.cpp
                    src/HttpPostClient.cpp
                    src/Settings.cpp
                    src/Socket.cpp
                    src/DVBLinkClient.cpp
                    src/RecordingStreamer.cpp
                    src/TimeShiftBuffer.cpp)

set(DVBLINK_HEADERS src/addon.h
                    src/base64.h
                    src/dvblink_connection.h
                    src/HttpPostClient.h
                    src/Settings.h
                    src/Socket.h
                    src/DVBLinkClient.h
                    src/RecordingStreamer.h
                    src/TimeShiftBuffer.h)

add_subdirectory(lib/libdvblinkremote)
set(DEPLIBS dvblinkremote ${TINYXML2_LIBRARIES})
if(WIN32)
  list(APPEND DEPLIBS ws2_32)
  add_definitions(-D_WINSOCKAPI_ -D_WINSOCK_DEPRECATED_NO_WARNINGS)
endif()

build_addon(pvr.dvblink DVBLINK DEPLIBS)

include(CPack)
