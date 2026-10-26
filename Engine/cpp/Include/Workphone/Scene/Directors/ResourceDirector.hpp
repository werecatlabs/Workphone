#ifndef ResourceDirector_h__
#define ResourceDirector_h__

#include <Workphone/System/Director.hpp>
#include <Workphone/Thread/SpinRWMutex.hpp>
#include <Workphone/System/SourceBudgetContainers.hpp>

namespace workphone
{
    namespace scene
    {

        /** Resource director implementation. */
        class WPCore_API ResourceDirector : public Director
        {
        public:
            static const String resourcePathStr;
            static const String resourceUUIDStr;
            static const String saveStr;
            static const String importStr;
            static const String importStatusStr;

            /** Constructor. */
            ResourceDirector();

            /** Destructor. */
            ~ResourceDirector() override;

            /** @copydoc IBuildDirector::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IBuildDirector::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc IBuildDirector::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IBuildDirector::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @copydoc IBuildDirector::import */
            void import() override;

            /** Runtime-only import progress shown by the existing property grid. */
            String getImportStatus() const;

            /** Start a request and prevent an older queued import from replacing its status. */
            u64 beginImport();
            void setImportStatus( u64 request, const String &status );

            /** Import progress is not part of the saved import options. */
            SmartPtr<ISharedObject> toData() const override;

            /** Get the resource path. */
            String getResourcePath() const;

            /** Set the resource path. */
            void setResourcePath( const String &resourcePath );

            /** Get the resource UUID. */
            String getResourceUUID() const;

            /** Set the resource UUID. */
            void setResourceUUID( const String &resourceUUID );

            WP_CLASS_REGISTER_DECL;

        protected:
            // The resource path.
            String m_resourcePath;

            // The resource UUID.
            String m_resourceUUID;

            mutable SpinRWMutex m_importStatusMutex;
            SourceBudgetString m_importStatus;
            u64 m_importRequest = 0;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // ResourceDirector_h__
