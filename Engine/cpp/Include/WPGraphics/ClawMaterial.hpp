#ifndef ClawMaterial_h__
#define ClawMaterial_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/Material.hpp>

struct wp_graphics_material;

namespace workphone
{
    namespace render
    {
        /**
         * @class ClawMaterial
         * @brief Production-ready material implementation backed by the C89 wp_graphics_material API.
         *
         * ClawMaterial wraps the low-level @c wp_graphics_material handle and bridges it to the
         * engine's @c Material base class. It provides defensive null-checking on every C API
         * call, logs errors through the engine's @c WP_LOG_* macros, and exposes material
         * properties (diffuse, specular, ambient, emissive, metalness, roughness, blend mode,
         * cull mode, flags, texture names) to the game editor through a data-driven
         * @c getProperties / @c setProperties implementation.
         *
         * @see Material
         * @see wp_graphics_material
         */
        class WPGraphics_API ClawMaterial : public Material
        {
        public:
            /** @brief Property name for the diffuse colour channel. */
            static const String diffuseColourStr;
            /** @brief Property name for the specular colour channel. */
            static const String specularColourStr;
            /** @brief Property name for the ambient colour channel. */
            static const String ambientColourStr;
            /** @brief Property name for the emissive colour channel. */
            static const String emissiveColourStr;
            /** @brief Property name for the metalness PBR parameter. */
            static const String metalnessStr;
            /** @brief Property name for the roughness PBR parameter. */
            static const String roughnessStr;
            /** @brief Property name for the material flags bitmask. */
            static const String flagsStr;
            /** @brief Property name for the blend mode enum. */
            static const String blendModeStr;
            /** @brief Property name for the cull mode enum. */
            static const String cullModeStr;
            /** @brief Property name for the texture names child group. */
            static const String textureNamesStr;
            /** @brief Property name for the dirty state flag. */
            static const String dirtyStr;

            /**
             * @brief Default constructor.
             *
             * Creates a new ClawMaterial by allocating a @c wp_graphics_material handle.
             * If allocation fails, the error is logged and the handle remains null;
             * subsequent operations will safely fall back to the base class.
             */
            ClawMaterial();

            /**
             * @brief Destructor.
             *
             * Releases the underlying @c wp_graphics_material handle if one was
             * successfully created. The pointer is set to null after destruction.
             */
            ~ClawMaterial() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void createMaterialByType() override;

            ClawMaterial( const ClawMaterial & ) = delete;
            ClawMaterial &operator=( const ClawMaterial & ) = delete;

            /**
             * @brief Loads the material from an external resource.
             *
             * Ensures the native @c wp_graphics_material handle exists, creating
             * one if it was previously null. Errors are logged but do not throw.
             *
             * @param resource Pointer to the source resource (may be null).
             */
            void loadResource( IResource *resource );

            /**
             * @brief Retrieves the native C89 material handle.
             *
             * @return Pointer to the @c wp_graphics_material, or null if creation failed.
             */
            wp_graphics_material *getNativeMaterial() const;

            /*
            void setMetalness( f32 metalness ) override;
            f32 getMetalness() const override;
            void setRoughness( f32 roughness ) override;
            f32 getRoughness() const override;
            void setDiffuse( const ColourF &diffuse ) override;
            ColourF getDiffuse() const override;
            void setSpecular( const ColourF &specular ) override;
            ColourF getSpecular() const override;
            void setEmissive( const ColourF &emissive ) override;
            ColourF getEmissive() const override;
            void setNormalStrength( f32 value ) override;
            f32 getNormalStrength() const override;
            void setAlphaClip( f32 value ) override;
            f32 getAlphaClip() const override;
            void setOpacity( f32 value ) override;
            f32 getOpacity() const override;
            void setBlendMode( u32 blendMode ) override;
            u32 getBlendMode() const override;
            void setDepthWrite( bool enabled ) override;
            bool getDepthWrite() const override;
            void setDepthTest( u32 mode ) override;
            u32 getDepthTest() const override;
            void setCullMode( u32 mode ) override;
            u32 getCullMode() const override;
            void setTransparent( bool transparent ) override;
            bool isTransparent() const override;
            void setCutout( bool cutout ) override;
            bool isCutout() const override;
            */

            /**
             * @brief Retrieves material properties for the game editor.
             *
             * Calls the base class @c getProperties first, then augments the
             * result with Claw-specific native properties from the C API.
             *
             * @return Smart pointer to a Properties object, or null on failure.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Applies editor-supplied properties to the material.
             *
             * Reads property values from the supplied Properties object and
             * pushes them to both the C API and the base class. Missing
             * properties are silently skipped.
             *
             * @param properties Smart pointer to a Properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Internal method to retrieve the underlying raw object pointer.
             *
             * @param ppObject Out parameter that receives the @c wp_graphics_material pointer.
             */
            void _getObject( void **ppObject ) const override;

            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            void syncNativeMaterial() const;
            wp_graphics_material *m_material;  ///< Underlying Claw material handle.
        };
    }  // namespace render
}  // namespace workphone

#endif  // MaterialClaw_h__
