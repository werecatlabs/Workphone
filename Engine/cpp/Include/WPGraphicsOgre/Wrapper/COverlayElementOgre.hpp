#ifndef _COverlayElementOgre_H
#define _COverlayElementOgre_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/IOverlayElement.hpp>
#include <Workphone/Graphics/OverlayElement.hpp>

namespace workphone
{
    namespace render
    {

        template <class T>
        class COverlayElementOgre : public OverlayElement<T>
        {
        public:
            COverlayElementOgre();
            ~COverlayElementOgre() override;

            virtual void _getObject( void **ppObject ) const override
            {
                *ppObject = m_element;
            }

            /** @copydoc IObject::isValid */
            bool isValid() const override
            {
                if( T::isLoaded() )
                {
                    if( auto parent = OverlayElement<T>::getParent() )
                    {
                        auto children = OverlayElement<T>::getChildren();
                        for( auto child : children )
                        {
                            if( !child->isValid() )
                            {
                                return false;
                            }
                        }

                        return true;
                    }

                    return true;
                }

                return false;
            }

            Ogre::OverlayElement *getElement() const
            {
                return m_element;
            }

            void setElement( Ogre::OverlayElement *element )
            {
                m_element = element;
            }

            virtual void setupMaterial( SmartPtr<IMaterial> material )
            {
            }

            WP_CLASS_REGISTER_TEMPLATE_DECL( COverlayElementOgre, T );

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

            Ogre::OverlayElement *m_element = nullptr;
        };

        template <class T>
        COverlayElementOgre<T>::COverlayElementOgre() : OverlayElement<T>()
        {
        }

        template <class T>
        COverlayElementOgre<T>::~COverlayElementOgre()
        {
        }

    }  // end namespace render
}  // namespace workphone

#endif
