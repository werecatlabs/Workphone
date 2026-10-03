#ifndef WPPhysxAutoLink_h__
#define WPPhysxAutoLink_h__

#if WP_USE_AUTO_LINK
#    ifdef _DEBUG
#        if WP_ARCH_TYPE == WP_ARCHITECTURE_32
#            pragma comment( lib, "WPPhysx.lib" )
#            pragma comment( lib, "PhysX3DEBUG_x86.lib" )
// #pragma comment(lib, "FoundationCHECKED.lib")
#            pragma comment( lib, "PhysX3CommonDEBUG_x86.lib" )

// #pragma comment(lib, "PhysX3CommonCHECKED.lib")
#            pragma comment( lib, "PhysX3CommonCHECKED_x86.lib" )

#            pragma comment( lib, "PhysX3CookingDEBUG_x86.lib" )
#            pragma comment( lib, "PhysX3ExtensionsDEBUG.lib" )
#            pragma comment( lib, "PhysX3CharacterKinematicDEBUG_x86.lib" )
#            pragma comment( lib, "PhysX3VehicleDEBUG.lib" )
#            pragma comment( lib, "PhysXVisualDebuggerSDKDEBUG.lib" )
#        elif WP_ARCH_TYPE == WP_ARCHITECTURE_64
#            pragma comment( lib, "WPPhysx.lib" )
#            pragma comment( lib, "PhysX3DEBUG_x64.lib" )
// #pragma comment(lib, "FoundationCHECKED.lib")
#            pragma comment( lib, "PhysX3CommonDEBUG_x64.lib" )

// #pragma comment(lib, "PhysX3CommonCHECKED.lib")
#            pragma comment( lib, "PhysX3CommonCHECKED_x64.lib" )

#            pragma comment( lib, "PhysX3CookingDEBUG_x64.lib" )
#            pragma comment( lib, "PhysX3ExtensionsDEBUG.lib" )
#            pragma comment( lib, "PhysX3CharacterKinematicDEBUG_x64.lib" )
#            pragma comment( lib, "PhysX3VehicleDEBUG.lib" )
#            pragma comment( lib, "PhysXVisualDebuggerSDKDEBUG.lib" )
#        endif
#    elif NDEBUG
#        if WP_ARCH_TYPE == WP_ARCHITECTURE_32
#            pragma comment( lib, "PhysX3_x86.lib" )
#            pragma comment( lib, "PhysX3Extensions.lib" )
#            pragma comment( lib, "PhysX3Common_x86.lib" )
#            pragma comment( lib, "PhysX3Vehicle.lib" )
#            pragma comment( lib, "PhysX3Cooking_x86.lib" )
#            pragma comment( lib, "PhysXProfileSDK.lib" )
#            pragma comment( lib, "PxTask.lib" )
#        else
#            pragma comment( lib, "PhysX3_x64.lib" )
#            pragma comment( lib, "PhysX3Extensions.lib" )
#            pragma comment( lib, "PhysX3Common_x64.lib" )
#            pragma comment( lib, "PhysX3Vehicle.lib" )
#            pragma comment( lib, "PhysX3Cooking_x64.lib" )
#            pragma comment( lib, "PhysXProfileSDK.lib" )
#            pragma comment( lib, "PxTask.lib" )
#        endif
#    else
#        if WP_ARCH_TYPE == WP_ARCHITECTURE_32
#            pragma comment( lib, "WPPhysx.lib" )
#            pragma comment( lib, "PhysX3_x86.lib" )
#            pragma comment( lib, "PhysX3Common_x86.lib" )
#            pragma comment( lib, "PhysX3Cooking_x86.lib" )
#            pragma comment( lib, "PhysX3Extensions.lib" )
#            pragma comment( lib, "PhysX3CharacterKinematic_x86.lib" )
#            pragma comment( lib, "PhysX3Vehicle.lib" )
#        else
#            pragma comment( lib, "WPPhysx.lib" )
#            pragma comment( lib, "PhysX3_x64.lib" )
#            pragma comment( lib, "PhysX3Extensions.lib" )
#            pragma comment( lib, "PhysX3Common_x64.lib" )
#            pragma comment( lib, "PhysX3Vehicle.lib" )
#            pragma comment( lib, "PhysX3Cooking_x64.lib" )
#            pragma comment( lib, "PhysXProfileSDK.lib" )
#            pragma comment( lib, "PxTask.lib" )
#        endif
#    endif
#endif

#endif // WPPhysxAutoLink_h__
