#ifndef __WP_LogManager_h__
#define __WP_LogManager_h__

#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/ILogManager.hpp>

#if WP_ERROR_LOG
#    define WP_LOG( x )                                                                  \
        if( auto applicationManager = workphone::core::IApplicationManager::instance() ) \
        {                                                                                \
            if( auto logManager = applicationManager->getLogManager() )                  \
            {                                                                            \
                logManager->logMessage( ( x ), workphone::ILogManager::Type::Info );     \
            }                                                                            \
        }
#    define WP_LOG_INFO( x )                                                             \
        if( auto applicationManager = workphone::core::IApplicationManager::instance() ) \
        {                                                                                \
            if( auto logManager = applicationManager->getLogManager() )                  \
            {                                                                            \
                logManager->logMessage( ( x ), workphone::ILogManager::Type::Info );     \
            }                                                                            \
        }
#    define WP_LOG_WARNING( x )                                                          \
        if( auto applicationManager = workphone::core::IApplicationManager::instance() ) \
        {                                                                                \
            if( auto logManager = applicationManager->getLogManager() )                  \
            {                                                                            \
                logManager->logMessage( ( x ), workphone::ILogManager::Type::Warning );  \
            }                                                                            \
        }
#    define WP_LOG_ERROR( x )                                                            \
        if( auto applicationManager = workphone::core::IApplicationManager::instance() ) \
        {                                                                                \
            if( auto logManager = applicationManager->getLogManager() )                  \
            {                                                                            \
                logManager->logMessage( ( x ), workphone::ILogManager::Type::Error );    \
            }                                                                            \
        }
#    define WP_LOG_EXCEPTION( ex )                                                       \
        if( auto applicationManager = workphone::core::IApplicationManager::instance() ) \
        {                                                                                \
            if( auto logManager = applicationManager->getLogManager() )                  \
            {                                                                            \
                const auto description = String( ex.what() );                            \
                logManager->logMessage( description, ILogManager::Type::Exception );     \
            }                                                                            \
        }
#else
#    define WP_LOG( x )
#    define WP_LOG_INFO( x )
#    define WP_LOG_WARNING( x )
#    define WP_LOG_ERROR( x )
#    define WP_LOG_EXCEPTION( ex )
#endif

#endif  // LogManager_h__
