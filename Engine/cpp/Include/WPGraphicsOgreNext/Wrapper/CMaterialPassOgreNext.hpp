#ifndef CMaterialPassOgreNext_h__
#define CMaterialPassOgreNext_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Graphics/MaterialPass.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <OgreCommon.h>

namespace workphone
{
    namespace render
    {

        /**
         * @class CMaterialPassOgreNext
         * @brief Ogre-next implementation of a material pass.
         *
         * This class wraps an Ogre::Pass and exposes the Workphone/Graphics
         * MaterialPass interface so the rest of the engine can interact
         * with Ogre materials through a common API.
         */
        class CMaterialPassOgreNext : public MaterialPass
        {
        public:
            /**
             * Construct a new CMaterialPassOgreNext instance.
             * The internal Ogre::Pass pointer is initialised to nullptr.
             */
            CMaterialPassOgreNext();

            /**
             * Destructor. Releases any engine-side resources owned by the pass.
             */
            ~CMaterialPassOgreNext() override;

            /**
             * Load resources or state for this material pass from the provided
             * shared object. The actual contents depend on the concrete
             * material implementation and data format used by the engine.
             *
             * @param data Shared object containing material pass data.
             * @see MaterialPass::load
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * Reload resources for this pass. This is typically used when
             * the underlying material data has been modified and must be
             * refreshed without recreating the entire material.
             *
             * @param data Shared object containing updated material pass data.
             * @see MaterialPass::reload
             */
            void reload( SmartPtr<ISharedObject> data ) override;

            /**
             * Unload or release resources associated with this pass. After
             * calling unload the pass should be in a state where it can be
             * reloaded or safely destroyed.
             *
             * @param data Optional shared object used during unload.
             * @see MaterialPass::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * Initialise this wrapper with an existing Ogre::Pass instance.
             * The wrapper will hold a non-owning pointer to the provided
             * pass and use it for subsequent material operations.
             *
             * @param pass Pointer to an existing Ogre::Pass (non-owning).
             */
            void initialise( Ogre::Pass *pass );

            /**
             * Create a new texture unit for this material pass.
             *
             * @return Smart pointer to the newly created material texture unit.
             * @see MaterialPass::createTextureUnit
             */
            SmartPtr<IMaterialTexture> createTextureUnit() override;

            /**
             * Set a texture resource on the given texture layer/index of the
             * pass. This will bind the texture to the Ogre pass texture unit.
             *
             * @param texture Smart pointer to the texture to assign.
             * @param layerIdx Index of the texture layer (default 0).
             * @see MaterialPass::setTexture
             */
            void setTexture( SmartPtr<ITexture> texture, u32 layerIdx = 0 ) override;

            /**
             * Convenience overload to set a texture by filename on the
             * specified texture layer. The implementation will typically
             * load or look up the texture resource before binding it.
             *
             * @param fileName Path or identifier of the texture resource.
             * @param layerIdx Index of the texture layer (default 0).
             * @see MaterialPass::setTexture
             */
            void setTexture( const String &fileName, u32 layerIdx = 0 ) override;

            /** @copydoc MaterialPass::getVertexShaderName */
            String getVertexShaderName() const override;

            /** @copydoc MaterialPass::setVertexShaderName */
            void setVertexShaderName( const String &name ) override;

            /** @copydoc MaterialPass::getFragmentShaderName */
            String getFragmentShaderName() const override;

            /** @copydoc MaterialPass::setFragmentShaderName */
            void setFragmentShaderName( const String &name ) override;

            /** @copydoc MaterialPass::getGeometryShaderName */
            String getGeometryShaderName() const override;

            /** @copydoc MaterialPass::setGeometryShaderName */
            void setGeometryShaderName( const String &name ) override;

            /**
             * Get the underlying Ogre::Pass pointer wrapped by this object.
             * This pointer is non-owning and may be nullptr if not initialised.
             *
             * @return Pointer to the Ogre::Pass or nullptr.
             */
            Ogre::Pass *getPass() const;

            /**
             * Assign the underlying Ogre::Pass for this wrapper. The wrapper
             * will not take ownership of the pass pointer.
             *
             * @param pass Pointer to an Ogre::Pass instance (non-owning).
             */
            void setPass( Ogre::Pass *pass );

            /**
             * Retrieve the Ogre HLMS datablock associated with this pass, if
             * any. HLMS datablocks are used by Ogre to hold material
             * properties for the PBS/HLMS pipeline.
             *
             * @return Pointer to the associated HlmsDatablock, or nullptr.
             */
            Ogre::HlmsDatablock *getDataBlock() const;

            /**
             * Prepare and configure the underlying Ogre material/passes.
             * Called after textures and other properties are applied to
             * finalise the material state before rendering.
             *
             * @see MaterialPass::setupMaterial
             */
            void setupMaterial() override;

            /**
             * Handle a full state change event. This method is intended to
             * update material/pass settings when the engine state changes.
             *
             * @param state Reference to the new state object.
             * @return true if the message was handled, false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state );

            /**
             * Handle an incoming state message directed to this pass. This is
             * used for more targeted updates than a full state change.
             *
             * @param message The state message to process.
             * @return true if the message was handled, false otherwise.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * Create and configure the texture units/slots required by this
             * pass. Implementations should ensure the Ogre::Pass has the
             * necessary number of texture units allocated.
             */
            void createTextureSlots() override;

            void applyCustomShaderPrograms( Ogre::Pass *pass, const MaterialPassStateData &state );

            Ogre::CullingMode toOgreCullingMode( u32 mode );

            void applyCullingMode( Ogre::HlmsPbsDatablock *datablock, u32 mode );

            /**
             * Non-owning pointer to the underlying Ogre::Pass object that
             * this wrapper manipulates. May be nullptr when not initialised.
             */
            Ogre::Pass *m_pass = nullptr;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // CPass_h__
