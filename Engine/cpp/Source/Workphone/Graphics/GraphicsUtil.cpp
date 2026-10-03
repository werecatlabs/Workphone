#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/GraphicsUtil.hpp>
#include <Workphone/Core/Handle.hpp>

namespace workphone
{
    namespace render
    {

        String GraphicsUtil::getTerrainTextureType( TerrainTextureTypes terrainTextureType )
        {
            switch( terrainTextureType )
            {
            case TerrainTextureTypes::Base:
                return String( "Base" );
            case TerrainTextureTypes::Splat:
                return String( "Splat" );
            case TerrainTextureTypes::Diffuse1:
                return String( "Layer1Diffuse" );
            case TerrainTextureTypes::Normal1:
                return String( "Layer1Normal" );
            case TerrainTextureTypes::Diffuse2:
                return String( "Layer2Diffuse" );
            case TerrainTextureTypes::Normal2:
                return String( "Layer2Normal" );
            case TerrainTextureTypes::Diffuse3:
                return String( "Layer3Diffuse" );
            case TerrainTextureTypes::Normal3:
                return String( "Layer3Normal" );
            case TerrainTextureTypes::Diffuse4:
                return String( "Layer4Diffuse" );
            case TerrainTextureTypes::Normal4:
                return String( "Layer4Normal" );
            default:
            {
            }
            break;
            }

            return String( "" );
        }

        String GraphicsUtil::getPbsTextureType( PbsTextureTypes pbsTextureType )
        {
            switch( pbsTextureType )
            {
            case PbsTextureTypes::PBSM_DIFFUSE:
                return String( "Diffuse" );
            case PbsTextureTypes::PBSM_NORMAL:
                return String( "Normal" );
                // case PbsTextureTypes::PBSM_SPECULAR:
                //	break;
            case PbsTextureTypes::PBSM_METALLIC:
                return String( "Metallic" );
            case PbsTextureTypes::PBSM_ROUGHNESS:
                return String( "Roughness" );
            case PbsTextureTypes::PBSM_DETAIL_WEIGHT:
                return String( "Detail Weight" );
            case PbsTextureTypes::PBSM_DETAIL0:
                return String( "Detail0" );
            case PbsTextureTypes::PBSM_DETAIL1:
                return String( "Detail1" );
            case PbsTextureTypes::PBSM_DETAIL2:
                return String( "Detail2" );
            case PbsTextureTypes::PBSM_DETAIL3:
                return String( "Detail3" );
            case PbsTextureTypes::PBSM_DETAIL0_NM:
                return String( "Detail0_nm" );
            case PbsTextureTypes::PBSM_DETAIL1_NM:
                return String( "Detail1_nm" );
            case PbsTextureTypes::PBSM_DETAIL2_NM:
                return String( "Detail2_nm" );
            case PbsTextureTypes::PBSM_DETAIL3_NM:
                return String( "Detail3_nm" );
            case PbsTextureTypes::PBSM_EMISSIVE:
                return String( "Emissive" );
            case PbsTextureTypes::PBSM_REFLECTION:
                return String( "Reflection" );
            default:
                break;
            };

            return String( "" );
        }

        String GraphicsUtil::getSkyboxTextureType( SkyboxTextureTypes skyboxTextureType )
        {
            switch( skyboxTextureType )
            {
            case SkyboxTextureTypes::Front:
                return String( "Front" );
            case SkyboxTextureTypes::Back:
                return String( "Back" );
            case SkyboxTextureTypes::Left:
                return String( "Left" );
            case SkyboxTextureTypes::Right:
                return String( "Right" );
            case SkyboxTextureTypes::Up:
                return String( "Up" );
            case SkyboxTextureTypes::Down:
                return String( "Down" );
            //case SkyboxTextureTypes::Cube:
            //    return String( "Cube" );
            default:
            {
                WP_ASSERT( false );
            }
            break;
            };

            return String( "" );
        }

        String GraphicsUtil::getSkyboxCubeTextureType( SkyboxCubeTextureTypes skyboxTextureType )
        {
            switch( skyboxTextureType )
            {
            case SkyboxCubeTextureTypes::Cube:
                return String( "Base" );
            default:
            {
                WP_ASSERT( false );
            }
            break;
            };

            return String( "" );
        }

        String GraphicsUtil::getMaterialType( MaterialType materialType )
        {
            switch( materialType )
            {
            case MaterialType::Standard:
                return String( "Standard" );
            case MaterialType::StandardSpecular:
                return String( "StandardSpecular" );
            case MaterialType::StandardTriPlanar:
                return String( "StandardTriPlanar" );
            case MaterialType::TerrainStandard:
                return String( "TerrainStandard" );
            case MaterialType::TerrainSpecular:
                return String( "TerrainSpecular" );
            case MaterialType::TerrainDiffuse:
                return String( "TerrainDiffuse" );
            case MaterialType::Skybox:
                return String( "Skybox" );
            case MaterialType::SkyboxCubemap:
                return String( "SkyboxCubemap" );
            case MaterialType::UI:
                return String( "UI" );
            case MaterialType::Custom:
                return String( "Custom" );
            default:
                break;
            }

            return String( "" );
        }

        Array<String> GraphicsUtil::getMaterialTypes()
        {
            Array<String> enumValues;
            enumValues.reserve( (size_t)MaterialType::Count );

            for( size_t i = 0; i < (size_t)MaterialType::Count; ++i )
            {
                auto eMaterialType = (MaterialType)i;
                auto eMaterialTypeStr = GraphicsUtil::getMaterialType( eMaterialType );
                enumValues.push_back( eMaterialTypeStr );
            }

            return enumValues;
        }

        String GraphicsUtil::getMaterialTypesString()
        {
            auto enumValues = String( "" );
            for( size_t i = 0; i < (size_t)MaterialType::Count; ++i )
            {
                auto eMaterialType = (MaterialType)i;
                auto eMaterialTypeStr = GraphicsUtil::getMaterialType( eMaterialType );
                // StringUtil::parseArray preserves whitespace in each token. Do not add a
                // space after the delimiter or enum property grids will fail to match every
                // material type after the first entry against its canonical value.
                enumValues += eMaterialTypeStr + ";";
            }

            return enumValues;
        }

    }  // end namespace render
}  // namespace workphone
