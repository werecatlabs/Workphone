#ifndef _FBPhysicsAutoLink_H
#define _FBPhysicsAutoLink_H

#if WP_USE_AUTO_LINK
#    ifdef _DEBUG
#        pragma comment( lib, "WPPhysics.lib" )
#    elif NDEBUG
#        pragma comment( lib, "WPPhysics.lib" )
#    else
#        pragma comment( lib, "WPPhysics.lib" )
#    endif
#endif

#endif
