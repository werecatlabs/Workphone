project(WorkphoneAudio LANGUAGES C)

set(AUDIO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
# Platform legacy sources are deliberately excluded: they are incomplete and
# previously compiled twice through textual includes. Device output remains in
# WPAudio until the native-core output spike is certified.
add_library(WorkphoneAudio STATIC
    ${AUDIO_ROOT}/Source/WorkphoneAudio/workphone_audio_core.c
    ${AUDIO_ROOT}/Source/WorkphoneAudio/workphone_audio_mgr.c
    ${AUDIO_ROOT}/Source/WorkphoneAudio/workphone_audio_sound.c
    ${AUDIO_ROOT}/Source/WorkphoneAudio/workphone_audio_listener.c
    ${AUDIO_ROOT}/Source/WorkphoneAudio/workphone_audio_event.c
    ${AUDIO_ROOT}/Source/WorkphoneAudio/workphone_audio_param.c
    ${AUDIO_ROOT}/Source/WorkphoneAudio/workphone_audio_project.c)
target_include_directories(WorkphoneAudio PUBLIC
    ${AUDIO_ROOT}/Include/WorkphoneAudio ${AUDIO_ROOT}/Include/WorkphoneCore)
set_target_properties(WorkphoneAudio PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON FOLDER Engine/C)
if(UNIX)
    target_link_libraries(WorkphoneAudio PUBLIC m)
endif()
