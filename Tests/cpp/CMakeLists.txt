set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cpp_defines)
include(cpp_compiler_options)

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



if(TARGET WorkphonePhysics)
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
