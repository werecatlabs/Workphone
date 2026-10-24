if(WP_BUILD_TESTS AND TARGET WorkphoneAudio)
    #add_subdirectory(./AudioProduction AudioProduction)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cpp_defines)
include(cpp_compiler_options)

if (WIN32 AND WP_GRAPHICS_SYSTEM_CLAW)
    include_directories( ${WP_ROOT}/Engine/c/Include )
endif()

if(TARGET Workphone)
    add_executable(WPMaterialParameterLayoutTests MaterialParameterLayoutTests.cpp)
    target_include_directories(WPMaterialParameterLayoutTests PRIVATE
        ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost)
    target_compile_definitions(WPMaterialParameterLayoutTests PRIVATE BOOST_ALL_NO_LIB)
    target_link_libraries(WPMaterialParameterLayoutTests PRIVATE Workphone)
    if(WIN32)
        target_link_directories(WPMaterialParameterLayoutTests PRIVATE
            ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
    endif()
    set_target_properties(WPMaterialParameterLayoutTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME WPGraphics.material_parameter_layout COMMAND WPMaterialParameterLayoutTests)
    set_tests_properties(WPGraphics.material_parameter_layout PROPERTIES LABELS "graphics;material;headless" TIMEOUT 30)
endif()

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
    if(WP_BUILD_RENDERER_DX11)
        add_executable(WPMaterialProgramBindingTests MaterialProgramBindingTests.cpp)
        target_include_directories(WPMaterialProgramBindingTests PRIVATE
            ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Engine/c/Include
            ${WP_ROOT}/Engine/c/Include/WorkphoneCore
            ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics ${WP_ROOT}/Dependencies/boost)
        target_compile_definitions(WPMaterialProgramBindingTests PRIVATE BOOST_ALL_NO_LIB)
        target_link_libraries(WPMaterialProgramBindingTests PRIVATE
            WPGraphics Workphone WorkphoneGraphics WorkphoneCore d3d11 d3dcompiler dxguid)
        target_link_directories(WPMaterialProgramBindingTests PRIVATE
            ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
        set_target_properties(WPMaterialProgramBindingTests PROPERTIES FOLDER Tests/Cpp)
        add_test(NAME WPGraphics.material_program_binding COMMAND WPMaterialProgramBindingTests)
        set_tests_properties(WPGraphics.material_program_binding PROPERTIES
            LABELS "graphics;material;gpu;packing" TIMEOUT 60)
    endif()
    if(TARGET WPImGui)
        add_executable(ImGuiViewportInputTests ImGuiViewportInputTests.cpp)
        target_include_directories(ImGuiViewportInputTests PRIVATE
            ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Engine/c/Include
            ${WP_ROOT}/Engine/c/Include/WorkphoneCore ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics
            ${WP_ROOT}/Engine/c/Include/WorkphonePlatformWin32
            ${WP_ROOT}/Dependencies/imgui ${WP_ROOT}/Dependencies/boost)
        target_compile_definitions(ImGuiViewportInputTests PRIVATE BOOST_ALL_NO_LIB
            WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR WORKPHONE_INCLUDE_FONT_BAKING
            WORKPHONE_INCLUDE_DEFAULT_FONT WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT
            WORKPHONE_INCLUDE_STANDARD_IO)
        target_link_libraries(ImGuiViewportInputTests PRIVATE WPImGui imgui WPGraphics Workphone
            WorkphoneCore WorkphoneGraphics WorkphonePlatformWin32 d3d11 dxgi d3dcompiler dxguid)
        target_link_directories(ImGuiViewportInputTests PRIVATE
            ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>
            ${WP_ROOT}/Dependencies/lib/windows/boost)
        set_target_properties(ImGuiViewportInputTests PROPERTIES FOLDER Tests/Cpp)
        add_test(NAME WPImGui.viewport_input COMMAND ImGuiViewportInputTests)
        set_tests_properties(WPImGui.viewport_input PROPERTIES
            LABELS "ui;editor;headless" TIMEOUT 30)
    endif()
    # Keep the active wrapper/backend regression suite buildable. Retained
    # executables cannot validate changes to the native renderer boundary.
    foreach(graphics_contract IN ITEMS ClawRendererContract ClawDX11Target ClawProduction)
        if(graphics_contract STREQUAL "ClawRendererContract")
            set(graphics_target WPGraphicsClawRendererTests)
            set(graphics_test WPGraphics.claw_text_contract)
        elseif(graphics_contract STREQUAL "ClawDX11Target")
            set(graphics_target WPGraphicsClawDX11TargetTests)
            set(graphics_test WPGraphics.claw_dx11_targets)
        else()
            set(graphics_target WPGraphicsClawProductionTests)
            set(graphics_test WPGraphics.claw_production_render)
        endif()
        if(WP_BUILD_RENDERER_DX11)
            add_executable(${graphics_target} ${graphics_contract}Tests.cpp)
            target_include_directories(${graphics_target} PRIVATE
                ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Engine/c/Include
                ${WP_ROOT}/Engine/c/Include/WorkphoneCore
                ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics)
            target_compile_definitions(${graphics_target} PRIVATE
                WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR WORKPHONE_INCLUDE_FONT_BAKING
                WORKPHONE_INCLUDE_DEFAULT_FONT WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT
                WORKPHONE_INCLUDE_STANDARD_IO BOOST_ALL_NO_LIB)
            target_link_libraries(${graphics_target} PRIVATE
                WPGraphics Workphone WorkphoneGraphics WorkphoneCore WorkphonePlatform d3d11)
            if(graphics_contract STREQUAL "ClawProduction")
                target_link_libraries(${graphics_target} PRIVATE dbghelp)
            endif()
            target_link_directories(${graphics_target} PRIVATE
                ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
            if(graphics_contract STREQUAL "ClawDX11Target" AND TARGET imgui)
                # The manager is internal to WPGraphics; compile it alongside the
                # test's ImGui context instead of exporting an additional DLL API.
                target_sources(${graphics_target} PRIVATE
                    ${WP_ROOT}/Engine/cpp/Source/WPGraphics/ClawImguiManager.cpp)
                target_include_directories(${graphics_target} PRIVATE
                    ${WP_ROOT}/Dependencies/imgui ${WP_ROOT}/Dependencies/boost
                    ${WP_ROOT}/Engine/c/Include/WorkphonePlatformWin32)
                target_compile_definitions(${graphics_target} PRIVATE
                    WP_TEST_IMGUI WP_TEST_ICON_FONT="${WP_ROOT}/bin/Media/fa-solid-900.ttf")
            endif()
            set_target_properties(${graphics_target} PROPERTIES FOLDER Tests/Cpp)
            add_test(NAME ${graphics_test} COMMAND ${graphics_target})
            set_tests_properties(${graphics_test} PROPERTIES LABELS "graphics;gpu" TIMEOUT 120)
            if(graphics_contract STREQUAL "ClawProduction")
                add_test(NAME WPGraphics.claw_lifecycle_soak
                    COMMAND ${graphics_target} --lifecycle-soak 32)
                set_tests_properties(WPGraphics.claw_lifecycle_soak PROPERTIES
                    LABELS "graphics;headless;lifecycle" TIMEOUT 120)
            endif()
        endif()
    endforeach()
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
    add_executable(WPGraphicsClawUIDestructionTests ClawUIDestructionTests.cpp)
    target_include_directories(WPGraphicsClawUIDestructionTests PRIVATE
        ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Engine/c/Include
        ${WP_ROOT}/Engine/c/Include/WorkphoneCore ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics)
    target_compile_definitions(WPGraphicsClawUIDestructionTests PRIVATE
        WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR WORKPHONE_INCLUDE_FONT_BAKING
        WORKPHONE_INCLUDE_DEFAULT_FONT WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT
        WORKPHONE_INCLUDE_STANDARD_IO BOOST_ALL_NO_LIB)
    target_link_libraries(WPGraphicsClawUIDestructionTests PRIVATE
        WPGraphics Workphone WorkphoneGraphics WorkphoneCore WorkphonePlatform)
    target_link_directories(WPGraphicsClawUIDestructionTests PRIVATE
        ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
    set_target_properties(WPGraphicsClawUIDestructionTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME WPGraphics.claw_ui_destruction COMMAND WPGraphicsClawUIDestructionTests)
    set_tests_properties(WPGraphics.claw_ui_destruction PROPERTIES LABELS "graphics;headless;lifecycle" TIMEOUT 60)
endif()

if (WP_BUILD_TESTS)
    add_executable(WPFoliageAuthoringTests FoliageAuthoringTests.cpp)
    target_compile_definitions(WPFoliageAuthoringTests PRIVATE BOOST_ALL_NO_LIB)
    target_include_directories(WPFoliageAuthoringTests PRIVATE
        ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost)
    target_link_libraries(WPFoliageAuthoringTests PRIVATE $<TARGET_LINKER_FILE:Workphone>)
    add_dependencies(WPFoliageAuthoringTests Workphone)
    if(WIN32)
        target_link_directories(WPFoliageAuthoringTests PRIVATE
            ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
    endif()
    set_target_properties(WPFoliageAuthoringTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME WPGraphics.foliage_authoring COMMAND WPFoliageAuthoringTests)
    set_tests_properties(WPGraphics.foliage_authoring PROPERTIES LABELS "foliage;headless;authoring" TIMEOUT 60)
    add_test(NAME WPGraphics.foliage_plant_geometry COMMAND WPFoliageAuthoringTests --plant-only)
    set_tests_properties(WPGraphics.foliage_plant_geometry PROPERTIES LABELS "foliage;headless;geometry" TIMEOUT 30)
    add_executable(WPTerrainContractsTests TerrainContractsTests.cpp)
    target_compile_definitions(WPTerrainContractsTests PRIVATE BOOST_ALL_NO_LIB)
    target_include_directories(WPTerrainContractsTests PRIVATE ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost)
    target_link_libraries(WPTerrainContractsTests PRIVATE Workphone $<$<PLATFORM_ID:Windows>:psapi>)
    if(WIN32)
        target_link_directories(WPTerrainContractsTests PRIVATE
            ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
    endif()
    set_target_properties(WPTerrainContractsTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME WPGraphics.terrain_contracts COMMAND WPTerrainContractsTests)
    set_tests_properties(WPGraphics.terrain_contracts PROPERTIES LABELS "terrain;editor;headless" TIMEOUT 60)
    add_test(NAME WPGraphics.terrain_representative_performance COMMAND WPTerrainContractsTests --performance)
    set_tests_properties(WPGraphics.terrain_representative_performance PROPERTIES LABELS "terrain;performance" TIMEOUT 900)
    # 1,000 regeneration/load/unload/resize cycles per the review's soak scenario.
    add_test(NAME WPGraphics.terrain_regeneration_soak COMMAND WPTerrainContractsTests --soak 1000)
    set_tests_properties(WPGraphics.terrain_regeneration_soak PROPERTIES LABELS "terrain;soak" TIMEOUT 900)
    if(TARGET WPPhysics)
        foreach(contract IN ITEMS Foliage TerrainPhysics)
            set(contract_target WP${contract}ContractsTests)
            add_executable(${contract_target} ${contract}ContractsTests.cpp)
            target_compile_definitions(${contract_target} PRIVATE BOOST_ALL_NO_LIB)
            target_include_directories(${contract_target} PRIVATE
                ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Engine/c/Include ${WP_ROOT}/Dependencies/boost
                ${WP_ROOT}/Engine/c/Include/WorkphonePhysics ${WP_ROOT}/Engine/c/Include/WorkphoneCore)
            target_link_libraries(${contract_target} PRIVATE Workphone WPPhysics WorkphonePhysics WorkphoneGraphics)
            if(WIN32)
                target_link_directories(${contract_target} PRIVATE
                    ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
            endif()
            set_target_properties(${contract_target} PROPERTIES FOLDER Tests/Cpp)
        endforeach()
        add_test(NAME WPGraphics.foliage_contracts COMMAND WPFoliageContractsTests)
        add_test(NAME WPGraphics.terrain_physics COMMAND WPTerrainPhysicsContractsTests)
        set_tests_properties(WPGraphics.foliage_contracts WPGraphics.terrain_physics PROPERTIES
            LABELS "graphics;headless;physics" TIMEOUT 120)
    endif()
    if(TARGET WPSQLite)
        add_executable(WPAssetCatalogTests AssetCatalogTests.cpp)
        if(WP_BUILD_RESOURCE_SYSTEM)
            target_compile_definitions(WPAssetCatalogTests PRIVATE WP_CATALOG_RESOURCE_SYSTEM_TESTS=1)
        endif()
        target_include_directories(WPAssetCatalogTests PRIVATE ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost)
        target_link_libraries(WPAssetCatalogTests PRIVATE $<TARGET_LINKER_FILE:WPSQLite> $<TARGET_LINKER_FILE:Workphone>)
        add_dependencies(WPAssetCatalogTests WPSQLite Workphone)

		if(WP_BUILD_EDITOR)
            target_sources(WPAssetCatalogTests PRIVATE
                ${WP_ROOT}/Tools/cpp/Editor/src/commands/Command.cpp
                ${WP_ROOT}/Tools/cpp/Editor/src/commands/ModifyPropertyCmd.cpp
                ${WP_ROOT}/Tools/cpp/Editor/src/commands/AddResourceCmd.cpp
                ${WP_ROOT}/Tools/cpp/Editor/src/jobs/ProjectCleanJob.cpp)
            target_include_directories(WPAssetCatalogTests PRIVATE ${WP_ROOT}/Tools/cpp/Editor/src)
            target_compile_definitions(WPAssetCatalogTests PRIVATE WP_EDITOR_ASSET_COMMAND_CONTRACTS=1)
        endif()

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

	#if (WP_GRAPHICS_SYSTEM_CLAW AND TARGET WPGraphics)
    #    if(WIN32 AND WP_BUILD_RENDERER_DX11)
    #        add_executable(WPGraphicsClawProductionTests ClawProductionTests.cpp)
    #        target_include_directories(WPGraphicsClawProductionTests PRIVATE
    #            ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Engine/c/Include
    #            ${WP_ROOT}/Engine/c/Include/WorkphoneCore ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics)
    #        target_link_libraries(WPGraphicsClawProductionTests PRIVATE
    #            WPGraphics Workphone WorkphoneGraphics WorkphoneCore WorkphonePlatform d3d11)
    #        target_link_directories(WPGraphicsClawProductionTests PRIVATE
    #            ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
    #        set_target_properties(WPGraphicsClawProductionTests PROPERTIES FOLDER Tests/Cpp)
    #        add_test(NAME WPGraphics.claw_production_render COMMAND WPGraphicsClawProductionTests)
    #        set_tests_properties(WPGraphics.claw_production_render PROPERTIES LABELS "graphics;gpu;production" TIMEOUT 60)
    if(WIN32 AND WP_GRAPHICS_SYSTEM_CLAW AND WP_BUILD_RENDERER_DX11 AND TARGET WPGraphics)
            add_executable(WPScenePickingTests ScenePickingTests.cpp)
            target_compile_definitions(WPScenePickingTests PRIVATE BOOST_ALL_NO_LIB)
            target_include_directories(WPScenePickingTests PRIVATE
                ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Engine/c/Include ${WP_ROOT}/Dependencies/boost
                ${WP_ROOT}/Engine/c/Include/WorkphoneCore ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics)
            target_link_libraries(WPScenePickingTests PRIVATE Workphone)
            target_link_directories(WPScenePickingTests PRIVATE
                ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
            set_target_properties(WPScenePickingTests PROPERTIES FOLDER Tests/Cpp)
            add_test(NAME WPEditor.scene_picking COMMAND WPScenePickingTests)
            set_tests_properties(WPEditor.scene_picking PROPERTIES LABELS "editor;graphics;headless" TIMEOUT 30)
            if(WP_BUILD_RESOURCE_SYSTEM AND TARGET WPSQLite)
                add_executable(WPMaterialLoadSequenceTests MaterialLoadSequenceTests.cpp)
                target_compile_definitions(WPMaterialLoadSequenceTests PRIVATE BOOST_ALL_NO_LIB)
                target_include_directories(WPMaterialLoadSequenceTests PRIVATE
                    ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Engine/c/Include ${WP_ROOT}/Dependencies/boost
                    ${WP_ROOT}/Engine/c/Include/WorkphoneCore
                    ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics
                    ${WP_ROOT}/Engine/c/Include/WorkphonePlatformWin32)
                target_link_libraries(WPMaterialLoadSequenceTests PRIVATE WPGraphics Workphone WPSQLite)
                target_link_directories(WPMaterialLoadSequenceTests PRIVATE
                    ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
                set_target_properties(WPMaterialLoadSequenceTests PROPERTIES FOLDER Tests/Cpp)
                add_test(NAME WPGraphics.material_load_sequence COMMAND WPMaterialLoadSequenceTests)
                set_tests_properties(WPGraphics.material_load_sequence PROPERTIES LABELS "graphics;assets;scene;headless" TIMEOUT 30)
                add_test(NAME WPGraphics.material_heap_profile_contract
                    COMMAND WPMaterialLoadSequenceTests --heap-profile-self-test)
                set_tests_properties(WPGraphics.material_heap_profile_contract PROPERTIES
                    LABELS "graphics;material;memory;headless" TIMEOUT 30)
                add_test(NAME WPGraphics.material_memory_profile_contract
                    COMMAND WPMaterialLoadSequenceTests --profile-full-memory 64 128)
                set_tests_properties(WPGraphics.material_memory_profile_contract PROPERTIES
                    LABELS "graphics;material;memory;headless" TIMEOUT 60)
                add_executable(WPGraphicsCookedResourceTests ClawCookedResourceTests.cpp)
                target_compile_definitions(WPGraphicsCookedResourceTests PRIVATE BOOST_ALL_NO_LIB)
                target_include_directories(WPGraphicsCookedResourceTests PRIVATE
                    ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Engine/c/Include ${WP_ROOT}/Dependencies/boost
                    ${WP_ROOT}/Engine/c/Include/WorkphoneCore
                    ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics
                    ${WP_ROOT}/Engine/c/Include/WorkphonePlatformWin32)
                target_link_libraries(WPGraphicsCookedResourceTests PRIVATE
                    WPGraphics Workphone WPSQLite WorkphoneGraphics WorkphoneCore WorkphonePlatformWin32 d3d11)
                if(TARGET WPProcedural)
                    target_link_libraries(WPGraphicsCookedResourceTests PRIVATE WPProcedural)
                    target_compile_definitions(WPGraphicsCookedResourceTests PRIVATE WP_COOKED_PROCEDURAL=1)
                else()
                    target_compile_definitions(WPGraphicsCookedResourceTests PRIVATE WP_COOKED_PROCEDURAL=0)
                endif()
                target_link_directories(WPGraphicsCookedResourceTests PRIVATE
                    ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
                set_target_properties(WPGraphicsCookedResourceTests PROPERTIES FOLDER Tests/Cpp)
                add_test(NAME WPGraphics.cooked_material_resource COMMAND WPGraphicsCookedResourceTests)
                set_tests_properties(WPGraphics.cooked_material_resource PROPERTIES LABELS "graphics;gpu;assets" TIMEOUT 90)
            endif()
    endif()
    #    endif()
	#	add_executable(WPGraphicsClawRendererTests ClawRendererContractTests.cpp)
	#	target_include_directories(
	#		WPGraphicsClawRendererTests
	#		PRIVATE ${WP_ROOT}/Engine/cpp/Include
	#				${WP_ROOT}/Engine/c/Include
	#				${WP_ROOT}/Engine/c/Include/WorkphoneCore
	#				${WP_ROOT}/Engine/c/Include/WorkphoneGraphics)
	#	target_compile_definitions(
	#		WPGraphicsClawRendererTests
	#		PRIVATE WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR
	#				WORKPHONE_INCLUDE_FONT_BAKING
	#				WORKPHONE_INCLUDE_DEFAULT_FONT
	#				WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT
	#				WORKPHONE_INCLUDE_STANDARD_IO)
	#	target_link_libraries(
	#		WPGraphicsClawRendererTests
	#		PRIVATE WPGraphics WorkphoneGraphics WorkphoneCore WorkphonePlatform)
	#	if (WIN32)
	#		target_link_directories(
	#			WPGraphicsClawRendererTests
	#			PRIVATE ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
	#	endif()
	#	set_target_properties(WPGraphicsClawRendererTests PROPERTIES FOLDER Tests/Cpp)
	#	add_test(NAME WPGraphics.claw_text_contract COMMAND WPGraphicsClawRendererTests)
	#	if (WIN32 AND WP_BUILD_RENDERER_DX11)
	#		add_executable(WPGraphicsClawDX11TargetTests ClawDX11TargetTests.cpp)
	#		target_include_directories(WPGraphicsClawDX11TargetTests PRIVATE
	#			${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Engine/c/Include
	#			${WP_ROOT}/Engine/c/Include/WorkphoneCore ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics)
	#		target_link_libraries(WPGraphicsClawDX11TargetTests PRIVATE
	#			WPGraphics WorkphoneGraphics WorkphoneCore WorkphonePlatform d3d11)
	#		target_link_directories(WPGraphicsClawDX11TargetTests PRIVATE
	#			${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
	#		set_target_properties(WPGraphicsClawDX11TargetTests PROPERTIES FOLDER Tests/Cpp)
	#		add_test(NAME WPGraphics.claw_dx11_targets COMMAND WPGraphicsClawDX11TargetTests)
	#	endif()
	#	add_executable(WPGraphicsClawUIDestructionTests ClawUIDestructionTests.cpp)
	#	target_include_directories(WPGraphicsClawUIDestructionTests PRIVATE
	#		${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Engine/c/Include
	#		${WP_ROOT}/Engine/c/Include/WorkphoneCore ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics)
	#	target_compile_definitions(WPGraphicsClawUIDestructionTests PRIVATE
	#		WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR WORKPHONE_INCLUDE_FONT_BAKING
	#		WORKPHONE_INCLUDE_DEFAULT_FONT WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT
	#		WORKPHONE_INCLUDE_STANDARD_IO)
	#	target_link_libraries(WPGraphicsClawUIDestructionTests PRIVATE
	#		WPGraphics WorkphoneGraphics WorkphoneCore WorkphonePlatform)
	#	if (WIN32)
	#		target_link_directories(WPGraphicsClawUIDestructionTests PRIVATE
	#			${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
	#	endif()
	#	set_target_properties(WPGraphicsClawUIDestructionTests PROPERTIES FOLDER Tests/Cpp)
	#	add_test(NAME WPGraphics.claw_ui_destruction COMMAND WPGraphicsClawUIDestructionTests)
	#endif()
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
    add_executable(WPAnimationCoreTests AnimationCoreTests.cpp
        ${WP_ROOT}/Tests/cpp/UnitTests/AnimationGraphTests.cpp ${WP_ROOT}/Tests/cpp/UnitTests/AnimationIKTests.cpp)
    target_compile_definitions(WPAnimationCoreTests PRIVATE BOOST_ALL_NO_LIB WIN32_LEAN_AND_MEAN)
    target_include_directories(WPAnimationCoreTests PRIVATE
        ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost ${WP_ROOT}/Tests/cpp/UnitTests)
    target_link_libraries(WPAnimationCoreTests PRIVATE $<TARGET_LINKER_FILE:Workphone>)
    add_dependencies(WPAnimationCoreTests Workphone)
    set_target_properties(WPAnimationCoreTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME WPAnimation.core_contracts COMMAND WPAnimationCoreTests)
    if(TARGET WPAssimp)
        add_executable(WPAnimationImportTests AnimationImportTests.cpp)
        target_compile_definitions(WPAnimationImportTests PRIVATE BOOST_ALL_NO_LIB WIN32_LEAN_AND_MEAN WP_TEST_SOURCE_ROOT="${WP_ROOT}")
        target_include_directories(WPAnimationImportTests PRIVATE
            ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost
            ${WP_ROOT}/Engine/cpp/Include/WPAssimp/Extern/assimp)
        target_link_libraries(WPAnimationImportTests PRIVATE
            $<TARGET_LINKER_FILE:WPAssimp> $<TARGET_LINKER_FILE:Workphone>)
        if(WIN32)
            target_include_directories(WPAnimationImportTests PRIVATE ${WP_ROOT}/Engine/cpp/Include/WPAssimp/Extern/assimp/vc142)
        elseif(APPLE)
            target_include_directories(WPAnimationImportTests PRIVATE ${WP_ROOT}/Engine/cpp/Include/WPAssimp/Extern/assimp/xcode)
        else()
            target_include_directories(WPAnimationImportTests PRIVATE ${WP_ROOT}/Engine/cpp/Include/WPAssimp/Extern/assimp/linux)
        endif()
        add_dependencies(WPAnimationImportTests WPAssimp Workphone)
        set_target_properties(WPAnimationImportTests PROPERTIES FOLDER Tests/Cpp)
        add_test(NAME WPAnimation.import_contracts COMMAND WPAnimationImportTests)
        if(TARGET Meshopt)
            add_executable(WPAssimpCompressedImportTests AssimpCompressedImportTests.cpp)
            target_include_directories(WPAssimpCompressedImportTests PRIVATE
                ${WP_ROOT}/Engine/cpp/Include
                ${WP_ROOT}/Engine/cpp/Include/WPAssimp/Extern/assimp)
            if(WIN32)
                target_include_directories(WPAssimpCompressedImportTests PRIVATE
                    ${WP_ROOT}/Engine/cpp/Include/WPAssimp/Extern/assimp/vc142)
            elseif(APPLE)
                target_include_directories(WPAssimpCompressedImportTests PRIVATE
                    ${WP_ROOT}/Engine/cpp/Include/WPAssimp/Extern/assimp/xcode)
            else()
                target_include_directories(WPAssimpCompressedImportTests PRIVATE
                    ${WP_ROOT}/Engine/cpp/Include/WPAssimp/Extern/assimp/linux)
            endif()
            target_link_libraries(WPAssimpCompressedImportTests PRIVATE
                $<TARGET_LINKER_FILE:WPAssimp> Meshopt)
            add_dependencies(WPAssimpCompressedImportTests WPAssimp)
            set_target_properties(WPAssimpCompressedImportTests PROPERTIES FOLDER Tests/Cpp)
            add_test(NAME WPAssimp.compressed_import COMMAND WPAssimpCompressedImportTests)
            set_tests_properties(WPAssimp.compressed_import PROPERTIES
                LABELS "assets;import;headless" TIMEOUT 30)
        endif()
    endif()

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

    add_executable(OpenCityLayoutTests OpenCityLayoutTests.cpp)
    target_compile_definitions(OpenCityLayoutTests PRIVATE BOOST_ALL_NO_LIB)
    target_include_directories(OpenCityLayoutTests PRIVATE ${WP_ROOT}/Engine/cpp/Include)
    target_link_libraries(OpenCityLayoutTests PRIVATE Workphone)
    if(WIN32)
        target_link_directories(OpenCityLayoutTests PRIVATE
            ${WP_ROOT}/Dependencies/lib/windows/boost
            ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
    endif()
    set_target_properties(OpenCityLayoutTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME Workphone.open_city_layout COMMAND OpenCityLayoutTests)
endif()

if(WIN32 AND WP_ENABLE_LUA AND TARGET WPLuaBind)
    add_executable(TerrainLuaBindingTests TerrainLuaBindingTests.cpp
        ${WP_ROOT}/Engine/cpp/Source/WPLua/LuaManager.cpp
        ${WP_ROOT}/Engine/cpp/Source/WPLua/LuaObjectData.cpp
        ${WP_ROOT}/Engine/cpp/Source/WPLua/NullScriptObject.cpp
        ${WP_ROOT}/Engine/cpp/Source/WPLua/LuaScriptCompiler.cpp)
    target_compile_definitions(TerrainLuaBindingTests PRIVATE BOOST_ALL_NO_LIB BOOST_BIND_GLOBAL_PLACEHOLDERS
        WP_PROFILE_LUA_CALLS=0 WPLua_EXPORTS WP_TEST_SOURCE_ROOT="${WP_ROOT}")
    target_include_directories(TerrainLuaBindingTests PRIVATE
        ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost
        ${WP_ROOT}/Dependencies/lua/inc ${WP_ROOT}/Dependencies/luabind/inc
        ${WP_ROOT}/Dependencies/luabind/inc/luabind)
    target_link_libraries(TerrainLuaBindingTests PRIVATE
        $<TARGET_LINKER_FILE:WPLuaBind> $<TARGET_LINKER_FILE:Workphone> lua lua_cjson luabind)
    target_link_directories(TerrainLuaBindingTests PRIVATE
        ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
    add_dependencies(TerrainLuaBindingTests WPLuaBind Workphone)
    set_target_properties(TerrainLuaBindingTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME WPGraphics.terrain_lua_workflow COMMAND TerrainLuaBindingTests)
    set_tests_properties(WPGraphics.terrain_lua_workflow PROPERTIES LABELS "terrain;editor;lua" TIMEOUT 60)
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
        if(WIN32)
            target_link_directories(${lua_host_target} PRIVATE
                ${WP_ROOT}/Dependencies/lib/windows/boost
                ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
        endif()
        target_sources(${lua_host_target} PRIVATE
            ${WP_ROOT}/Engine/cpp/Source/WPLua/LuaScriptCompiler.cpp)
        target_compile_definitions(${lua_host_target} PRIVATE WPLua_EXPORTS)
    endforeach()
endif()

# Aggregate target covering the terrain validation suite referenced by
# docs/TERRAIN_WORKFLOW.md ("--target WPTerrainValidation"). Dependencies are
# conditional so the target stays valid across reduced configures.
if (WP_BUILD_TESTS)
    set(WP_TERRAIN_VALIDATION_TARGETS WPTerrainContractsTests)
    if(TARGET WPTerrainPhysicsContractsTests)
        list(APPEND WP_TERRAIN_VALIDATION_TARGETS WPTerrainPhysicsContractsTests)
    endif()
    if(TARGET TerrainLuaBindingTests)
        list(APPEND WP_TERRAIN_VALIDATION_TARGETS TerrainLuaBindingTests)
    endif()
    if(TARGET WPGraphicsClawProductionTests)
        list(APPEND WP_TERRAIN_VALIDATION_TARGETS WPGraphicsClawProductionTests)
    endif()
    if(TARGET WPGraphicsCookedResourceTests)
        list(APPEND WP_TERRAIN_VALIDATION_TARGETS WPGraphicsCookedResourceTests)
    endif()
    if(TARGET Editor)
        list(APPEND WP_TERRAIN_VALIDATION_TARGETS Editor)
    endif()
    add_custom_target(WPTerrainValidation DEPENDS ${WP_TERRAIN_VALIDATION_TARGETS})
    set_target_properties(WPTerrainValidation PROPERTIES FOLDER Tests)
endif()
if(WP_BUILD_TESTS AND TARGET Workphone)
    add_executable(WPPlanarGraphTests PlanarGraphTests.cpp)
    target_include_directories(WPPlanarGraphTests PRIVATE
        ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost)
    target_link_libraries(WPPlanarGraphTests PRIVATE $<TARGET_LINKER_FILE:Workphone>)
    add_dependencies(WPPlanarGraphTests Workphone)
    set_target_properties(WPPlanarGraphTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME WPProcedural.planar_graph COMMAND WPPlanarGraphTests)
endif()

if(WP_BUILD_TESTS AND TARGET WPProcedural)
    add_executable(WPProceduralCityTests
        ProceduralCityTestMain.cpp UnitTests/ProceduralCityTests.cpp)
    target_include_directories(WPProceduralCityTests PRIVATE
        ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost)
    target_compile_definitions(WPProceduralCityTests PRIVATE BOOST_ALL_NO_LIB)
    target_link_libraries(WPProceduralCityTests PRIVATE
        $<TARGET_LINKER_FILE:Workphone> $<TARGET_LINKER_FILE:WPProcedural>)
    add_dependencies(WPProceduralCityTests Workphone WPProcedural)
    set_target_properties(WPProceduralCityTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME WPProcedural.city_production COMMAND WPProceduralCityTests --report_level=detailed)
    set_tests_properties(WPProcedural.city_production PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:WPProceduralCityTests>"
        LABELS "procedural;city;headless" TIMEOUT 120)

    add_executable(WPBlockGeneratorTests BlockGeneratorTests.cpp CityGeneratorTests.cpp)
    target_include_directories(WPBlockGeneratorTests PRIVATE
        ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost)
    target_compile_definitions(WPBlockGeneratorTests PRIVATE BOOST_ALL_NO_LIB)
    target_link_libraries(WPBlockGeneratorTests PRIVATE
        $<TARGET_LINKER_FILE:Workphone> $<TARGET_LINKER_FILE:WPProcedural>)
    add_dependencies(WPBlockGeneratorTests Workphone WPProcedural)
    set_target_properties(WPBlockGeneratorTests PROPERTIES FOLDER Tests/Cpp)
    add_test(NAME WPProcedural.block_generator COMMAND WPBlockGeneratorTests)
    set_tests_properties(WPProcedural.block_generator PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:WPBlockGeneratorTests>" TIMEOUT 60)
endif()
