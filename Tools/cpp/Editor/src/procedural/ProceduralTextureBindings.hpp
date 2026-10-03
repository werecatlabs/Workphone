#ifndef WP_PROCEDURAL_TEXTURE_BINDINGS_HPP
#define WP_PROCEDURAL_TEXTURE_BINDINGS_HPP

#include <EditorPrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace render  { class ITexture; class IMaterial; }
    namespace procedural { class IProceduralTexture; }
    namespace editor
    {

        /**
         * @class ProceduralTextureBindings
         * @brief C++ bindings exposing procedural texture generation to the Lua editor layer.
         *
         * This is the single C++ entry point for all Lua-callable procedural texture operations.
         * It generates CPU-side pixel data (noise, normal maps, colour-corrected) and uploads
         * it to the runtime ITexture system, then optionally assigns to IMaterial slots.
         *
         * Implemented algorithms:
         *   - Perlin / value / worley / FBM noise (CPU, row-major RGBA)
         *   - Heightmap → normal map (Sobel-lite, configurable strength)
         *   - Colour correction: brightness, contrast, saturation, levels (black/white point)
         *   - Material slot assignment
         *   - Disk export (PPM fallback; format-specific codec delegated to runtime)
         *
         * @note Boolean/texture-atlas operations are deferred to the runtime. This binding
         *       exposes the API contract so Lua can call it.
         */
        class ProceduralTextureBindings : public IScriptReceiver
        {
        public:
            WP_CLASS_REGISTER_DECL;

            ProceduralTextureBindings();
            ~ProceduralTextureBindings() override;

            // --- Procedural texture generators ---------------------------------

            /** Generate a noise texture.
             *  noiseType: 0=perlin, 1=simplex, 2=value, 3=worley, 4=fbm.
             *  turbulence: use absolute value for ridged/fbm look. */
            SmartPtr<render::ITexture> generateNoiseTexture(
                int width, int height,
                u32 noiseType,
                real_Num frequency,
                u32 octaves,
                real_Num lacunarity,
                real_Num persistence,
                u32 seed,
                bool turbulence,
                const String &outputPath );

            /** Generate a normal map from a heightmap texture.
             *  strength: normal strength multiplier.
             *  invertY: flip Y for DXT-normal vs OpenGL conventions. */
            SmartPtr<render::ITexture> generateNormalMapFromHeight(
                SmartPtr<render::ITexture> heightTex,
                real_Num strength,
                bool invertY,
                const String &outputPath );

            /** Colour-correct an existing texture.
             *  brightness, contrast: -1..1 range.
             *  saturation: 0..2 range (0=greyscale, 1=normal, 2=oversaturated).
             *  blackPoint/whitePoint: 0..1 histogram levels. */
            SmartPtr<render::ITexture> colourCorrectTexture(
                SmartPtr<render::ITexture> sourceTex,
                real_Num brightness,
                real_Num contrast,
                real_Num saturation,
                real_Num blackPoint,
                real_Num whitePoint,
                const String &outputPath );

            // --- Material output ----------------------------------------------

            /** Assign a texture to a material texture slot by name hash. */
            void assignTextureToMaterialSlot(
                SmartPtr<render::IMaterial> material,
                const String &slotName,
                SmartPtr<render::ITexture> texture );

            /** Create or retrieve a material by name from the material manager. */
            SmartPtr<render::IMaterial> createOrGetMaterial( const String &materialName );

            // --- Persistence -------------------------------------------------

            /** Save a runtime texture to disk.
             *  format is advisory; currently writes PPM as a universal fallback. */
            bool saveTextureToFile(
                SmartPtr<render::ITexture> texture,
                const String &filePath,
                const String &format );

        private:
            s32 setProperty( hash_type, const String & ) override { return 0; }
            s32 getProperty( hash_type, String & ) const override { return 0; }
            s32 setProperty( hash_type, const Parameter & ) override { return 0; }
            s32 setProperty( hash_type, const Parameters & ) override { return 0; }
            s32 setProperty( hash_type, void * ) override { return 0; }
            s32 getProperty( hash_type, Parameter & ) const override { return 0; }
            s32 getProperty( hash_type, Parameters & ) const override { return 0; }
            s32 getProperty( hash_type, void * ) const override { return 0; }
            s32 callFunction( hash_type, const Parameters &, Parameters & ) override { return 0; }
            s32 callFunction( hash_type, SmartPtr<ISharedObject>, Parameters & ) override { return 0; }
        };

    }  // namespace editor
}  // namespace workphone

#endif  // WP_PROCEDURAL_TEXTURE_BINDINGS_HPP
