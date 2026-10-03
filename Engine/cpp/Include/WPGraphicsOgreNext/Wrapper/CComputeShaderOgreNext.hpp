#ifndef CComputeShaderOgreNext_h__
#define CComputeShaderOgreNext_h__

#include <Workphone/Graphics/ComputeShader.hpp>
#include <OgreHighLevelGpuProgram.h>

namespace workphone
{
    namespace render
    {

        class CComputeShaderOgreNext : public ComputeShader
        {
        public:
            CComputeShaderOgreNext();
            ~CComputeShaderOgreNext() override;

            bool compile() override;
            bool reload() override;
            void invalidate() override;

            bool createPipeline() override;
            void destroyPipeline() override;
            void getPipelineHandle( void **handle ) const override;
            void getNativeHandle( void **handle ) const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Ogre::HighLevelGpuProgramPtr m_program;

            String getOgreLanguage() const;
            String getOgreResourceName() const;
            void releaseProgram();
        };


    }  // namespace render
}  // namespace workphone

#endif // CComputeShaderOgreNext_h__
