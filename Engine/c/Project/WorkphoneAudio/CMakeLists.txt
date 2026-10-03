project(WorkphoneAudio)

include_directories( ../../Include/WorkphoneCore/ )
include_directories( ../../Include/WorkphoneAudio/ )

file(GLOB_RECURSE HEADER_FILES "../../Include/WorkphoneAudio/*.h" )

# Only include the base audio manager implementation
set(SOURCE_FILES "../../Source/WorkphoneAudio/workphone_audio_mgr.c")

# Add platform-specific backend
if(WIN32)    
	list(APPEND SOURCE_FILES "../../Source/WorkphoneAudio/workphone_audio_mgr_win32.c")
elseif(APPLE)    
	if(IOS)        
		list(APPEND SOURCE_FILES "../../Source/WorkphoneAudio/workphone_audio_mgr_ios.c")    
	else()        
		list(APPEND SOURCE_FILES "../../Source/WorkphoneAudio/workphone_audio_mgr_macos.c")    
	endif()
elseif(ANDROID)    
	list(APPEND SOURCE_FILES "../../Source/WorkphoneAudio/workphone_audio_mgr_android.c")
endif()

add_library(WorkphoneAudio STATIC ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties (WorkphoneAudio PROPERTIES FOLDER Engine/C)

if (WIN32)    
	target_compile_definitions(WorkphoneAudio PRIVATE _WIN32_WINNT=0x0602)    
	target_link_libraries(WorkphoneAudio xaudio2 winmm ole32)
endif()

if (NOT WP_STATIC_LIB)    
	target_compile_definitions(WorkphoneAudio PUBLIC WorkphoneAudio_EXPORTS)
endif()