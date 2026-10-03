#ifndef __WP_CShaderOgreNext_hpp__
#define __WP_CShaderOgreNext_hpp__

#include <Workphone/Graphics/Shader.hpp>
#include <OgreHighLevelGpuProgram.h>

namespace workphone
{
    namespace render
    {

        /**
             * @class CShaderOgreNext
             * @brief OgreNext implementation of the Shader class.
             *
             * This class wraps an Ogre::HighLevelGpuProgram to provide shader functionality 
             * within the workphone render engine.
             */
            class CShaderOgreNext : public Shader
        {
        public:
            CShaderOgreNext();
            ~CShaderOgreNext() override;

            /** @brief Compiles the shader program. @return True if compilation succeeded. */
            bool compile() override;
            /** @brief Reloads the shader from source. @return True if reload succeeded. */
            bool reload() override;
            /** @brief Invalidates the current shader state. */
            void invalidate() override;
            /** @brief Retrieves the native Ogre handle. @param handle Pointer to store the native handle. */
            void getNativeHandle( void **handle ) const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Ogre::HighLevelGpuProgramPtr m_program; ///< Ogre high-level GPU program pointer.

            /** @brief Returns the Ogre-specific language string. */
            String getOgreLanguage() const;
            /** @brief Returns the Ogre GPU program type. */
            Ogre::GpuProgramType getOgreProgramType() const;
            /** @brief Checks if the current shader configuration is supported by Ogre. */
            bool isSupportedByOgre() const;
            /** @brief Returns the resource name used by Ogre. */
            String getOgreResourceName() const;
            /** @brief Releases the underlying Ogre program. */
            void releaseProgram();
        };

    }  // namespace render
}  // namespace workphone

#endif  // __WP_CShaderOgreNext_hpp__
