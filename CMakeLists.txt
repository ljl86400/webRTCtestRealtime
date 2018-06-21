CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(SOUND)

include_directories(/usr/include/webRtcModule)

AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR} SOUND_SRS)

ADD_EXECUTABLE(alsa_record alsa_record.cpp)
ADD_EXECUTABLE(alsa_playback alsa_playback.c)
ADD_EXECUTABLE(asoundlibInf asoundlibInf.c)
ADD_EXECUTABLE(asoundHwSet asoundHwSet.c)

TARGET_LINK_LIBRARIES(alsa_record libasound.so)
TARGET_LINK_LIBRARIES(alsa_record libwebRTC.so)
TARGET_LINK_LIBRARIES(alsa_record libpthread.so)
TARGET_LINK_LIBRARIES(alsa_playback libasound.so)
TARGET_LINK_LIBRARIES(asoundlibInf libasound.so)
TARGET_LINK_LIBRARIES(asoundHwSet libasound.so)

