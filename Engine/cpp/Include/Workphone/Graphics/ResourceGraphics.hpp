#ifndef CResourceGraphics_h__
#define CResourceGraphics_h__

#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/System/Resource.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/Handle.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone
{
    namespace render
    {

        template <class T>
        class ResourceGraphics : public Resource<T>
        {
        public:
            ResourceGraphics()
            {
            }

            ResourceGraphics( u32 poolTypeId )
            {
            }

            ~ResourceGraphics() override
            {
            }

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data )
            {
                Resource<T>::unload( data );
            }

            void saveToFile( const String &filePath )
            {
            }

            void loadFromFile( const String &filePath )
            {
            }

            /** @copydoc IResource::getProperties */
            virtual SmartPtr<Properties> getProperties() const
            {
                auto properties = Resource<T>::getProperties();

                auto name = ResourceGraphics<T>::getName();
                properties->setProperty( "name", name );
                return properties;
            }

            /** @copydoc IResource::setProperties */
            virtual void setProperties( SmartPtr<Properties> properties )
            {
                auto name = String();
                properties->getProperty( "name", name );

                ResourceGraphics<T>::setName( name );
            }

            void _getObject( void **ppObject ) const
            {
                *ppObject = nullptr;
            }

            virtual void createStateObject()
            {
            }

            virtual void destroyStateObject()
            {
            }

            bool isThreadSafe() const
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();
                auto renderTask = graphicsSystem->getRenderTask();

                auto task = Thread::getCurrentTask();

                const auto &loadingState = T::getLoadingState();

                return loadingState == LoadingState::Loaded && task == renderTask;
            }

            void addMessage( SmartPtr<IStateMessage> message )
            {
                if( message )
                {
                    auto applicationManager = core::IApplicationManager::instancePtr();
                    auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

                    message->setSender( this );

                    if( auto stateContext = Resource<T>::getStateContext() )
                    {
                        const auto stateTask = graphicsSystem->getStateTask();
                        stateContext->addMessage( stateTask, message );
                    }
                }
            }

            WP_CLASS_REGISTER_TEMPLATE_DECL( ResourceGraphics, T );
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::render, ResourceGraphics, T, Resource<T> );

    }  // end namespace render
}  // namespace workphone

#endif  // CResourceOgre_h__
