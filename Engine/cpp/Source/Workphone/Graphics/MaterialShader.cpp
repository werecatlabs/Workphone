// Data-driven material shader descriptor + registry.
//
// Step 3 of the Esoterica material integration. Renderer-agnostic descriptor for
// a material shader (name + flags + parameter list) plus a process-wide registry.
// Built-in shaders (DefaultPBR / Unlit / ColorOnly) are registered lazily.

#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Graphics/MaterialShader.hpp"

namespace workphone
{
    namespace render
    {

        //---------------------------------------------------------------------------------------
        // MaterialShader
        //---------------------------------------------------------------------------------------

        void MaterialShader::buildInstance( MaterialShaderParametersInstance &outInstance ) const
        {
            outInstance.Reset( m_parameterInfo );
            outInstance.InitializeDefaultResourceHandles();
        }

        //---------------------------------------------------------------------------------------
        // MaterialShaderRegistry
        //---------------------------------------------------------------------------------------

        MaterialShaderRegistry &MaterialShaderRegistry::instance()
        {
            static MaterialShaderRegistry s_registry;
            return s_registry;
        }

        void MaterialShaderRegistry::registerShader( const MaterialShader &shader )
        {
            if( !shader.isValid() )
            {
                return;
            }

            for( auto &existing : m_shaders )
            {
                if( existing.getShaderID() == shader.getShaderID() )
                {
                    existing = shader;
                    return;
                }
            }
            m_shaders.push_back( shader );
        }

        const MaterialShader *MaterialShaderRegistry::find( const String &shaderID ) const
        {
            for( const auto &shader : m_shaders )
            {
                if( shader.getShaderID() == shaderID )
                {
                    return &shader;
                }
            }
            return nullptr;
        }

        //---------------------------------------------------------------------------------------
        // Built-in shaders
        //
        // These mirror the standard PBR parameter set the legacy Material already exposes, plus a
        // couple of non-PBR workflows to demonstrate that the system is no longer hard-coded to
        // PBR. The editor (step 4) lists these in the shader picker and auto-generates a property
        // grid from each shader's parameter info.
        //---------------------------------------------------------------------------------------

        void MaterialShaderRegistry::registerBuiltinShaders()
        {
            if( m_builtinsRegistered )
            {
                return;
            }
            m_builtinsRegistered = true;

            // DefaultPBR: the standard physically-based workflow. Every slot below maps cleanly
            // onto the legacy Material PBR getters via syncPBRToShaderParameters().
            {
                MaterialShader pbr( "DefaultPBR" );
                pbr.addParameter( MaterialShaderParameterType::Colour, "Albedo" )
                    .addParameter( MaterialShaderParameterType::Scalar, "Metalness" )
                    .addParameter( MaterialShaderParameterType::Scalar, "Roughness" )
                    .addParameter( MaterialShaderParameterType::Colour, "Emissive" )
                    .addParameter( MaterialShaderParameterType::Scalar, "Opacity" )
                    .addParameter( MaterialShaderParameterType::Scalar, "AlphaClip" )
                    .addParameter( MaterialShaderParameterType::Texture, "AlbedoMap" )
                    .addParameter( MaterialShaderParameterType::Texture, "NormalMap" )
                    .addParameter( MaterialShaderParameterType::Texture, "RoughnessMap" )
                    .addParameter( MaterialShaderParameterType::Texture, "MetallicMap" )
                    .addParameter( MaterialShaderParameterType::Texture, "EmissiveMap" )
                    .addParameter( MaterialShaderParameterType::Texture, "OcclusionMap" );
                registerShader( pbr );
            }

            // Unlit: a non-PBR workflow. Emissive colour + a single texture, no lighting response.
            {
                MaterialShader unlit( "Unlit", static_cast<u32>( MaterialShaderFlag::AlphaBlend ) );
                unlit.addParameter( MaterialShaderParameterType::Colour, "Emissive" )
                    .addParameter( MaterialShaderParameterType::Scalar, "Opacity" )
                    .addParameter( MaterialShaderParameterType::Texture, "AlbedoMap" );
                registerShader( unlit );
            }

            // ColorOnly: the simplest possible workflow; a flat colour with opacity. Demonstrates
            // that a material shader need not declare any textures at all.
            {
                MaterialShader colorOnly( "ColorOnly" );
                colorOnly.addParameter( MaterialShaderParameterType::Colour, "Color" )
                    .addParameter( MaterialShaderParameterType::Scalar, "Opacity" );
                registerShader( colorOnly );
            }
        }

    }  // namespace render
}  // namespace workphone
