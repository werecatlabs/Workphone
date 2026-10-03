#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/Debug.hpp>
#include <Workphone/Interface/Graphics/IDebugLine.hpp>
#include <Workphone/Interface/Graphics/IDebugCircle.hpp>
#include <Workphone/Interface/Graphics/IOverlay.hpp>
#include <Workphone/Interface/Graphics/IOverlayElement.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{
    namespace render
    {

        Debug::StateListener::StateListener()
        {
        }

        Debug::StateListener::~StateListener()
        {
        }

        bool Debug::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            if( auto owner = m_owner.lock() )
            {
                //return owner->handleStateMessage( message );
            }

            return false;
        }

        bool Debug::StateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            if( auto owner = m_owner.lock() )
            {
                //return owner->handleStateChanged( state );
            }

            return false;
        }

        SmartPtr<Debug> Debug::StateListener::getOwner() const
        {
            return m_owner.lock();
        }

        void Debug::StateListener::setOwner( SmartPtr<Debug> owner )
        {
            m_owner = owner;
        }

        Debug::Debug()
        {
        }

        Debug::~Debug()
        {
        }

        void Debug::load( SmartPtr<ISharedObject> data )
        {
            WP_LOG( "Loading Debug" );

            // Create line material
            createLineMaterial();

            // Call parent load
            SharedGraphicsObject<IDebug>::load( data );
        }

        void Debug::unload( SmartPtr<ISharedObject> data )
        {
            WP_LOG( "Unloading Debug" );

            // Clear all debug objects
            clear();

            // Unload overlay if exists
            if( m_overlay )
            {
                m_overlay->unload( nullptr );
                m_overlay = nullptr;
            }

            // Call parent unload
            SharedGraphicsObject<IDebug>::unload( data );
        }

        void Debug::preUpdate()
        {
        }

        void Debug::update()
        {
            // Process removal queue
            SmartPtr<IDebugLine> line;
            while( m_removeQueue.try_pop( line ) )
            {
                if( line )
                {
                    line->unload( nullptr );
                }
            }
        }

        void Debug::postUpdate()
        {
        }

        void Debug::clear()
        {
            // Clear debug lines
            for( size_t i = 0; i < m_debugLines.size(); ++i )
            {
                auto line = m_debugLines.at( i );
                if( line )
                {
                    line->unload( nullptr );
                }
            }

            m_debugLines.clear();

            // Clear debug circles
            for( size_t i = 0; i < m_debugCircles.size(); ++i )
            {
                auto circle = m_debugCircles.at( i );
                if( circle )
                {
                    circle->unload( nullptr );
                }
            }

            m_debugCircles.clear();

            // Clear overlay elements
            for( size_t i = 0; i < m_overlayElements.size(); ++i )
            {
                auto element = m_overlayElements.at( i );
                if( element )
                {
                    element->unload( nullptr );
                }
            }

            m_overlayElements.clear();
        }

        void Debug::drawPoint( hash_type id, const Vector3<real_Num> &position, u32 color )
        {
            // For now, a point can be represented as a very small line or a sphere
            // This is a placeholder implementation
            WP_LOG( "Drawing point at position" );
        }

        SmartPtr<IDebugLine> Debug::drawLine( hash_type id, const Vector3<real_Num> &start,
                                              const Vector3<real_Num> &end, u32 colour )
        {
            auto line = addLine( id );
            if( line )
            {
                line->setPosition( start );
                line->setVector( end - start );
                line->setColour( colour );
                line->setVisible( true );
                line->setDirty( true );
            }
            return line;
        }

        SmartPtr<IDebugCircle> Debug::drawCircle( hash_type id, const Vector3<real_Num> &position,
                                                  const Quaternion<real_Num> &orientation,
                                                  real_Num radius, u32 color )
        {
            auto circle = addCircle( id );
            if( circle )
            {
                // Implementation depends on IDebugCircle interface
                // For now, just mark as created
            }
            return circle;
        }

        void Debug::drawText( hash_type id, const Vector2<real_Num> &position, const String &text,
                              u32 color )
        {
            auto element = getElementById( id );
            if( !element )
            {
                // Create new overlay element if factory is available
                // This would need access to overlay element factory
            }

            if( element )
            {
                // Set position, text, and color
                // Implementation depends on IOverlayElement interface
            }
        }

        void Debug::createLineMaterial()
        {
            // Material creation would be handled by the graphics system
            // This is a placeholder for material setup
        }

        SmartPtr<IDebugLine> Debug::addLine( hash_type id )
        {
            // Check if line with id already exists
            auto existingLine = getLine( id );
            if( existingLine )
            {
                return existingLine;
            }

            // Create new line (would require factory)
            // For now, return nullptr as placeholder
            SmartPtr<IDebugLine> line = nullptr;

            if( line )
            {
                m_debugLines.push_back( line );
            }

            return line;
        }

        void Debug::removeLine( hash_type id )
        {
            auto line = getLine( id );
            if( line )
            {
                removeLine( line );
            }
        }

        void Debug::removeLine( SmartPtr<IDebugLine> debugLine )
        {
            if( debugLine )
            {
                auto it = std::find( m_debugLines.begin(), m_debugLines.end(), debugLine );
                if( it != m_debugLines.end() )
                {
                    m_debugLines.erase( it );
                    m_removeQueue.push( debugLine );
                }
            }
        }

        SmartPtr<IDebugLine> Debug::getLine( hash_type id ) const
        {
            // Implementation would search by id
            // Would need to store id mapping
            return nullptr;
        }

        SmartPtr<IDebugCircle> Debug::addCircle( hash_type id )
        {
            // Check if circle with id already exists
            auto existingCircle = getCircle( id );
            if( existingCircle )
            {
                return existingCircle;
            }

            // Create new circle (would require factory)
            // For now, return nullptr as placeholder
            SmartPtr<IDebugCircle> circle = nullptr;

            if( circle )
            {
                m_debugCircles.push_back( circle );
            }

            return circle;
        }

        void Debug::removeCircle( hash_type id )
        {
            auto circle = getCircle( id );
            if( circle )
            {
                removeCircle( circle );
            }
        }

        void Debug::removeCircle( SmartPtr<IDebugCircle> debugCircle )
        {
            if( debugCircle )
            {
                auto it = std::find( m_debugCircles.begin(), m_debugCircles.end(), debugCircle );
                if( it != m_debugCircles.end() )
                {
                    m_debugCircles.erase( it );
                }
            }
        }

        SmartPtr<IDebugCircle> Debug::getCircle( hash_type id ) const
        {
            // Implementation would search by id
            // Would need to store id mapping
            return nullptr;
        }

        SmartPtr<IOverlay> Debug::getOverlay() const
        {
            return m_overlay;
        }

        void Debug::setOverlay( SmartPtr<IOverlay> overlay )
        {
            m_overlay = overlay;
        }

        void Debug::addOverlayElement( SmartPtr<IOverlayElement> element )
        {
            if( element )
            {
                m_overlayElements.push_back( element );
            }
        }

        SmartPtr<IOverlayElement> Debug::getElementById( hash_type id ) const
        {
            for( size_t i = 0; i < m_overlayElements.size(); ++i )
            {
                auto element = m_overlayElements.at( i );
                if( element )
                {
                    // Check if this element's id matches
                    // Implementation depends on how ids are stored
                }
            }

            return nullptr;
        }

    }  // end namespace render
}  // namespace workphone
