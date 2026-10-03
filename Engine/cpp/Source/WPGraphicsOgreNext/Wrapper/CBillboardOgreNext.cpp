#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include "WPGraphicsOgreNext/Wrapper/CBillboardOgreNext.hpp"
#include <OgreBillboard.h>
#include "OgreUtil.hpp"

namespace workphone::render
{

    CBillboardOgreNext::CBillboardOgreNext() : m_bb( nullptr )
    {
    }

    CBillboardOgreNext::~CBillboardOgreNext() = default;

    void CBillboardOgreNext::initialise( Ogre::v1::Billboard *bb )
    {
        m_bb = bb;
    }

    void CBillboardOgreNext::setPosition( const Vector3F &position )
    {
        m_position = position;
        if( m_bb )
        {
            m_bb->setPosition( OgreUtil::convertToOgre( position ) );
        }
    }

    auto CBillboardOgreNext::getPosition() const -> Vector3F
    {
        if( m_bb )
        {
            return OgreUtil::convert( m_bb->getPosition() );
        }
        return m_position;
    }

    void CBillboardOgreNext::setDimensions( const Vector2F &dimensions )
    {
        m_scale = Vector3F( dimensions.X(), dimensions.Y(), 1.0f );
        if( m_bb )
        {
            m_bb->setDimensions( dimensions.X(), dimensions.Y() );
        }
    }

    auto CBillboardOgreNext::getDimensions() const -> Vector2F
    {
        if( m_bb && m_bb->hasOwnDimensions() )
        {
            return Vector2F( m_bb->getOwnWidth(), m_bb->getOwnHeight() );
        }
        return Vector2F::zero();
    }

    void CBillboardOgreNext::setColour( const ColourF &colour )
    {
        if( m_bb )
        {
            m_bb->setColour( OgreUtil::convertToOgre( colour ) );
        }
    }

    auto CBillboardOgreNext::getColour() const -> ColourF
    {
        if( m_bb )
        {
            return OgreUtil::convert( m_bb->getColour() );
        }
        return ColourF::Red;
    }

    void CBillboardOgreNext::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = m_bb;
        }
    }

    auto CBillboardOgreNext::IsFree( void *element ) -> bool
    {
        // CBillboardPtr billboard = *(static_cast<CBillboardPtr*>(element));
        // if( billboard->getReferenceCount() == 2 )
        //	return true;

        return false;
    }

    void CBillboardOgreNext::setOrientation( const QuaternionF &orientation )
    {
        m_orientation = orientation;
    }

    auto CBillboardOgreNext::getOrientation() const -> QuaternionF
    {
        return m_orientation;
    }

    void CBillboardOgreNext::setScale( const Vector3F &dimensions )
    {
        m_scale = dimensions;
        if( m_bb )
        {
            m_bb->setDimensions( dimensions.X(), dimensions.Y() );
        }
    }

    auto CBillboardOgreNext::getScale() const -> Vector3F
    {
        return m_scale;
    }

    auto CBillboardOgreNext::_getRenderSystemTransform() const -> void *
    {
        return nullptr;
    }

    auto CBillboardOgreNext::getRenderData() const -> void *
    {
        return m_bb;
    }

    void CBillboardOgreNext::setRenderData( void *renderData )
    {
        m_bb = static_cast<Ogre::v1::Billboard *>( renderData );
    }

}  // namespace workphone::render
