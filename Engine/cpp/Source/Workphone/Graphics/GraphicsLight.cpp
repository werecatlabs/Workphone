#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/GraphicsLight.hpp>
#include <Workphone/State/States/LightStateData.hpp>
#include "Workphone/State/States/LightAttenuationStateData.hpp"
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone, GraphicsLight, GraphicsObject<IGraphicsLight> );

    static const String s_typeStr = String( "type" );
    static const String s_diffuseColourStr = String( "diffuseColour" );
    static const String s_specularColourStr = String( "specularColour" );
    static const String s_rangeStr = String( "range" );
    static const String s_constantStr = String( "constant" );
    static const String s_linearStr = String( "linear" );
    static const String s_quadraticStr = String( "quadratic" );
    static const String s_powerScaleStr = String( "powerScale" );

    GraphicsLight::GraphicsLight() = default;

    GraphicsLight::~GraphicsLight() = default;

    void GraphicsLight::setType( LightTypes type )
    {
        auto iLightType = static_cast<hash32>( type );

        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->invalidateStateDataById<LightStateData>( getId() ) )
            {
                data->lightType = (LightTypes)iLightType;
            }
        }
    }

    LightTypes GraphicsLight::getType() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->getStateDataById<LightStateData>( getId() ) )
            {
                return static_cast<LightTypes>( data->lightType );
            }
        }

        return LightTypes::LT_DIRECTIONAL;
    }

    void GraphicsLight::setDiffuseColour( const ColourF &colour )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->invalidateStateDataById<LightStateData>( getId() ) )
            {
                data->diffuseColour = colour;
            }
        }
    }

    ColourF GraphicsLight::getDiffuseColour() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->getStateDataById<LightStateData>( getId() ) )
            {
                return data->diffuseColour;
            }
        }

        return ColourF::White;
    }

    void GraphicsLight::setSpecularColour( const ColourF &colour )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->invalidateStateDataById<LightStateData>( getId() ) )
            {
                data->specularColour = colour;
            }
        }
    }

    ColourF GraphicsLight::getSpecularColour() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->getStateDataById<LightStateData>( getId() ) )
            {
                return data->specularColour;
            }
        }

        return ColourF::White;
    }

    void GraphicsLight::setAttenuation( f32 range, f32 constant, f32 linear, f32 quadratic )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->invalidateStateDataById<LightAttenuationStateData>( getId() ) )
            {
                data->range = range;
                data->constant = constant;
                data->linear = linear;
                data->quadratic = quadratic;
            }
        }
    }

    f32 GraphicsLight::getAttenuationRange() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->getStateDataById<LightAttenuationStateData>( getId() ) )
            {
                return data->range;
            }
        }

        return 0.0f;
    }

    f32 GraphicsLight::getAttenuationConstant() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->getStateDataById<LightAttenuationStateData>( getId() ) )
            {
                return data->constant;
            }
        }

        return 0.0f;
    }

    f32 GraphicsLight::getAttenuationLinear() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->getStateDataById<LightAttenuationStateData>( getId() ) )
            {
                return data->linear;
            }
        }

        return 0.0f;
    }

    f32 GraphicsLight::getAttenuationQuadric() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->getStateDataById<LightAttenuationStateData>( getId() ) )
            {
                return data->quadratic;
            }
        }

        return 0.0f;
    }

    void GraphicsLight::setDirection( const Vector3<real_Num> &vec )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->invalidateStateDataById<LightStateData>( getId() ) )
            {
                data->direction = vec;
            }
        }
    }

    Vector3<real_Num> GraphicsLight::getDirection() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->getStateDataById<LightStateData>( getId() ) )
            {
                return data->direction;
            }
        }

        return Vector3<real_Num>::zero();
    }

    Vector3<real_Num> GraphicsLight::getDerivedDirection() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->getStateDataById<LightStateData>( getId() ) )
            {
                return data->derivedDirection;
            }
        }

        return Vector3<real_Num>::zero();
    }

    f32 GraphicsLight::getPowerScale() const
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->getStateDataById<LightAttenuationStateData>( getId() ) )
            {
                return data->powerScale;
            }
        }

        return 0.0f;
    }

    void GraphicsLight::setPowerScale( f32 powerScale )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto data = stateContext->invalidateStateDataById<LightAttenuationStateData>( getId() ) )
            {
                data->powerScale = powerScale;
            }
        }
    }

    SmartPtr<Properties> GraphicsLight::getProperties() const
    {
        auto properties = GraphicsObject<IGraphicsLight>::getProperties();
        properties->setProperty( s_typeStr, (u32)getType() );
        properties->setProperty( s_diffuseColourStr, getDiffuseColour() );
        properties->setProperty( s_specularColourStr, getSpecularColour() );
        properties->setProperty( s_rangeStr, getAttenuationRange() );
        properties->setProperty( s_constantStr, getAttenuationConstant() );
        properties->setProperty( s_linearStr, getAttenuationLinear() );
        properties->setProperty( s_quadraticStr, getAttenuationQuadric() );
        properties->setProperty( s_powerScaleStr, getPowerScale() );
        return properties;
    }

    void GraphicsLight::setProperties( SmartPtr<Properties> properties )
    {
        GraphicsObject<IGraphicsLight>::setProperties( properties );
    }

}  // namespace workphone::render
