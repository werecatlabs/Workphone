#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawLight.hpp>
#include <WPGraphics/ClawUtil.hpp>
#include <Workphone/Workphone.hpp>
#include "workphone_graphics_light.h"

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawLight, GraphicsLight );

    ClawLight::ClawLight() : m_light( wp_light_create() )
    {
    }

    ClawLight::~ClawLight()
    {
        wp_light_destroy( m_light );
        m_light = nullptr;
    }

    void ClawLight::setType( LightTypes type )
    {
        if( m_light )
        {
            wp_light_set_type( m_light, static_cast<wp_light_type>( type ) );
        }

        GraphicsLight::setType( type );
    }

    LightTypes ClawLight::getType() const
    {
        return m_light ? static_cast<LightTypes>( wp_light_get_type( m_light ) ) : GraphicsLight::getType();
    }

    void ClawLight::setDiffuseColour( const ColourF &colour )
    {
        if( m_light )
        {
            wp_light_set_diffuse_colour( m_light, ClawUtil::toCColour( colour ) );
        }
        GraphicsLight::setDiffuseColour( colour );
    }

    ColourF ClawLight::getDiffuseColour() const
    {
        return m_light ? ClawUtil::fromCColour( wp_light_get_diffuse_colour( m_light ) )
                       : GraphicsLight::getDiffuseColour();
    }

    void ClawLight::setSpecularColour( const ColourF &colour )
    {
        if( m_light )
        {
            wp_light_set_specular_colour( m_light, ClawUtil::toCColour( colour ) );
        }
        GraphicsLight::setSpecularColour( colour );
    }

    ColourF ClawLight::getSpecularColour() const
    {
        return m_light ? ClawUtil::fromCColour( wp_light_get_specular_colour( m_light ) )
                       : GraphicsLight::getSpecularColour();
    }

    void ClawLight::setAttenuation( f32 range, f32 constant, f32 linear, f32 quadratic )
    {
        if( m_light )
        {
            wp_light_set_attenuation( m_light, range, constant, linear, quadratic );
        }
        GraphicsLight::setAttenuation( range, constant, linear, quadratic );
    }

    f32 ClawLight::getAttenuationRange() const
    {
        return m_light ? wp_light_get_attenuation_range( m_light ) : GraphicsLight::getAttenuationRange();
    }

    f32 ClawLight::getAttenuationConstant() const
    {
        return m_light ? wp_light_get_attenuation_constant( m_light ) : GraphicsLight::getAttenuationConstant();
    }

    f32 ClawLight::getAttenuationLinear() const
    {
        return m_light ? wp_light_get_attenuation_linear( m_light ) : GraphicsLight::getAttenuationLinear();
    }

    f32 ClawLight::getAttenuationQuadric() const
    {
        return m_light ? wp_light_get_attenuation_quadratic( m_light ) : GraphicsLight::getAttenuationQuadric();
    }

    void ClawLight::setDirection( const Vector3<real_Num> &direction )
    {
        if( m_light )
        {
            wp_light_set_direction( m_light, ClawUtil::toCVector( direction ) );
        }
        GraphicsLight::setDirection( direction );
    }

    Vector3<real_Num> ClawLight::getDirection() const
    {
        return m_light ? ClawUtil::fromCVector( wp_light_get_direction( m_light ) )
                       : GraphicsLight::getDirection();
    }

    Vector3<real_Num> ClawLight::getDerivedDirection() const
    {
        // Native scene-node transforms are authoritative during rendering.
        // The C++ world-transform cache may not have processed the latest update.
        if( m_light && wp_light_get_node( m_light ) )
            return ClawUtil::fromCVector( wp_light_get_derived_direction( m_light ) );

        if( auto owner = getOwner() )
        {
            return owner->getWorldOrientation() * getDirection();
        }

        return m_light ? ClawUtil::fromCVector( wp_light_get_derived_direction( m_light ) )
                       : GraphicsLight::getDerivedDirection();
    }

    f32 ClawLight::getPowerScale() const
    {
        return m_light ? wp_light_get_power_scale( m_light ) : GraphicsLight::getPowerScale();
    }

    void ClawLight::setPowerScale( f32 powerScale )
    {
        if( m_light )
        {
            wp_light_set_power_scale( m_light, powerScale );
        }
        GraphicsLight::setPowerScale( powerScale );
    }

    void ClawLight::setVisible( bool visible )
    {
        if( m_light )
        {
            wp_light_set_visible( m_light, visible ? 1 : 0 );
        }
        GraphicsLight::setVisible( visible );
    }

    bool ClawLight::isVisible() const
    {
        return m_light ? wp_light_is_visible( m_light ) != 0 : GraphicsLight::isVisible();
    }

    void ClawLight::setCastShadows( bool castShadows )
    {
        if( m_light )
        {
            wp_light_set_cast_shadows( m_light, castShadows ? 1 : 0 );
        }
        GraphicsLight::setCastShadows( castShadows );
    }

    bool ClawLight::getCastShadows() const
    {
        return m_light ? wp_light_get_cast_shadows( m_light ) != 0 : GraphicsLight::getCastShadows();
    }

    void ClawLight::setVisibilityFlags( u32 flags )
    {
        if( m_light )
        {
            wp_light_set_visibility_mask( m_light, flags );
        }
        GraphicsLight::setVisibilityFlags( flags );
    }

    u32 ClawLight::getVisibilityFlags() const
    {
        return m_light ? wp_light_get_visibility_mask( m_light ) : GraphicsLight::getVisibilityFlags();
    }

    SmartPtr<IGraphicsObject> ClawLight::clone( const String &name ) const
    {
        auto light = workphone::make_ptr<ClawLight>();
        light->setType( getType() );
        light->setDiffuseColour( getDiffuseColour() );
        light->setSpecularColour( getSpecularColour() );
        light->setAttenuation( getAttenuationRange(), getAttenuationConstant(), getAttenuationLinear(),
                               getAttenuationQuadric() );
        light->setDirection( getDirection() );
        light->setPowerScale( getPowerScale() );
        light->setSpotlightRange( getSpotlightInnerAngle(), getSpotlightOuterAngle(),
                                  getSpotlightFalloff() );
        light->setVisible( isVisible() );
        light->setCastShadows( getCastShadows() );
        light->setVisibilityFlags( getVisibilityFlags() );
        return light;
    }

    void ClawLight::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = m_light;
        }
    }

    void ClawLight::setPosition( const Vector3<real_Num> &position )
    {
        if( m_light )
        {
            wp_light_set_position( m_light, ClawUtil::toCVector( position ) );
        }
    }

    Vector3<real_Num> ClawLight::getPosition() const
    {
        return m_light ? ClawUtil::fromCVector( wp_light_get_position( m_light ) )
                       : Vector3<real_Num>::zero();
    }

    void ClawLight::setSpotlightRange( f32 innerAngle, f32 outerAngle, f32 falloff )
    {
        if( m_light )
        {
            wp_light_set_spotlight_range( m_light, innerAngle, outerAngle, falloff );
        }
    }

    f32 ClawLight::getSpotlightInnerAngle() const
    {
        return m_light ? wp_light_get_spotlight_inner_angle( m_light ) : 0.0f;
    }

    f32 ClawLight::getSpotlightOuterAngle() const
    {
        return m_light ? wp_light_get_spotlight_outer_angle( m_light ) : 0.0f;
    }

    f32 ClawLight::getSpotlightFalloff() const
    {
        return m_light ? wp_light_get_spotlight_falloff( m_light ) : 0.0f;
    }

    wp_light *ClawLight::getNativeLight() const
    {
        return m_light;
    }
}  // namespace workphone::render
