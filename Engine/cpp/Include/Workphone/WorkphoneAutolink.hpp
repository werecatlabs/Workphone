#ifndef __WPCoreAutolink_H
#define __WPCoreAutolink_H

#include <Workphone/WorkphoneConfig.hpp>

#if WP_USE_AUTO_LINK
#    if defined _WP_STATIC_LIB_
#        ifdef _DEBUG
#            ifdef WP_EXPORTS
#            else
#                pragma comment( lib, "Workphone.lib" )
#            endif  // WP_EXPORT
#        elif NDEBUG
#            ifdef WP_EXPORTS
#            else
#                pragma comment( lib, "Workphone.lib" )
#            endif  // WP_EXPORT
#        else
#            ifdef WP_EXPORTS
#            else
#                pragma comment( lib, "Workphone.lib" )
#            endif  // WP_EXPORT
#        endif
#    endif
#endif

#endif
