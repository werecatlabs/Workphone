#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/MaterialPassStateData.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    using namespace render;

    namespace
    {
        template <class T>
        struct EditorField
        {
            const char *name;
            T MaterialPassStateData::*member;
        };

        struct EditorBoolField
        {
            const char *name;
            u32 flag;
        };

        const EditorField<f32> FloatFields[] = {
            { "alphaClip", &MaterialPassStateData::alphaClip },
            { "opacity", &MaterialPassStateData::opacity },
            { "refractionAmount", &MaterialPassStateData::refractionAmount },
            { "refractionIor", &MaterialPassStateData::refractionIor },
            { "metalness", &MaterialPassStateData::metalness },
            { "roughness", &MaterialPassStateData::roughness },
            { "specular", &MaterialPassStateData::specular },
            { "normalStrength", &MaterialPassStateData::normalStrength },
            { "aoStrength", &MaterialPassStateData::aoStrength },
            { "heightScale", &MaterialPassStateData::heightScale },
            { "parallaxSteps", &MaterialPassStateData::parallaxSteps },
            { "emissionIntensity", &MaterialPassStateData::emissionIntensity },
            { "clearCoat", &MaterialPassStateData::clearCoat },
            { "clearCoatRoughness", &MaterialPassStateData::clearCoatRoughness },
            { "anisotropy", &MaterialPassStateData::anisotropy },
            { "uvTilingX", &MaterialPassStateData::uvTilingX },
            { "uvTilingY", &MaterialPassStateData::uvTilingY },
            { "uvOffsetX", &MaterialPassStateData::uvOffsetX },
            { "uvOffsetY", &MaterialPassStateData::uvOffsetY },
            { "uvRotation", &MaterialPassStateData::uvRotation },
            { "uvTriplanarScale", &MaterialPassStateData::uvTriplanarScale },
            { "uvAniso", &MaterialPassStateData::uvAniso },
            { "detailTilingX", &MaterialPassStateData::detailTilingX },
            { "detailTilingY", &MaterialPassStateData::detailTilingY },
            { "detailOffsetX", &MaterialPassStateData::detailOffsetX },
            { "detailOffsetY", &MaterialPassStateData::detailOffsetY },
            { "detailRotation", &MaterialPassStateData::detailRotation },
            { "detailStrength", &MaterialPassStateData::detailStrength },
            { "detailNormalStrength", &MaterialPassStateData::detailNormalStrength },
            { "maxTextureSize", &MaterialPassStateData::maxTextureSize },
            { "renderQueue", &MaterialPassStateData::renderQueue },
            { "sortPriority", &MaterialPassStateData::sortPriority },
            { "stencilRef", &MaterialPassStateData::stencilRef },
            { "stencilReadMask", &MaterialPassStateData::stencilReadMask },
            { "stencilWriteMask", &MaterialPassStateData::stencilWriteMask },
            { "layerMaskStrength", &MaterialPassStateData::layerMaskStrength },
            { "previewExposure", &MaterialPassStateData::previewExposure },
            { "previewRotation", &MaterialPassStateData::previewRotation },
        };

        const EditorField<u32> UIntFields[] = {
            { "preset", &MaterialPassStateData::preset },
            { "renderMode", &MaterialPassStateData::renderMode },
            { "workflow", &MaterialPassStateData::workflow },
            { "cullMode", &MaterialPassStateData::cullMode },
            { "uvSet", &MaterialPassStateData::uvSet },
            { "uvProjection", &MaterialPassStateData::uvProjection },
            { "uvWrapU", &MaterialPassStateData::uvWrapU },
            { "uvWrapV", &MaterialPassStateData::uvWrapV },
            { "uvFilter", &MaterialPassStateData::uvFilter },
            { "detailBlendMode", &MaterialPassStateData::detailBlendMode },
            { "metallicSource", &MaterialPassStateData::metallicSource },
            { "roughnessSource", &MaterialPassStateData::roughnessSource },
            { "aoSource", &MaterialPassStateData::aoSource },
            { "opacitySource", &MaterialPassStateData::opacitySource },
            { "heightSource", &MaterialPassStateData::heightSource },
            { "blendMode", &MaterialPassStateData::blendMode },
            { "srcBlend", &MaterialPassStateData::srcBlend },
            { "dstBlend", &MaterialPassStateData::dstBlend },
            { "depthTest", &MaterialPassStateData::depthTest },
            { "layerBlend", &MaterialPassStateData::layerBlend },
            { "materialVariant", &MaterialPassStateData::materialVariant },
            { "technique", &MaterialPassStateData::technique },
            { "pcCompression", &MaterialPassStateData::pcCompression },
            { "macCompression", &MaterialPassStateData::macCompression },
            { "iosCompression", &MaterialPassStateData::iosCompression },
            { "androidCompression", &MaterialPassStateData::androidCompression },
            { "previewShape", &MaterialPassStateData::previewShape },
            { "previewEnvironment", &MaterialPassStateData::previewEnvironment },
        };

        const EditorBoolField BoolFields[] = {
            { "transparent", transparentFlag },
            { "cutout", cutoutFlag },
            { "emissionEnabled", emissionEnabledFlag },
            { "refractionEnabled", refractionEnabledFlag },
            { "doubleSided", doubleSidedFlag },
            { "receiveShadows", receiveShadowsFlag },
            { "castShadows", castShadowsFlag },
            { "generateMipmaps", generateMipmapsFlag },
            { "srgb", srgbFlag },
            { "normalMap", normalMapFlag },
            { "textureStreaming", textureStreamingFlag },
            { "depthWrite", depthWriteFlag },
            { "gpuInstancing", gpuInstancingFlag },
            { "srpBatcher", srpBatcherFlag },
            { "receiveDecals", receiveDecalsFlag },
            { "showUvChecker", showUvCheckerFlag },
            { "showWireframe", showWireframeFlag },
            { "showTangents", showTangentsFlag },
            { "showMipLevels", showMipLevelsFlag },
        };

        const EditorField<FixedString<WP_MAX_PATH>> StringFields[] = {
            { "shaderPath", &MaterialPassStateData::shaderPath },
            { "keywords", &MaterialPassStateData::keywords },
        };

        template <class T, size_t N>
        bool setEditorField( MaterialPassStateData &state, const String &name, const T &value,
                             const EditorField<T> ( &fields )[N] )
        {
            for( const auto &field : fields )
            {
                if( name == field.name )
                {
                    state.*( field.member ) = value;
                    return true;
                }
            }

            return false;
        }

        template <class T, size_t N>
        bool getEditorField( const MaterialPassStateData &state, const String &name, T &value,
                             const EditorField<T> ( &fields )[N] )
        {
            for( const auto &field : fields )
            {
                if( name == field.name )
                {
                    value = state.*( field.member );
                    return true;
                }
            }

            return false;
        }

        template <class T, size_t N>
        void writeEditorFields( const MaterialPassStateData &state, Properties &properties,
                                const EditorField<T> ( &fields )[N] )
        {
            for( const auto &field : fields )
            {
                properties.setProperty( field.name, state.*( field.member ) );
            }
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, MaterialPassStateData, StateData );

    MaterialPassStateData::MaterialPassStateData() : StateData( MaterialPassStateData::typeInfo() )
    {
        setFlag( receiveShadowsFlag, true );
        setFlag( castShadowsFlag, true );
    }

    MaterialPassStateData::~MaterialPassStateData() = default;

    bool MaterialPassStateData::setEditorFloat( const String &name, f32 value )
    {
        return setEditorField( *this, name, value, FloatFields );
    }

    bool MaterialPassStateData::getEditorFloat( const String &name, f32 &value ) const
    {
        return getEditorField( *this, name, value, FloatFields );
    }

    bool MaterialPassStateData::setEditorUInt( const String &name, u32 value )
    {
        if( name == "materialType" )
        {
            if( value < static_cast<u32>( MaterialType::Count ) )
            {
                materialType = static_cast<MaterialType>( value );
                return true;
            }

            return false;
        }

        return setEditorField( *this, name, value, UIntFields );
    }

    bool MaterialPassStateData::getEditorUInt( const String &name, u32 &value ) const
    {
        if( name == "materialType" )
        {
            value = static_cast<u32>( static_cast<MaterialType>( materialType ) );
            return true;
        }

        return getEditorField( *this, name, value, UIntFields );
    }

    bool MaterialPassStateData::setEditorBool( const String &name, bool value )
    {
        for( const auto &field : BoolFields )
        {
            if( name == field.name )
            {
                setFlag( field.flag, value );
                return true;
            }
        }

        return false;
    }

    bool MaterialPassStateData::getEditorBool( const String &name, bool &value ) const
    {
        for( const auto &field : BoolFields )
        {
            if( name == field.name )
            {
                value = getFlag( field.flag );
                return true;
            }
        }

        return false;
    }

    bool MaterialPassStateData::setEditorString( const String &name, const String &value )
    {
        for( const auto &field : StringFields )
        {
            if( name == field.name )
            {
                ( this->*( field.member ) ) = value.c_str();
                return true;
            }
        }

        return false;
    }

    bool MaterialPassStateData::getEditorString( const String &name, String &value ) const
    {
        for( const auto &field : StringFields )
        {
            if( name == field.name )
            {
                value = ( this->*( field.member ) ).c_str();
                return true;
            }
        }

        return false;
    }

    void MaterialPassStateData::setFlag( u32 flag, bool value )
    {
        flags = BitUtil::setFlagValue( flags, flag, value );
    }

    bool MaterialPassStateData::getFlag( u32 flag ) const
    {
        return BitUtil::getFlagValue( flags, flag );
    }

    void MaterialPassStateData::writeEditorSettings( Properties &floats, Properties &uints,
                                                     Properties &bools, Properties &strings ) const
    {
        writeEditorFields( *this, floats, FloatFields );
        writeEditorFields( *this, uints, UIntFields );
        for( const auto &field : BoolFields )
        {
            bools.setProperty( field.name, getFlag( field.flag ) );
        }
        for( const auto &field : StringFields )
        {
            strings.setProperty( field.name, String( ( this->*( field.member ) ).c_str() ) );
        }
    }

}  // namespace workphone
