#ifndef ResourceDirector_h__
#define ResourceDirector_h__

#include <Workphone/System/Director.hpp>

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
        };

    }  // namespace scene
}  // namespace workphone

#endif  // ResourceDirector_h__
