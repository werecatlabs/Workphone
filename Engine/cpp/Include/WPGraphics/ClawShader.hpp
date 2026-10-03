#ifndef __ClawShader_h__
#define __ClawShader_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/Shader.hpp>

struct wp_shader;

namespace workphone
{
    namespace render
    {
        /**
         * @class ClawShader
         * @brief Production-ready shader implementation bridging the engine's Shader base class
         *        with the C89 wp_shader C API.
         *
         * ClawShader wraps a low-level @c wp_shader handle and exposes shader configuration
         * (stage, language, source, entry point, profile, defines, compile status) to the
         * game editor through a data-driven @c getProperties / @c setProperties implementation.
         *
         * The C89 object is the canonical backend-facing descriptor and is owned by this
         * wrapper. Programs retain attached descriptors, so their lifetime remains safe
         * when the high-level wrapper is released.
         *
         * @see Shader
         * @see IShader
         * @see wp_shader
         */
        class WPGraphics_API ClawShader : public Shader
        {
        public:
            /** @brief Property name for the shader stage enum. */
            static const String stageStr;
            /** @brief Property name for the shader language enum. */
            static const String languageStr;
            /** @brief Property name for the shader source code string. */
            static const String sourceStr;
            /** @brief Property name for the shader entry point string. */
            static const String entryPointStr;
            /** @brief Property name for the shader target profile string. */
            static const String profileStr;
            /** @brief Property name for the shader compile status enum. */
            static const String statusStr;
            /** @brief Property name for the shader defines child group. */
            static const String definesStr;

            /**
             * @brief Default constructor.
             *
             * Creates an owned C89 shader descriptor with deterministic empty state.
             */
            ClawShader();

            /**
             * @brief Destructor.
             *
             * Releases the owned descriptor. Attached C programs retain their own reference.
             */
            ~ClawShader() override;

            ClawShader( const ClawShader & ) = delete;
            ClawShader &operator=( const ClawShader & ) = delete;

            Stage getStage() const override;
            void setStage( Stage stage ) override;
            Language getLanguage() const override;
            void setLanguage( Language language ) override;
            String getSource() const override;
            void setSource( const String &source ) override;
            String getEntryPoint() const override;
            void setEntryPoint( const String &entryPoint ) override;
            String getProfile() const override;
            void setProfile( const String &profile ) override;
            Array<Define> getDefines() const override;
            void setDefines( const Array<Define> &defines ) override;
            void setDefine( const String &name, const String &value = String() ) override;
            void removeDefine( const String &name ) override;
            void clearDefines() override;
            bool compile() override;
            bool reload() override;
            void invalidate() override;
            Status getStatus() const override;
            bool isCompiled() const override;
            Array<Diagnostic> getDiagnostics() const override;
            void clearDiagnostics() override;
            Array<u8> getBinary() const override;
            bool setBinary( const Array<u8> &binary, Language language ) override;
            bool setSpecializationConstant( const String &name, u32 value ) override;
            bool getSpecializationConstant( const String &name, u32 &value ) const override;

            /**
             * @brief Retrieves shader properties for the game editor.
             *
             * Calls the base class @c getProperties first, then augments the
             * result with shader-specific properties (stage, language, source,
             * entry point, profile, status, defines).
             *
             * @return Smart pointer to a Properties object, or null on failure.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Applies editor-supplied properties to the shader.
             *
             * Reads property values from the supplied Properties object and
             * pushes them to the base class setters. Missing properties are
             * silently skipped.
             *
             * @param properties Smart pointer to a Properties object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Returns an opaque backend handle.
             *
             * @param handle Out parameter that receives the compiled backend object, if any.
             */
            void getNativeHandle( void **handle ) const override;

            /**
             * @brief Retrieves the native C shader handle.
             *
             * @return Borrowed pointer to the retained @c wp_shader descriptor.
             */
            wp_shader *getNativeShader() const;

            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            wp_shader *m_shader;  ///< Owned underlying Claw shader handle.
        };
    }  // namespace render
}  // namespace workphone

#endif  // __ClawShader_h__
