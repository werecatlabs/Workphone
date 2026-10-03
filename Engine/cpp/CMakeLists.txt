set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cpp_defines)
include(cpp_compiler_options)

if (ANDROID)
	function(wp_add_android_dependency TARGET_NAME DEPENDENCY_DIR)
		if (NOT TARGET ${TARGET_NAME})
			add_subdirectory(${WP_ROOT}/Dependencies/${DEPENDENCY_DIR}
				${CMAKE_BINARY_DIR}/Dependencies/${DEPENDENCY_DIR})
		endif()
	endfunction()

	wp_add_android_dependency(boost_thread boost)
	wp_add_android_dependency(freetype freetype)
	wp_add_android_dependency(icucommon icu)
	wp_add_android_dependency(harfbuzz harfbuzz)
	target_include_directories(harfbuzz PRIVATE ${WP_ROOT}/Dependencies/android)
	target_include_directories(harfbuzz-subset PRIVATE ${WP_ROOT}/Dependencies/android)
	wp_add_android_dependency(lua lua)
	wp_add_android_dependency(lua_cjson lua_cjson)
	wp_add_android_dependency(luabind luabind)
	wp_add_android_dependency(zlib zlib)
	wp_add_android_dependency(minizip minizip)
	if (NOT TARGET nativefiledialog)
		add_library(nativefiledialog STATIC ${WP_ROOT}/Dependencies/android/nfd_android.cpp)
		target_include_directories(nativefiledialog PUBLIC ${WP_ROOT}/Dependencies/nativefiledialog/include)
	endif()
	wp_add_android_dependency(rttr rttr)
	wp_add_android_dependency(sds_library sds_library)
	wp_add_android_dependency(tinyxml tinyxml)
	wp_add_android_dependency(yaml-cpp yaml-cpp)
	wp_add_android_dependency(zzip zzip)
endif()

#add_subdirectory(Project/WPAi)
#add_subdirectory(Project/WPAnimation)
#add_subdirectory(Project/WPApplication)
if (WP_BUILD_AUDIO)
	add_subdirectory(Project/WPAudio)
endif()

if (WP_USE_ASSET_IMPORT)
	add_subdirectory(Project/WPAssimp)
endif()

if (WP_BUILD_BULLETPHYSICS)
	add_subdirectory(Project/WPBulletPhysics)
endif ()

if (WP_BUILD_CEGUI)
	add_subdirectory(Project/WPCEGui)
endif ()

if (WP_BUILD_IMGUI)
	add_subdirectory(Project/WPImGui)
endif ()

add_subdirectory(Project/Workphone)

if (WP_BUILD_DIRECTSHOW)
	add_subdirectory(Project/WPDirectShow)
endif ()

if (NOT WP_MOBILE)
	add_subdirectory(Project/WPFFMpeg)
	add_subdirectory(Project/WPTheoraVideo)
endif()
#add_subdirectory(Project/WPGStreamer)

if (WP_BUILD_FMOD)
	add_subdirectory(Project/WPFmodStudio)
endif ()

if (NOT WP_MOBILE)
	add_subdirectory(Project/WPRuntime)
endif()

if (WP_GRAPHICS_SYSTEM_CLAW)
	add_subdirectory(Project/WPGraphics)
endif()

if (WP_GRAPHICS_SYSTEM_OGRE)
	add_subdirectory(Project/WPGraphicsOgre)
endif()

if (WP_GRAPHICS_SYSTEM_OGRENEXT)
	add_subdirectory(Project/WPGraphicsOgreNext)
endif()

if (WP_ENABLE_LUA)
	add_subdirectory(Project/WPLua)
	add_subdirectory(Project/WPLuaBind)
endif()

if (WP_ENABLE_PYPY)
	add_subdirectory(Project/WPPyPy)
	add_subdirectory(Project/WPPyPyBind)
endif()

if (WP_ENABLE_PYTHON)
	add_subdirectory(Project/WPPython)
	add_subdirectory(Project/WPPythonBind)
endif()

add_subdirectory(Project/WPProcedural)
add_subdirectory(Project/WPVehiclePhysics)

if (WP_BUILD_PHYSX)
	add_subdirectory(Project/WPPhysx)
endif()

if (ANDROID AND NOT TARGET WorkphonePhysics)
	add_subdirectory(../c/Project/WorkphonePhysics ${CMAKE_BINARY_DIR}/Engine/c/Project/WorkphonePhysics)
endif()

add_subdirectory(Project/WPPhysics)

option(WP_BUILD_NAVIGATION "Build navigation mesh system" OFF)

if(WP_BUILD_NAVIGATION)
	add_subdirectory(Project/WPNavmesh)
endif()

if (WP_BUILD_SQLITE)
	add_subdirectory(Project/WPSQLite)
endif()

if (WP_BUILD_ODE)
	add_subdirectory(Project/WPODE3)
endif()
	
if (WP_ENABLE_RAKNET)
	add_subdirectory(Project/WPRakNet)
endif()

if (WP_BUILD_C)
	add_subdirectory(Project/WPNetwork)
endif()

if (WP_BUILD_OISINPUT)
	add_subdirectory(Project/WPOISInput)
endif()

if (ANDROID)
	add_subdirectory(Project/WPRuntime)
endif()

if (WP_USE_HOUDINI)
	#add_subdirectory(Project/WPHoudini)
endif()

if (WP_BUILD_WXWIDGETS)
	add_subdirectory(Project/WPWxWidgets)
endif ()




