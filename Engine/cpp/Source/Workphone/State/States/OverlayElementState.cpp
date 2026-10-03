#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/OverlayElementState.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, OverlayElementState, StateData );

    OverlayElementState::OverlayElementState() = default;
    OverlayElementState::~OverlayElementState() = default;

    auto OverlayElementState::getPosition() const -> Vector2<real_Num>
    {
        return m_position;
    }

    void OverlayElementState::setPosition( const Vector2<real_Num> &position )
    {
        if( m_position != position )
        {
            m_position = position;
        }
    }

    auto OverlayElementState::getSize() const -> Vector2<real_Num>
    {
        return m_size;
    }

    void OverlayElementState::setSize( const Vector2<real_Num> &size )
    {
        if( m_size != size )
        {
            m_size = size;
        }
    }

    auto OverlayElementState::getName() const -> String
    {
        return m_name;
    }

    void OverlayElementState::setName( const String &name )
    {
        m_name = name;
    }

    void OverlayElementState::setMetricsMode( [[maybe_unused]] u8 metricsMode )
    {
        if( m_metricsMode != metricsMode )
        {
            m_metricsMode = metricsMode;
        }
    }

    auto OverlayElementState::getMetricsMode() const -> u8
    {
        return m_metricsMode;
    }

    void OverlayElementState::setHorizontalAlignment( [[maybe_unused]] u8 gha )
    {
        if( m_gha != gha )
        {
            m_gha = gha;
        }
    }

    auto OverlayElementState::getHorizontalAlignment() const -> u8
    {
        return m_gha;
    }

    void OverlayElementState::setVerticalAlignment( [[maybe_unused]] u8 gva )
    {
        if( m_gva != gva )
        {
            m_gva = gva;
        }
    }

    auto OverlayElementState::getVerticalAlignment() const -> u8
    {
        return m_gva;
    }

    auto OverlayElementState::getMaterial() const -> SmartPtr<render::IMaterial>
    {
        return m_material;
    }

    void OverlayElementState::setMaterial( SmartPtr<render::IMaterial> material )
    {
        if( getMaterial() != material )
        {
            m_material = material;
        }
    }

    void OverlayElementState::setCaption( [[maybe_unused]] const String &text )
    {
        m_caption = text;
    }

    auto OverlayElementState::getCaption() const -> String
    {
        return m_caption;
    }

    void OverlayElementState::setVisible( [[maybe_unused]] bool visible )
    {
        if( m_visible != visible )
        {
            m_visible = visible;
        }
    }

    auto OverlayElementState::isVisible() const -> bool
    {
        return m_visible;
    }

    auto OverlayElementState::getZOrder() const -> u32
    {
        return m_zOrder;
    }

    void OverlayElementState::setZOrder( u32 zOrder )
    {
        if( m_zOrder != zOrder )
        {
            m_zOrder = zOrder;
        }
    }

    void OverlayElementState::setColour( const ColourF &colour )
    {
        if( m_colour != colour )
        {
            m_colour = colour;
        }
    }

    auto OverlayElementState::getColour() const -> ColourF
    {
        return m_colour;
    }
}  // namespace workphone
