#ifndef _COverlayElementContainer_H
#define _COverlayElementContainer_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IOverlayElementContainer.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayElementOgreNext.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/State/States/State.hpp>

namespace workphone
{
    namespace render
    {

        class COverlayElementContainer : public COverlayElementOgreNext<IOverlayElementContainer>
        {
        public:
            COverlayElementContainer();
            ~COverlayElementContainer() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            bool isContainer() const override;

            // IOverlayElementContainer functions
            void addChild( SmartPtr<IOverlayElement> element ) override;
            void removeChild( SmartPtr<IOverlayElement> element ) override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @copydoc IObject::isValid */
            bool isValid() const override;

            Ogre::v1::OverlayContainer *getContainerElement() const;
            void setContainerElement( Ogre::v1::OverlayContainer *container );

            WP_CLASS_REGISTER_DECL;

        protected:
            class StateListener : public StateListenerOgre
            {
            public:
                StateListener() = default;
                ~StateListener() override = default;
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;
            };

            class MaterialStateListener : public IStateListener
            {
            public:
                MaterialStateListener() = default;
                MaterialStateListener( COverlayElementContainer *owner );

                ~MaterialStateListener() override;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                SmartPtr<COverlayElementContainer> getOwner() const;
                void setOwner( SmartPtr<COverlayElementContainer> owner );

            protected:
                AtomicSmartPtr<COverlayElementContainer> m_owner;
            };

            void setupMaterial( SmartPtr<IMaterial> material ) override;
            void createStateContext() override;

            void materialLoaded( SmartPtr<IMaterial> material );

            Ogre::v1::OverlayContainer *m_container = nullptr;

            SmartPtr<IMaterial> m_material;

            SmartPtr<IStateListener> m_materialStateListener;
        };
    }  // end namespace render
}  // namespace workphone

#endif
