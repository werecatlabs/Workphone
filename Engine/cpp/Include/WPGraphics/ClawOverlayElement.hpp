#ifndef _WPGraphics_COverlayElement_H
#define _WPGraphics_COverlayElement_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IOverlayElement.hpp>
#include <Workphone/Interface/Graphics/IOverlay.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone
{
    namespace render
    {
        class ClawOverlay;

        /**
         * @class COverlayElement
         * @brief Base implementation of an overlay element.
         * @tparam TInterface The specific overlay element interface to implement.
         */
        template <typename TInterface = IOverlayElement>
        class ClawOverlayElement : public TInterface
        {
        public:
            ClawOverlayElement() = default;
            ~ClawOverlayElement() override = default;

            void load( SmartPtr<ISharedObject> data )
            {
                try
                {
                    setLoadingState( LoadingState::Loading );
                    m_children.reserve( 8 );
                    setLoadingState( LoadingState::Loaded );
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
            }

            void unload( SmartPtr<ISharedObject> data )
            {
                try
                {
                    const auto &loadingState = getLoadingState();
                    if( loadingState != LoadingState::Unloaded )
                    {
                        setLoadingState( LoadingState::Unloading );

                        for( auto child : m_children )
                        {
                            WP_ASSERT( child );
                            if( child )
                            {
                                child->unload( nullptr );
                            }
                        }

                        m_children.clear();

                        setLoadingState( LoadingState::Unloaded );
                    }
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
            }

            Vector2<real_Num> getPosition() const override
            {
                return m_position;
            }

            void setPosition( const Vector2<real_Num> &position ) override
            {
                m_position = position;
            }

            Vector2<real_Num> getSize() const override
            {
                return m_size;
            }

            void setSize( const Vector2<real_Num> &size ) override
            {
                m_size = size;
            }

            u32 getZOrder() const override
            {
                return m_zOrder;
            }

            void setZOrder( u32 zOrder ) override
            {
                m_zOrder = zOrder;
            }

            void setColour( const ColourF &colour ) override
            {
                m_colour = colour;
            }

            ColourF getColour() const override
            {
                return m_colour;
            }

            void setMetricsMode( u8 metricsMode ) override
            {
                m_metricsMode = metricsMode;
            }

            u8 getMetricsMode() const override
            {
                return m_metricsMode;
            }

            void setHorizontalAlignment( u8 gha ) override
            {
                m_horizontalAlignment = gha;
            }

            u8 getHorizontalAlignment() const override
            {
                return m_horizontalAlignment;
            }

            void setVerticalAlignment( u8 gva ) override
            {
                m_verticalAlignment = gva;
            }

            u8 getVerticalAlignment() const override
            {
                return m_verticalAlignment;
            }

            bool isContainer() const override
            {
                return true;
            }

            SmartPtr<IOverlay> getOverlay() const override
            {
                return m_overlay;
            }

            void setOverlay( SmartPtr<IOverlay> overlay ) override
            {
                m_overlay = overlay;
            }

            SmartPtr<IOverlayElement> getParent() const override
            {
                return m_parent;
            }

            void setParent( SmartPtr<IOverlayElement> element ) override
            {
                m_parent = element;
            }

            void addChild( SmartPtr<IOverlayElement> element ) override
            {
                WP_ASSERT( element );
                if( !element )
                {
                    return;
                }

                m_children.emplace_back( element );
                element->setParent( this );
            }

            void removeChild( SmartPtr<IOverlayElement> element ) override
            {
                WP_ASSERT( element );
                if( !element )
                {
                    return;
                }

                auto it = std::remove( m_children.begin(), m_children.end(), element );
                if( it != m_children.end() )
                {
                    element->setParent( nullptr );
                    m_children.erase( it, m_children.end() );
                }
            }

            Array<SmartPtr<IOverlayElement>> getChildren() const override
            {
                return m_children.snapshot();
            }

            void _getObject( void **ppObject ) const override
            {
                *ppObject = nullptr;
            }

            void setMaterial( SmartPtr<IMaterial> material ) override
            {
                m_material = material;
            }

            SmartPtr<IMaterial> getMaterial() const override
            {
                return m_material;
            }

            void setCaption( const String &text ) override
            {
                m_caption = text;
            }

            String getCaption() const override
            {
                return m_caption;
            }

            void setVisible( bool visible ) override
            {
                m_visible = visible;
            }

            bool isVisible() const override
            {
                return m_visible;
            }

        protected:
            Vector2<real_Num> m_position = Vector2<real_Num>( 0, 0 );
            Vector2<real_Num> m_size = Vector2<real_Num>( 0, 0 );
            u32 m_zOrder = 0;
            ColourF m_colour = ColourF::White;
            u8 m_metricsMode = GMM_RELATIVE;
            u8 m_horizontalAlignment = GHA_LEFT;
            u8 m_verticalAlignment = GVA_TOP;
            SmartPtr<IOverlay> m_overlay;
            SmartPtr<IOverlayElement> m_parent;
            ConcurrentArray<SmartPtr<IOverlayElement>> m_children;
            SmartPtr<IMaterial> m_material;
            String m_caption;
            bool m_visible = true;
        };
    }  // end namespace render
}  // namespace workphone

#endif
