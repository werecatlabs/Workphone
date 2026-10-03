#ifndef _COverlayElementOgreNext_H
#define _COverlayElementOgreNext_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/OverlayElement.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>

namespace workphone
{
    namespace render
    {

        template <class T>
        class COverlayElementOgreNext : public OverlayElement<T>
        {
        public:
            COverlayElementOgreNext();
            ~COverlayElementOgreNext() override;

            virtual void _getObject( void **ppObject ) const override;

            /** @copydoc IObject::isValid */
            bool isValid() const override;

            Ogre::v1::OverlayElement *getElement() const;

            void setElement( Ogre::v1::OverlayElement *element );

            virtual void setupMaterial( SmartPtr<IMaterial> material );

            SmartPtr<Properties> getProperties() const;

            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_TEMPLATE_DECL( COverlayElementOgreNext, T );

        protected:
            class StateListenerOgre : public OverlayElement<T>::ElementStateListener
            {
            public:
                StateListenerOgre() = default;
                ~StateListenerOgre() override = default;

                virtual bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                virtual bool handleStateChanged( SmartPtr<IState> &state ) override;
            };

            virtual void createStateContext();

            AtomicValue<Ogre::v1::OverlayElement *> m_element = nullptr;
        };

    }  // end namespace render
}  // namespace workphone

#endif
