PROJECT(dvblinkremote)

ENABLE_LANGUAGE(CXX)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(SOURCES channel.cpp
            dvblinkremotecommunication.cpp
            dvblinkremote.cpp
            dvblinkremotehttp.cpp
            epg.cpp
            favorites.cpp
            generic_response.cpp
            item_metadata.cpp
            m3u_playlist.cpp
            parental_lock.cpp
            playback_container.cpp
            playback_item.cpp
            playback_object.cpp
            program.cpp
            recording.cpp
            recording_settings.cpp
            remove_playback_object_request.cpp
            server_info.cpp
            scheduling.cpp
            stop_stream_request.cpp
            stream.cpp
            streaming_capabilities.cpp
            stream_request.cpp
            transcoded_video_stream_request.cpp
            transcoding_options.cpp
            util.cpp
            xml_object_serializer_factory.cpp)

if(NOT WIN32)
  add_options(ALL_LANGUAGES ALL_BUILDS "-fPIC")
endif()

ADD_LIBRARY(dvblinkremote STATIC ${SOURCES})

if(WIN32)
  target_link_libraries(dvblinkremote ws2_32)
endif()
