#ifndef CMaterialOgreNext_h__
#define CMaterialOgreNext_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Memory/AtomicRawPtr.hpp>
#include <OgreMaterial.h>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Material implementation for the Ogre Next rendering backend.
         *
         * This class wraps Ogre-specific material objects and provides the
         * engine-agnostic Material interface used by the rest of the engine.
         * It manages an Ogre::Material, an optional HlmsDatablock pointer and
         * related texture resources.
         */
        class CMaterialOgreNext : public Material
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Constructs an empty material wrapper. The underlying Ogre
             * objects will be created when the material is loaded.
             */
            CMaterialOgreNext();

            /**
             * @brief Construct a material wrapper with a resource manager.
             * @param resourceManager Pointer to the resource manager used for
             *        creating and loading textures and other resources.
             */
            CMaterialOgreNext( IResourceManager *resourceManager );

            /**
             * @brief Destructor — releases any owned Ogre resources.
             */
            ~CMaterialOgreNext() override;

            /**
             * @brief Load material data from a shared object.
             * @param data Shared object containing material definitions.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Reload material data (e.g. when source files changed).
             * @param data Shared object containing updated material data.
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload material resources and release GPU memory.
             * @param data Optional shared object associated with the material.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Set a cube texture from 6 individual texture faces.
             * @param textures Array of 6 textures representing cube faces.
             * @param layerIdx Index of the texture layer to set.
             */
            void setCubicTexture( const Array<SmartPtr<ITexture>> &textures, u32 layerIdx = 0 ) override;

            /**
             * @brief Set a cube texture from a single file (e.g. DDS or cube map file).
             * @param fileName Path to the cube texture file.
             * @param uvw True if texture coordinates should use UVW mapping.
             * @param layerIdx Index of the texture layer to set.
             */
            void setCubicTexture( const String &fileName, bool uvw, u32 layerIdx = 0 ) override;

            /**
             * @name Set shader fragment parameters
             * Convenience overloads to set scalar and vector parameters used by
             * fragment/fragment-like shaders.
             */
            //@{
            void setFragmentParam( const String &name, f32 value ) override;
            void setFragmentParam( const String &name, const Vector2F &value ) override;
            void setFragmentParam( const String &name, const Vector3F &value ) override;
            void setFragmentParam( const String &name, const Vector4F &value ) override;
            void setFragmentParam( const String &name, const ColourF &value ) override;
            //@}

            /**
             * @brief Create and return a new technique for this material.
             * @return Smart pointer to the created IMaterialTechnique instance.
             */
            SmartPtr<IMaterialTechnique> createTechnique() override;

            /**
             * @brief Set texture scale for a specific texture/pass/technique.
             * @param scale Scaling vector to apply to the texture coordinates.
             * @param textureIndex Index of the texture within the pass.
             * @param passIndex Index of the pass within the technique.
             * @param techniqueIndex Index of the technique within the material.
             */
            void setScale( const Vector3F &scale, u32 textureIndex = 0, u32 passIndex = 0,
                           u32 techniqueIndex = 0 ) override;

            /** @copydoc Material::setUVTiling */
            void setUVTiling( const Vector2F &tiling ) override;

            /** @copydoc Material::setUVOffset */
            void setUVOffset( const Vector2F &offset ) override;

            /** @copydoc Material::setUVRotation */
            void setUVRotation( f32 rotation ) override;

            /**
             * @copydoc Material::getChildObjects
             *
             * Returns child objects owned by this material, such as textures
             * or techniques, so they can be inspected or serialized by the
             * resource system.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Access the underlying Ogre HLMS datablock pointer.
             * @return Pointer to the Ogre::HlmsDatablock or nullptr if none set.
             */
            Ogre::HlmsDatablock *getHlmsDatablock() const;

            /**
             * @brief Set the HlmsDatablock used by this material.
             * @param hlmsDatablock Pointer to an existing Ogre::HlmsDatablock.
             */
            void setHlmsDatablock( Ogre::HlmsDatablock *hlmsDatablock );

            /**
             * @brief Get the name of the datablock associated with this material.
             * @return Datablock name string.
             */
            String getDatablockName() const;

            /**
             * @brief Set the name of the datablock to reference for this material.
             * @param datablockName Name of the datablock.
             */
            void setDatablockName( const String &datablockName );

            /**
             * @brief Handle incoming state messages relevant to this material.
             * @param message State message to process.
             * @return True if the message was handled, false otherwise.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief React to changes in attached state objects.
             * @param state The state that has changed.
             * @return True if the change was handled, false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Create or initialize the state object used by this material
             *        for responding to engine state updates.
             */
            void createStateObject();

            /**
             * @brief Create the underlying material object according to the
             *        material type. Overrides Material::createMaterialByType.
             */
            void createMaterialByType() override;

            /// Pointer to the Ogre HLMS datablock (may be nullptr).
            AtomicRawPtr<Ogre::HlmsDatablock> m_hlmsDatablock;

            /// Cached backend-specific datablocks. Reusing these prevents repeated workflow
            /// changes from allocating an unbounded number of Ogre datablocks.
            AtomicRawPtr<Ogre::HlmsDatablock> m_pbsDatablock;
            AtomicRawPtr<Ogre::HlmsDatablock> m_unlitDatablock;

            /// Last material type successfully applied to the renderer. MaterialStateData is
            /// the requested type; a mismatch causes the state task to retry reconstruction.
            Atomic<MaterialType> m_appliedMaterialType = MaterialType::Count;

            /// Reference-counted Ogre material object managed by Ogre.
            Ogre::MaterialPtr m_material;

            /// Name of the HLMS datablock used by this material (if any).
            String m_datablockName;

            String m_pbsDatablockName;
            String m_unlitDatablockName;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // IMaterial_h__
