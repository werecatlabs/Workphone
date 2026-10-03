#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Graphics/Billboard.hpp"

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, Billboard, IBillboard );

    Billboard::Billboard() = default;
    Billboard::~Billboard() = default;

    void Billboard::setPosition( const Vector3<real_Num> &position )
    {
        m_position = position;
    }

    Vector3<real_Num> Billboard::getPosition() const
    {
        return m_position;
    }

    void Billboard::setOrientation( const Quaternion<real_Num> &orientation )
    {
        m_orientation = orientation;
    }

    Quaternion<real_Num> Billboard::getOrientation() const
    {
        return m_orientation;
    }

    void Billboard::setScale( const Vector3<real_Num> &dimensions )
    {
        m_scale = dimensions;
    }

    Vector3<real_Num> Billboard::getScale() const
    {
        return m_scale;
    }

    void *Billboard::_getRenderSystemTransform() const
    {
        return m_renderData;
    }

    void Billboard::setColour( const ColourF &colour )
    {
        m_colour = colour;
    }

    ColourF Billboard::getColour() const
    {
        return m_colour;
    }

    void *Billboard::getRenderData() const
    {
        return m_renderData;
    }

    void Billboard::setRenderData( void *renderData )
    {
        m_renderData = renderData;
    }

    void Billboard::_getObject( void **ppObject ) const
    {
        if( ppObject )
            *ppObject = m_renderData;
    }
}  // namespace workphone::render
