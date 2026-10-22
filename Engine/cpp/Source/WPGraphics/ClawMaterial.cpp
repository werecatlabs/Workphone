#include "WPGraphics/WPClawHammerPCH.hpp"
#include "WPGraphics/ClawMaterial.hpp"
#include "WPGraphics/ClawUtil.hpp"
#include <Workphone/Workphone.hpp>
#include "workphone_graphics_material.h"

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawMaterial, Material );

        const String ClawMaterial::diffuseColourStr = "diffuseColour";
        const String ClawMaterial::specularColourStr = "specularColour";
        const String ClawMaterial::ambientColourStr = "ambientColour";
        const String ClawMaterial::emissiveColourStr = "emissiveColour";
        const String ClawMaterial::metalnessStr = "metalness";
        const String ClawMaterial::roughnessStr = "roughness";
        const String ClawMaterial::flagsStr = "flags";
        const String ClawMaterial::blendModeStr = "blendMode";
        const String ClawMaterial::cullModeStr = "cullMode";
        const String ClawMaterial::textureNamesStr = "textureNames";
        const String ClawMaterial::dirtyStr = "dirty";

        ClawMaterial::ClawMaterial() : m_material( wp_graphics_material_create() )
        {
            if( !m_material )
            {
                WP_LOG_ERROR( "ClawMaterial::ClawMaterial: failed to create native material." );
            }
        }

        ClawMaterial::~ClawMaterial()
        {
            if( m_material )
            {
                wp_graphics_material_destroy( m_material );
                m_material = nullptr;
            }
        }

        void ClawMaterial::loadResource( IResource *resource )
        {
            if( !m_material )
            {
                m_material = wp_graphics_material_create();
                if( !m_material )
                {
                    WP_LOG_ERROR( "ClawMaterial::loadResource: failed to create native material." );
                }
            }
        }

        wp_graphics_material *ClawMaterial::getNativeMaterial() const
        {
            syncNativeMaterial();
            return m_material;
        }

        void ClawMaterial::createMaterialByType()
        {
            // All material setters write pass state. A newly-created Claw material
            // previously had no passes, so edits were silently discarded.
            if( getNumTechniques() == 0u )
            {
                createTechnique();
            }
            for( auto technique : getTechniques() )
            {
                if( technique && technique->getNumPasses() == 0u )
                {
                    technique->createPass();
                }
            }
            Material::createMaterialByType();
        }

        void ClawMaterial::load( SmartPtr<ISharedObject> data )
        {
            auto application = core::IApplicationManager::instance();
            ScopedLock lock( application->getGraphicsSystem() );
            // Selection may load a resource that is already in the graphics queue.
            // Reprocessing that entry must not reload its file over live edits.
            if( isLoaded() )
            {
                return;
            }
            createMaterialByType();
            Material::load( data );
            syncNativeMaterial();
            setLoadingState( LoadingState::Loaded );
        }

        void ClawMaterial::syncNativeMaterial() const
        {
            if( !m_material ) return;
            const auto state = getRenderState();
            if( !state ) return;
            if( m_syncedRenderState == state &&
                m_syncedNativeRevision == wp_graphics_material_get_revision( m_material ) ) return;
            wp_graphics_material_values values;
            values.struct_size = sizeof( values );
            wp_graphics_material_get_values( m_material, &values );
            values.diffuse = ClawUtil::toCColour( state->diffuseColour );
            values.specular = ClawUtil::toCColour( state->specularColour );
            values.emissive = ClawUtil::toCColour( state->emissiveColour );
            values.metalness = state->metalness;
            values.roughness = state->roughness;
            values.normal_scale = state->normalStrength;
            values.occlusion_strength = state->aoStrength;
            values.emissive_intensity = state->emissionIntensity;
            values.alpha_cutoff = state->alphaClip;
            values.clearcoat = state->clearCoat;
            values.clearcoat_roughness = state->clearCoatRoughness;
            values.transmission = state->refractionAmount;
            values.index_of_refraction = state->refractionIor;
            values.blend_mode = ClawUtil::toCBlendMode( state->blendMode );
            values.cull_mode = state->getFlag( doubleSidedFlag ) ? WORKPHONE_CULL_MODE_NONE :
                                                                   ClawUtil::toCCullMode( state->cullMode );
            const auto setFlag = [&]( wp_u32 flag, bool enabled ) {
                values.flags = enabled ? values.flags | flag : values.flags & ~flag;
            };
            setFlag( WORKPHONE_MATERIAL_FLAG_TRANSPARENT, state->getFlag( render::transparentFlag ) );
            setFlag( WORKPHONE_MATERIAL_FLAG_CUTOUT, state->getFlag( render::cutoutFlag ) );
            setFlag( WORKPHONE_MATERIAL_FLAG_EMISSION, state->getFlag( emissionEnabledFlag ) );
            setFlag( WORKPHONE_MATERIAL_FLAG_REFRACTION, state->getFlag( refractionEnabledFlag ) );
            setFlag( WORKPHONE_MATERIAL_FLAG_DEPTH_WRITE, state->getFlag( depthWriteFlag ) );
            setFlag( WORKPHONE_MATERIAL_FLAG_DEPTH_CHECK, state->depthTest != 0u );
            wp_graphics_material_set_values( m_material, &values );
            m_syncedRenderState = state;
            m_syncedNativeRevision = wp_graphics_material_get_revision( m_material );
        }

        /*
        void ClawMaterial::setMetalness( f32 metalness )
        {
            Material::setMetalness( metalness );
            if( m_material )
                wp_graphics_material_set_metalness( m_material, metalness );
        }

        f32 ClawMaterial::getMetalness() const
        {
            return m_material ? wp_graphics_material_get_metalness( m_material )
                              : Material::getMetalness();
        }

        void ClawMaterial::setRoughness( f32 roughness )
        {
            Material::setRoughness( roughness );
            if( m_material )
                wp_graphics_material_set_roughness( m_material, roughness );
        }

        f32 ClawMaterial::getRoughness() const
        {
            return m_material ? wp_graphics_material_get_roughness( m_material )
                              : Material::getRoughness();
        }

        void ClawMaterial::setDiffuse( const ColourF &diffuse )
        {
            Material::setDiffuse( diffuse );
            if( m_material )
                wp_graphics_material_set_diffuse( m_material, toCColour( diffuse ) );
        }

        ColourF ClawMaterial::getDiffuse() const
        {
            return m_material ? fromCColour( wp_graphics_material_get_diffuse( m_material ) )
                              : Material::getDiffuse();
        }

        void ClawMaterial::setSpecular( const ColourF &specular )
        {
            Material::setSpecular( specular );
            if( m_material )
                wp_graphics_material_set_specular( m_material, toCColour( specular ) );
        }

        ColourF ClawMaterial::getSpecular() const
        {
            return m_material ? fromCColour( wp_graphics_material_get_specular( m_material ) )
                              : Material::getSpecular();
        }

        void ClawMaterial::setEmissive( const ColourF &emissive )
        {
            Material::setEmissive( emissive );
            if( m_material )
                wp_graphics_material_set_emissive( m_material, toCColour( emissive ) );
        }

        ColourF ClawMaterial::getEmissive() const
        {
            return m_material ? fromCColour( wp_graphics_material_get_emissive( m_material ) )
                              : Material::getEmissive();
        }

        void ClawMaterial::setNormalStrength( f32 value )
        {
            Material::setNormalStrength( value );
            if( m_material )
                wp_graphics_material_set_normal_scale( m_material, value );
        }

        f32 ClawMaterial::getNormalStrength() const
        {
            return m_material ? wp_graphics_material_get_normal_scale( m_material )
                              : Material::getNormalStrength();
        }

        void ClawMaterial::setAlphaClip( f32 value )
        {
            Material::setAlphaClip( value );
            if( m_material )
                wp_graphics_material_set_alpha_cutoff( m_material, value );
        }

        f32 ClawMaterial::getAlphaClip() const
        {
            return m_material ? wp_graphics_material_get_alpha_cutoff( m_material )
                              : Material::getAlphaClip();
        }

        void ClawMaterial::setOpacity( f32 value )
        {
            Material::setOpacity( value );
            if( m_material )
            {
                auto colour = wp_graphics_material_get_diffuse( m_material );
                colour.a = value;
                wp_graphics_material_set_diffuse( m_material, colour );
                wp_graphics_material_set_flag( m_material, WORKPHONE_MATERIAL_FLAG_TRANSPARENT,
                                               value < 0.999f ? 1 : 0 );
            }
        }

        f32 ClawMaterial::getOpacity() const
        {
            return m_material ? wp_graphics_material_get_diffuse( m_material ).a
                              : Material::getOpacity();
        }

        void ClawMaterial::setBlendMode( u32 mode )
        {
            Material::setBlendMode( mode );
            if( m_material )
                wp_graphics_material_set_blend_mode( m_material, toCBlendMode( mode ) );
        }

        u32 ClawMaterial::getBlendMode() const
        {
            return m_material ? fromCBlendMode( wp_graphics_material_get_blend_mode( m_material ) )
                              : Material::getBlendMode();
        }

        void ClawMaterial::setDepthWrite( bool enabled )
        {
            Material::setDepthWrite( enabled );
            if( m_material )
                wp_graphics_material_set_flag( m_material, WORKPHONE_MATERIAL_FLAG_DEPTH_WRITE,
                                               enabled ? 1 : 0 );
        }

        bool ClawMaterial::getDepthWrite() const
        {
            return m_material ? wp_graphics_material_has_flag( m_material,
                                                               WORKPHONE_MATERIAL_FLAG_DEPTH_WRITE ) != 0
                              : Material::getDepthWrite();
        }

        void ClawMaterial::setDepthTest( u32 mode )
        {
            Material::setDepthTest( mode );
            if( m_material )
                wp_graphics_material_set_flag( m_material, WORKPHONE_MATERIAL_FLAG_DEPTH_CHECK,
                                               mode != 0u ? 1 : 0 );
        }

        u32 ClawMaterial::getDepthTest() const
        {
            return m_material && !wp_graphics_material_has_flag( m_material,
                                                                 WORKPHONE_MATERIAL_FLAG_DEPTH_CHECK )
                       ? 0u
                       : Material::getDepthTest();
        }

        void ClawMaterial::setCullMode( u32 mode )
        {
            Material::setCullMode( mode );
            if( m_material )
                wp_graphics_material_set_cull_mode( m_material, toCCullMode( mode ) );
        }

        u32 ClawMaterial::getCullMode() const
        {
            return m_material ? fromCCullMode( wp_graphics_material_get_cull_mode( m_material ) )
                              : Material::getCullMode();
        }

        void ClawMaterial::setTransparent( bool transparent )
        {
            Material::setTransparent( transparent );
            if( m_material )
                wp_graphics_material_set_flag( m_material, WORKPHONE_MATERIAL_FLAG_TRANSPARENT,
                                               transparent ? 1 : 0 );
        }

        bool ClawMaterial::isTransparent() const
        {
            return m_material ? wp_graphics_material_has_flag( m_material,
                                                               WORKPHONE_MATERIAL_FLAG_TRANSPARENT ) != 0
                              : Material::isTransparent();
        }

        void ClawMaterial::setCutout( bool cutout )
        {
            Material::setCutout( cutout );
            if( m_material )
                wp_graphics_material_set_flag( m_material, WORKPHONE_MATERIAL_FLAG_CUTOUT,
                                               cutout ? 1 : 0 );
        }

        bool ClawMaterial::isCutout() const
        {
            return m_material
                       ? wp_graphics_material_has_flag( m_material, WORKPHONE_MATERIAL_FLAG_CUTOUT ) != 0
                       : Material::isCutout();
        }
        */

        SmartPtr<Properties> ClawMaterial::getProperties() const
        {
            try
            {
                syncNativeMaterial();
                auto properties = Material::getProperties();
                if( !properties )
                {
                    WP_LOG_WARNING(
                        "ClawMaterial::getProperties: base class returned null properties." );
                    return {};
                }

                if( !m_material )
                {
                    WP_LOG_WARNING(
                        "ClawMaterial::getProperties: native material is null, returning base "
                        "properties." );
                    return properties;
                }

                properties->setProperty(
                    diffuseColourStr,
                    ClawUtil::fromCColour( wp_graphics_material_get_diffuse( m_material ) ) );
                properties->setProperty(
                    specularColourStr,
                    ClawUtil::fromCColour( wp_graphics_material_get_specular( m_material ) ) );
                properties->setProperty(
                    ambientColourStr,
                    ClawUtil::fromCColour( wp_graphics_material_get_ambient( m_material ) ) );
                properties->setProperty(
                    emissiveColourStr,
                    ClawUtil::fromCColour( wp_graphics_material_get_emissive( m_material ) ) );
                properties->setProperty( metalnessStr,
                                         wp_graphics_material_get_metalness( m_material ) );
                properties->setProperty( roughnessStr,
                                         wp_graphics_material_get_roughness( m_material ) );
                properties->setProperty( flagsStr, wp_graphics_material_get_flags( m_material ) );
                properties->setProperty( blendModeStr, getBlendMode() );
                properties->setProperty( cullModeStr, getCullMode() );

                auto textureProperties = workphone::make_ptr<Properties>();
                textureProperties->setName( textureNamesStr );
                properties->addChild( textureProperties );

                for( s32 layer = 0; layer < WP_MATERIAL_MAX_TEXTURES; ++layer )
                {
                    const auto name = getTextureName( static_cast<u32>( layer ) );
                    textureProperties->setProperty( "texture" + String( std::to_string( layer ) ),
                                                    name );
                }

                properties->setProperty( dirtyStr, wp_graphics_material_is_dirty( m_material ) != 0 );

                return properties;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return {};
        }

        void ClawMaterial::setProperties( SmartPtr<Properties> properties )
        {
            try
            {
                if( !properties )
                {
                    WP_LOG_WARNING( "ClawMaterial::setProperties: null properties supplied." );
                    return;
                }

                Material::setProperties( properties );

                if( !m_material )
                {
                    WP_LOG_WARNING(
                        "ClawMaterial::setProperties: native material is null, only base properties "
                        "applied." );
                    return;
                }

                // A saved bag contains both shader values and native aliases.
                // Restore aliases as pass overrides when an authored semantic is
                // present, rather than feeding those mirrors back into its owner.
                // A partial legacy-only bag remains a material-wide editor edit.
                const auto authored = [&]( const String &name ) {
                    const bool active = m_shaderWorkflow == ShaderWorkflow::Pbr ||
                        ( m_shaderWorkflow == ShaderWorkflow::Unlit &&
                          ( name == shaderEmissiveParamStr || name == shaderOpacityParamStr ) );
                    return active &&
                           properties->hasProperty( shaderParamPrefixStr + name );
                };
                const auto restorePasses = [&]( const auto &update ) {
                    for( auto technique : getTechniques() )
                        if( technique )
                            for( auto pass : technique->getPasses() )
                                if( pass ) update( *pass );
                };
                auto diffuseColour = getDiffuse();
                if( properties->getPropertyValue( diffuseColourStr, diffuseColour ) )
                {
                    if( authored( shaderAlbedoParamStr ) || authored( shaderOpacityParamStr ) )
                        restorePasses( [&]( IMaterialPass &pass ) { pass.setDiffuse( diffuseColour ); } );
                    else setDiffuse( diffuseColour );
                }

                auto specularColour = getSpecular();
                if( properties->getPropertyValue( specularColourStr, specularColour ) )
                {
                    setSpecular( specularColour );
                }

                auto ambientColour =
                    ClawUtil::fromCColour( wp_graphics_material_get_ambient( m_material ) );
                if( properties->getPropertyValue( ambientColourStr, ambientColour ) )
                {
                    wp_graphics_material_set_ambient( m_material, ClawUtil::toCColour( ambientColour ) );
                }

                auto emissiveColour = getEmissive();
                if( properties->getPropertyValue( emissiveColourStr, emissiveColour ) )
                {
                    if( authored( shaderEmissiveParamStr ) )
                        restorePasses( [&]( IMaterialPass &pass ) { pass.setEmissive( emissiveColour ); } );
                    else setEmissive( emissiveColour );
                }

                auto metalness = getMetalness();
                if( properties->getPropertyValue( metalnessStr, metalness ) )
                {
                    if( authored( shaderMetalnessParamStr ) )
                        restorePasses( [&]( IMaterialPass &pass ) { pass.setMetalness( metalness ); } );
                    else setMetalness( metalness );
                }

                auto roughness = getRoughness();
                if( properties->getPropertyValue( roughnessStr, roughness ) )
                {
                    if( authored( shaderRoughnessParamStr ) )
                        restorePasses( [&]( IMaterialPass &pass ) { pass.setRoughness( roughness ); } );
                    else setRoughness( roughness );
                }

                auto flags = wp_graphics_material_get_flags( m_material );
                if( properties->getPropertyValue( flagsStr, flags ) )
                {
                    wp_graphics_material_set_flags( m_material, flags );
                    setTransparent( ( flags & WORKPHONE_MATERIAL_FLAG_TRANSPARENT ) != 0 );
                    setCutout( ( flags & WORKPHONE_MATERIAL_FLAG_CUTOUT ) != 0 );
                    setEmissionEnabled( ( flags & WORKPHONE_MATERIAL_FLAG_EMISSION ) != 0 );
                    setRefractionEnabled( ( flags & WORKPHONE_MATERIAL_FLAG_REFRACTION ) != 0 );
                    setDepthWrite( ( flags & WORKPHONE_MATERIAL_FLAG_DEPTH_WRITE ) != 0 );
                    if( !( flags & WORKPHONE_MATERIAL_FLAG_DEPTH_CHECK ) )
                    {
                        setDepthTest( 0u );
                    }
                }

                auto blendMode = static_cast<u32>( wp_graphics_material_get_blend_mode( m_material ) );
                if( properties->getPropertyValue( blendModeStr, blendMode ) )
                {
                    setBlendMode( blendMode );
                }

                auto cullMode = static_cast<u32>( wp_graphics_material_get_cull_mode( m_material ) );
                if( properties->getPropertyValue( cullModeStr, cullMode ) )
                {
                    setCullMode( cullMode );
                }

                if( auto textureProperties = properties->getChild( textureNamesStr ) )
                {
                    for( s32 layer = 0; layer < WP_MATERIAL_MAX_TEXTURES; ++layer )
                    {
                        auto textureName = String();
                        String propertyName = "texture" + String( std::to_string( layer ) );
                        if( textureProperties->getPropertyValue( propertyName, textureName ) )
                        {
                            auto cStr = static_cast<const wp_c8 *>(
                                static_cast<const void *>( textureName.c_str() ) );
                            wp_graphics_material_set_texture_name( m_material, layer, cStr );
                            if( textureName.empty() )
                            {
                                setTexture( SmartPtr<ITexture>(), static_cast<u32>( layer ) );
                            }
                            else
                            {
                                setTexture( textureName, static_cast<u32>( layer ) );
                            }
                        }
                    }
                }

                bool dirty = wp_graphics_material_is_dirty( m_material ) != 0;
                if( properties->getPropertyValue( dirtyStr, dirty ) )
                {
                    if( dirty )
                    {
                        wp_graphics_material_mark_dirty( m_material );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void ClawMaterial::_getObject( void **ppObject ) const
        {
            if( ppObject )
            {
                *ppObject = getNativeMaterial();
            }
        }

        bool ClawMaterial::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        bool ClawMaterial::handleStateChanged( SmartPtr<IState> &state )
        {
            return Material::handleStateChanged( state );
        }

    }  // namespace render
}  // namespace workphone
