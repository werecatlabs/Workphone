#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/Overlay.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Graphics/IOverlayElement.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone
{
    namespace render
    {

        WP_CLASS_REGISTER_DERIVED( workphone::render, Overlay, SharedGraphicsObject<IOverlay> );

        Overlay::Overlay() = default;

        Overlay::~Overlay() = default;

        void Overlay::addElement( SmartPtr<IOverlayElement> element )
        {
            if( element )
            {
                m_elements.push_back( element );
            }
        }

        bool Overlay::removeElement( SmartPtr<IOverlayElement> element )
        {
            if( !element )
            {
                return false;
            }

            auto it = std::find( m_elements.begin(), m_elements.end(), element );
            if( it != m_elements.end() )
            {
                m_elements.erase( it );
                return true;
            }

            return false;
        }

        Array<SmartPtr<IOverlayElement>> Overlay::getElements() const
        {
            return m_elements;
        }

        void Overlay::addSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode )
        {
            // Scene nodes are rendered as part of the overlay but not stored
            // as overlay elements. This is a no-op in the base implementation.
            // Derived classes or wrappers (e.g., COverlayOgre) handle scene node rendering.
        }

        bool Overlay::removeSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode )
        {
            // Scene nodes are not stored in this implementation.
            // Derived classes or wrappers (e.g., COverlayOgre) handle scene node rendering.
            return false;
        }

        void Overlay::setVisible( bool visible )
        {
            for( auto &element : m_elements )
            {
                if( element )
                {
                    element->setVisible( visible );
                }
            }
        }

        bool Overlay::isVisible() const
        {
            // An overlay is considered visible if it has at least one visible element
            // or if it has no elements (default visible state)
            if( m_elements.empty() )
            {
                return true;
            }

            for( const auto &element : m_elements )
            {
                if( element && element->isVisible() )
                {
                    return true;
                }
            }

            return false;
        }

        void Overlay::setZOrder( u32 zorder )
        {
            for( auto &element : m_elements )
            {
                if( element )
                {
                    element->setZOrder( zorder );
                }
            }
        }

        u32 Overlay::getZOrder() const
        {
            // Return the z-order of the first element, or 0 if no elements exist
            if( !m_elements.empty() && m_elements[0] )
            {
                return m_elements[0]->getZOrder();
            }

            return 0;
        }

        void Overlay::updateZOrder()
        {
            // Update z-order for all elements based on their position in the array
            for( size_t i = 0; i < m_elements.size(); ++i )
            {
                if( m_elements[i] )
                {
                    m_elements[i]->setZOrder( static_cast<u32>( i ) );
                }
            }
        }

        Vector2I Overlay::getAbsoluteResolution() const
        {
            // Return the resolution of the first element, or zero vector if no elements exist
            if( !m_elements.empty() && m_elements[0] )
            {
                //return m_elements[0]->getAbsoluteResolution();
            }

            return Vector2I( 0, 0 );
        }

        void Overlay::setAbsoluteResolution( const Vector2I &absoluteResolution )
        {
            for( auto &element : m_elements )
            {
                if( element )
                {
                    //element->setAbsoluteResolution( absoluteResolution );
                }
            }
        }

        void Overlay::_getObject( void **ppObject ) const
        {
            if( ppObject )
            {
                *ppObject = nullptr;
            }
        }

    }  // end namespace render
}  // namespace workphone
