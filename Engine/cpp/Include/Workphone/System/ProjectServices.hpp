#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <memory>

namespace workphone
{
    /** Providers own transport/authentication; local implementations support isolated development. */
    class WPCore_API IProjectCloudSaveProvider
    {
    public:
        virtual ~IProjectCloudSaveProvider() = default;
        virtual bool put( const String &slot, const Array<u8> &bytes, String &error ) = 0;
        virtual bool get( const String &slot, Array<u8> &bytes, String &error ) = 0;
        virtual bool remove( const String &slot, String &error ) = 0;
        virtual bool list( Array<String> &slots, String &error ) = 0;
    };

    class WPCore_API IProjectAnalyticsProvider
    {
    public:
        virtual ~IProjectAnalyticsProvider() = default;
        virtual bool record( const String &event, SmartPtr<Properties> fields, String &error ) = 0;
    };

    class WPCore_API IProjectCrashProvider
    {
    public:
        virtual ~IProjectCrashProvider() = default;
        virtual bool setEnabled( bool enabled, String &error ) = 0;
        virtual bool capture( const String &reason, String &path, String &error ) = 0;
    };

    struct ProjectServiceProviders
    {
        std::shared_ptr<IProjectCloudSaveProvider> cloud;
        std::shared_ptr<IProjectAnalyticsProvider> analytics;
        std::shared_ptr<IProjectCrashProvider> crash;
    };

    WPCore_API ProjectServiceProviders makeLocalProjectServiceProviders( const String &directory );
    /** Installs plugin overrides for subsequent Apply/startup; empty fields use local providers. */
    WPCore_API void setProjectServiceProviders( ProjectServiceProviders providers );
    /** Atomically publishes enable gates and providers. Remote providers may be injected by plugins. */
    WPCore_API bool configureProjectServices( ProjectServiceProviders providers, bool cloud,
                                             bool analytics, bool crash, String &error );
    /** Apply local development services and selected network provider after factories are installed. */
    WPCore_API bool applyProjectServiceSettings( SmartPtr<Properties> settings,
                                                const String &projectDirectory, String &error );
    WPCore_API void releaseProjectServiceSettings();
    WPCore_API bool putProjectCloudSave( const String &slot, const Array<u8> &bytes, String &error );
    WPCore_API bool getProjectCloudSave( const String &slot, Array<u8> &bytes, String &error );
    WPCore_API bool removeProjectCloudSave( const String &slot, String &error );
    WPCore_API bool listProjectCloudSaves( Array<String> &slots, String &error );
    WPCore_API bool recordProjectAnalytics( const String &event, SmartPtr<Properties> fields,
                                            String &error );
    WPCore_API bool captureProjectCrashReport( const String &reason, String &path, String &error );
}
