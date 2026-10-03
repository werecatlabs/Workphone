#ifndef WPFMODStudioAutolink_h__
#define WPFMODStudioAutolink_h__

#if WP_USE_AUTO_LINK
#    ifdef _DEBUG
#        pragma comment( lib, "fmodstudio_vc.lib" )
#        pragma comment( lib, "fmod_vc.lib" )
#        pragma comment( lib, "WPFMODStudio_d.lib" )
#    elif NDEBUG
#        pragma comment( lib, "fmodstudio_vc.lib" )
#        pragma comment( lib, "fmod_vc.lib" )
#        pragma comment( lib, "WPFMODStudio.lib" )
#    else
#        pragma comment( lib, "fmodstudio_vc.lib" )
#        pragma comment( lib, "fmod_vc.lib" )
#        pragma comment( lib, "WPFMODStudio.lib" )
#    endif
#endif

#endif  // WPFMODStudioAutolink_h__
