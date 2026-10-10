if(WP_BUILD_TESTS AND TARGET WorkphoneAudio)
    add_subdirectory(../AudioProduction AudioProduction)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cpp_defines)
include(cpp_compiler_options)

if(TARGET WPNetwork)
    add_executable(WPNetworkIntegrationTests NetworkProductionTests.cpp)
    target_compile_definitions(WPNetworkIntegrationTests PRIVATE BOOST_ALL_NO_LIB)
    target_include_directories(WPNetworkIntegrationTests PRIVATE
        ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Engine/c/Include ${WP_ROOT}/Dependencies/boost)
    target_link_libraries(WPNetworkIntegrationTests PRIVATE WPNetwork Workphone)
    set_target_properties(WPNetworkIntegrationTests PROPERTIES FOLDER Tests/Network)
    add_test(NAME WPNetwork.integration COMMAND WPNetworkIntegrationTests)
    set_tests_properties(WPNetwork.integration PROPERTIES LABELS "network;headless;scene" TIMEOUT 30)
endif()

if(WIN32 AND TARGET WPGraphics AND WP_GRAPHICS_SYSTEM_CLAW)
    add_executable(ClawCubemapTests ClawCubemapTests.cpp)
    target_compile_definitions(ClawCubemapTests PRIVATE BOOST_ALL_NO_LIB)
    target_include_directories(ClawCubemapTests PRIVATE
        ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost
        ${WP_ROOT}/Engine/c/Include/WorkphoneCore
        ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics
        ${WP_ROOT}/Engine/c/Include/WorkphonePlatformWin32)
    target_link_libraries(ClawCubemapTests PRIVATE WPGraphics WorkphonePlatformWin32 WorkphoneCore d3d11)
    target_link_directories(ClawCubemapTests PRIVATE
        ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
    set_target_properties(ClawCubemapTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME WPGraphics.cubemap_pbr COMMAND ClawCubemapTests)
endif()

if (WP_BUILD_TESTS)
    if(TARGET WPSQLite)
        add_executable(WPAssetCatalogTests AssetCatalogTests.cpp)
        if(WP_BUILD_RESOURCE_SYSTEM)
            target_compile_definitions(WPAssetCatalogTests PRIVATE WP_CATALOG_RESOURCE_SYSTEM_TESTS=1)
        endif()
        target_include_directories(WPAssetCatalogTests PRIVATE ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost)
        target_link_libraries(WPAssetCatalogTests PRIVATE $<TARGET_LINKER_FILE:WPSQLite> $<TARGET_LINKER_FILE:Workphone>)
        add_dependencies(WPAssetCatalogTests WPSQLite Workphone)
        set_target_properties(WPAssetCatalogTests PROPERTIES FOLDER Tests/Cpp)
        add_test(NAME WorkphoneAssets.catalog_contracts COMMAND WPAssetCatalogTests)
        set_tests_properties(WorkphoneAssets.catalog_contracts PROPERTIES LABELS "assets;database;headless" TIMEOUT 60)
    endif()
	if (WP_BUILD_WXWIDGETS)
		add_subdirectory(./Client)
	endif()
	
	if (WIN32)
		#add_subdirectory(./CppUnitTests)
	endif()
	
	if (WP_USE_HOUDINI)
		#add_subdirectory(./HoudiniRender)
	endif()
		
	add_subdirectory(./Renderer)	
	add_subdirectory(./RendererMT)

	if (WIN32)
		if (WP_BUILD_WXWIDGETS)
			add_subdirectory(./RendererOgre)
			add_subdirectory(./OgreOverlayTest)
		endif()
	endif()

	add_subdirectory(./OverlayTest)
	
	if (WP_BUILD_WXWIDGETS)
		add_subdirectory(./Server)
	endif()
	
	add_subdirectory(./Terrain)

	if (WP_BUILD_WXWIDGETS)
		add_subdirectory(./TestRacknetProgram)
	endif()

	add_subdirectory(./UnitTests)

	if (WP_BUILD_RESOURCE_SYSTEM AND TARGET WPSQLite)
		add_subdirectory(./ResourceSystem)
	endif()

	if (WP_GRAPHICS_SYSTEM_CLAW AND TARGET WPGraphics)
        if(WIN32 AND WP_BUILD_RENDERER_DX11)
            add_executable(WPGraphicsClawProductionTests ClawProductionTests.cpp)
            target_include_directories(WPGraphicsClawProductionTests PRIVATE
                ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Engine/c/Include
                ${WP_ROOT}/Engine/c/Include/WorkphoneCore ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics)
            target_link_libraries(WPGraphicsClawProductionTests PRIVATE
                WPGraphics Workphone WorkphoneGraphics WorkphoneCore WorkphonePlatform d3d11)
            target_link_directories(WPGraphicsClawProductionTests PRIVATE
                ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
            set_target_properties(WPGraphicsClawProductionTests PROPERTIES FOLDER Tests/Cpp)
            add_test(NAME WPGraphics.claw_production_render COMMAND WPGraphicsClawProductionTests)
            set_tests_properties(WPGraphics.claw_production_render PROPERTIES LABELS "graphics;gpu;production" TIMEOUT 60)
            if(WP_BUILD_RESOURCE_SYSTEM AND TARGET WPSQLite)
                add_executable(WPGraphicsCookedResourceTests ClawCookedResourceTests.cpp)
                target_compile_definitions(WPGraphicsCookedResourceTests PRIVATE BOOST_ALL_NO_LIB)
                target_include_directories(WPGraphicsCookedResourceTests PRIVATE
                    ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost
                    ${WP_ROOT}/Engine/c/Include/WorkphoneCore
                    ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics
                    ${WP_ROOT}/Engine/c/Include/WorkphonePlatformWin32)
                target_link_libraries(WPGraphicsCookedResourceTests PRIVATE
                    WPGraphics Workphone WPSQLite WorkphoneGraphics WorkphoneCore WorkphonePlatformWin32 d3d11)
                target_link_directories(WPGraphicsCookedResourceTests PRIVATE
                    ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
                set_target_properties(WPGraphicsCookedResourceTests PROPERTIES FOLDER Tests/Cpp)
                add_test(NAME WPGraphics.cooked_material_resource COMMAND WPGraphicsCookedResourceTests)
                set_tests_properties(WPGraphics.cooked_material_resource PROPERTIES LABELS "graphics;gpu;assets" TIMEOUT 90)
            endif()
        endif()
		add_executable(WPGraphicsClawRendererTests ClawRendererContractTests.cpp)
		target_include_directories(
			WPGraphicsClawRendererTests
			PRIVATE ${WP_ROOT}/Engine/cpp/Include
					${WP_ROOT}/Engine/c/Include
					${WP_ROOT}/Engine/c/Include/WorkphoneCore
					${WP_ROOT}/Engine/c/Include/WorkphoneGraphics)
		target_compile_definitions(
			WPGraphicsClawRendererTests
			PRIVATE WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR
					WORKPHONE_INCLUDE_FONT_BAKING
					WORKPHONE_INCLUDE_DEFAULT_FONT
					WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT
					WORKPHONE_INCLUDE_STANDARD_IO)
		target_link_libraries(
			WPGraphicsClawRendererTests
			PRIVATE WPGraphics WorkphoneGraphics WorkphoneCore WorkphonePlatform)
		if (WIN32)
			target_link_directories(
				WPGraphicsClawRendererTests
				PRIVATE ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
		endif()
		set_target_properties(WPGraphicsClawRendererTests PROPERTIES FOLDER Tests/Cpp)
		add_test(NAME WPGraphics.claw_text_contract COMMAND WPGraphicsClawRendererTests)
		if (WIN32 AND WP_BUILD_RENDERER_DX11)
			add_executable(WPGraphicsClawDX11TargetTests ClawDX11TargetTests.cpp)
			target_include_directories(WPGraphicsClawDX11TargetTests PRIVATE
				${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Engine/c/Include
				${WP_ROOT}/Engine/c/Include/WorkphoneCore ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics)
			target_link_libraries(WPGraphicsClawDX11TargetTests PRIVATE
				WPGraphics WorkphoneGraphics WorkphoneCore WorkphonePlatform d3d11)
			target_link_directories(WPGraphicsClawDX11TargetTests PRIVATE
				${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
			set_target_properties(WPGraphicsClawDX11TargetTests PROPERTIES FOLDER Tests/Cpp)
			add_test(NAME WPGraphics.claw_dx11_targets COMMAND WPGraphicsClawDX11TargetTests)
		endif()
		add_executable(WPGraphicsClawUIDestructionTests ClawUIDestructionTests.cpp)
		target_include_directories(WPGraphicsClawUIDestructionTests PRIVATE
			${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Engine/c/Include
			${WP_ROOT}/Engine/c/Include/WorkphoneCore ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics)
		target_compile_definitions(WPGraphicsClawUIDestructionTests PRIVATE
			WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR WORKPHONE_INCLUDE_FONT_BAKING
			WORKPHONE_INCLUDE_DEFAULT_FONT WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT
			WORKPHONE_INCLUDE_STANDARD_IO)
		target_link_libraries(WPGraphicsClawUIDestructionTests PRIVATE
			WPGraphics WorkphoneGraphics WorkphoneCore WorkphonePlatform)
		if (WIN32)
			target_link_directories(WPGraphicsClawUIDestructionTests PRIVATE
				${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
		endif()
		set_target_properties(WPGraphicsClawUIDestructionTests PROPERTIES FOLDER Tests/Cpp)
		add_test(NAME WPGraphics.claw_ui_destruction COMMAND WPGraphicsClawUIDestructionTests)
	endif()
endif()



add_executable(VehicleAudioMixTests VehicleAudioMixTests.cpp)
target_include_directories(VehicleAudioMixTests PRIVATE ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Samples/cpp/VehicleAdvanced)
set_target_properties(VehicleAudioMixTests PROPERTIES FOLDER Tests/Cpp)
add_test(NAME WPVehicle.audio_mix COMMAND VehicleAudioMixTests)

add_executable(VehicleDecalTests VehicleDecalTests.cpp)
target_include_directories(VehicleDecalTests PRIVATE ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Samples/cpp/VehicleAdvanced)
set_target_properties(VehicleDecalTests PROPERTIES FOLDER Tests/Cpp)
add_test(NAME WPVehicle.skid_decals COMMAND VehicleDecalTests)

if(TARGET WorkphonePhysics)
    add_executable(PhysicsSceneCapacityTests PhysicsSceneCapacityTests.cpp)
    target_include_directories(PhysicsSceneCapacityTests PRIVATE
        ${WP_ROOT}/Engine/c/Include/WorkphonePhysics ${WP_ROOT}/Engine/c/Include/WorkphoneCore)
    target_link_libraries(PhysicsSceneCapacityTests PRIVATE WorkphonePhysics WorkphoneCore)
    set_target_properties(PhysicsSceneCapacityTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME WPPhysics.scene_capacity COMMAND PhysicsSceneCapacityTests)
    add_executable(VehicleHandlingTests VehicleHandlingTests.cpp)
    target_include_directories(VehicleHandlingTests PRIVATE ${WP_ROOT}/Engine/cpp/Include)
    set_target_properties(VehicleHandlingTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME WPVehicle.assisted_handling COMMAND VehicleHandlingTests)
    add_executable(VehicleInertiaTests VehicleInertiaTests.cpp)
    target_include_directories(VehicleInertiaTests PRIVATE
        ${WP_ROOT}/Engine/c/Include/WorkphonePhysics ${WP_ROOT}/Engine/c/Include/WorkphoneCore)
    target_link_libraries(VehicleInertiaTests PRIVATE WorkphonePhysics WorkphoneCore)
    set_target_properties(VehicleInertiaTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME WPPhysics.rotated_vehicle_inertia COMMAND VehicleInertiaTests)
endif()

if(TARGET Workphone)
    add_executable(StateContextListenerTests StateContextListenerTests.cpp)
    target_compile_definitions(StateContextListenerTests PRIVATE BOOST_ALL_NO_LIB)
    target_include_directories(StateContextListenerTests PRIVATE ${WP_ROOT}/Engine/cpp/Include)
    target_link_libraries(StateContextListenerTests PRIVATE Workphone)
    if(WIN32)
        target_link_directories(StateContextListenerTests PRIVATE
            ${WP_ROOT}/Dependencies/lib/windows/boost
            ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
    endif()
    set_target_properties(StateContextListenerTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME Workphone.state_listener_fanout COMMAND StateContextListenerTests)

    add_executable(MeshImposterTests MeshImposterTests.cpp)
    target_compile_definitions(MeshImposterTests PRIVATE BOOST_ALL_NO_LIB)
    target_include_directories(MeshImposterTests PRIVATE ${WP_ROOT}/Engine/cpp/Include)
    target_link_libraries(MeshImposterTests PRIVATE Workphone)
    if(WIN32)
        target_link_directories(MeshImposterTests PRIVATE
            ${WP_ROOT}/Dependencies/lib/windows/boost
            ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
    endif()
    set_target_properties(MeshImposterTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME Workphone.mesh_imposters COMMAND MeshImposterTests)
endif()

if(WIN32 AND WP_ENABLE_LUA AND TARGET WPLuaBind)
    # Scene bindings call concrete Lua classes which are not DLL-exported.
    # Compile the production host sources into this focused binding executable.
    add_executable(VehicleLuaBindingTests VehicleLuaBindingTests.cpp
        ${WP_ROOT}/Engine/cpp/Source/WPLua/LuaManager.cpp
        ${WP_ROOT}/Engine/cpp/Source/WPLua/LuaObjectData.cpp
        ${WP_ROOT}/Engine/cpp/Source/WPLua/NullScriptObject.cpp)
    target_compile_definitions(VehicleLuaBindingTests PRIVATE BOOST_ALL_NO_LIB BOOST_BIND_GLOBAL_PLACEHOLDERS WP_PROFILE_LUA_CALLS=0 WP_TEST_SOURCE_ROOT="${WP_ROOT}")
    target_include_directories(VehicleLuaBindingTests PRIVATE
        ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost
        ${WP_ROOT}/Dependencies/lua/inc ${WP_ROOT}/Dependencies/luabind/inc
        ${WP_ROOT}/Dependencies/luabind/inc/luabind)
    target_link_libraries(VehicleLuaBindingTests PRIVATE
        $<TARGET_LINKER_FILE:WPLuaBind> $<TARGET_LINKER_FILE:Workphone> lua lua_cjson luabind)
    add_dependencies(VehicleLuaBindingTests WPLuaBind Workphone)
    set_target_properties(VehicleLuaBindingTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME WPVehicle.lua_presentation_bindings COMMAND VehicleLuaBindingTests)

    add_executable(LuaRuntimeTests LuaRuntimeTests.cpp
        ${WP_ROOT}/Engine/cpp/Source/WPLua/LuaManager.cpp
        ${WP_ROOT}/Engine/cpp/Source/WPLua/LuaObjectData.cpp
        ${WP_ROOT}/Engine/cpp/Source/WPLua/NullScriptObject.cpp)
    target_compile_definitions(LuaRuntimeTests PRIVATE BOOST_ALL_NO_LIB BOOST_BIND_GLOBAL_PLACEHOLDERS WP_PROFILE_LUA_CALLS=0)
    target_include_directories(LuaRuntimeTests PRIVATE
        ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost
        ${WP_ROOT}/Dependencies/lua/inc ${WP_ROOT}/Dependencies/luabind/inc
        ${WP_ROOT}/Dependencies/luabind/inc/luabind)
    target_link_libraries(LuaRuntimeTests PRIVATE
        $<TARGET_LINKER_FILE:WPLuaBind> $<TARGET_LINKER_FILE:Workphone> lua lua_cjson luabind)
    add_dependencies(LuaRuntimeTests WPLuaBind Workphone)
    if(TARGET WPSQLite)
        target_compile_definitions(LuaRuntimeTests PRIVATE WP_LUA_ASSET_TESTS=1)
        target_link_libraries(LuaRuntimeTests PRIVATE $<TARGET_LINKER_FILE:WPSQLite>)
        add_dependencies(LuaRuntimeTests WPSQLite)
    endif()
    set_target_properties(LuaRuntimeTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME WPLua.runtime_contracts COMMAND LuaRuntimeTests)
    foreach(lua_host_target VehicleLuaBindingTests LuaRuntimeTests)
        target_sources(${lua_host_target} PRIVATE
            ${WP_ROOT}/Engine/cpp/Source/WPLua/LuaScriptCompiler.cpp)
        target_compile_definitions(${lua_host_target} PRIVATE WPLua_EXPORTS)
    endforeach()
endif()
