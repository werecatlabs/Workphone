#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIVector.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::ui
{
    ClawUIVector::ClawUIVector()
    {
        setType( "Vector" );
    }

    ClawUIVector::~ClawUIVector() = default;

    String ClawUIVector::getFileName() const
    {
        return m_fileName;
    }

    void ClawUIVector::setFileName( const String &fileName )
    {
        m_fileName = fileName;
    }

    SmartPtr<render::IMaterial> ClawUIVector::getMaterial() const
    {
        return m_material;
    }

    void ClawUIVector::setMaterial( SmartPtr<render::IMaterial> material )
    {
        m_material = material;
    }

    Vector3<real_Num> ClawUIVector::getValue() const
    {
        return m_value;
    }

    void ClawUIVector::setValue( const Vector3<real_Num> &value )
    {
        m_value = value;
    }

    void ClawUIVector::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        const auto bounds = getWorkphoneBounds();
        const auto componentWidth = bounds.w / 3.0f;
        auto x = static_cast<wp_f64>( m_value.X() );
        auto y = static_cast<wp_f64>( m_value.Y() );
        auto z = static_cast<wp_f64>( m_value.Z() );
        auto propertyPrefix = getLabel();
        if( propertyPrefix.empty() )
        {
            propertyPrefix = getName();
        }
        const auto xLabel = propertyPrefix + ".X";
        const auto yLabel = propertyPrefix + ".Y";
        const auto zLabel = propertyPrefix + ".Z";

        wp_property_wp_f64( ctx, reinterpret_cast<const wp_c8 *>( xLabel.c_str() ), -1000000.0, &x,
                            1000000.0, 0.1, 0.01f );
        wp_layout_space_push( ctx, { bounds.x + componentWidth, bounds.y, componentWidth, bounds.h } );
        wp_property_wp_f64( ctx, reinterpret_cast<const wp_c8 *>( yLabel.c_str() ), -1000000.0, &y,
                            1000000.0, 0.1, 0.01f );
        wp_layout_space_push( ctx,
                              { bounds.x + componentWidth * 2.0f, bounds.y, componentWidth, bounds.h } );
        wp_property_wp_f64( ctx, reinterpret_cast<const wp_c8 *>( zLabel.c_str() ), -1000000.0, &z,
                            1000000.0, 0.1, 0.01f );

        m_value = Vector3<real_Num>( static_cast<real_Num>( x ), static_cast<real_Num>( y ),
                                     static_cast<real_Num>( z ) );
        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
