cmake_minimum_required(VERSION 3.5)
PROJECT(dvblinkremote)

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
            timeshift.cpp
            transcoded_video_stream_request.cpp
            transcoding_options.cpp
            util.cpp
            resume_info.cpp
            xml_object_serializer_factory.cpp)

ADD_LIBRARY(dvblinkremote STATIC ${SOURCES})
set_property(TARGET dvblinkremote PROPERTY POSITION_INDEPENDENT_CODE ON)

if(WIN32)
  target_link_libraries(dvblinkremote ws2_32)
endif()
