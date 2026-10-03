#ifndef WPPhysxPCH_h__
#define WPPhysxPCH_h__

#if WP_USE_PRECOMPILED_HEADERS
#    include <Workphone/WorkphoneHeaders.hpp>
// #    include "PxPhysicsAPI.h"
// #    include "extensions/PxExtensionsAPI.h"
#endif

#ifdef free
#    undef free
#endif

#ifdef malloc
#    undef malloc
#endif

#ifdef realloc
#    undef realloc
#endif

#endif // WPPhysxPCH_h__
