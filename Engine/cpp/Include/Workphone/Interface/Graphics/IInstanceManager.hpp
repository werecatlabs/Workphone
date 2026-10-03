#ifndef __IInstanceManager_h__
#define __IInstanceManager_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * Interface for managing instancing.
         */
        class WPCore_API IInstanceManager : public ISharedObject
        {
        public:
            /**
             * Enumerates the possible techniques for instancing.
             */
            enum InstancingTechnique
            {
                ShaderBased,        ///< Shader-based instancing, any SM 2.0+ @See InstanceBatchShader
                TextureVTF,         ///< Texture vertex fetch instancing, requires VTF and SM 3.0+ @See
                                    ///< InstanceBatchVTF
                HWInstancingBasic,  ///< Hardware instancing, requires SM 3.0+ and hardware instancing
                                    ///< support @See InstanceBatchHW
                HWInstancingVTF,    ///< Hardware instancing with texture vertex fetch, requires SM 3.0+,
                                    ///< hardware instancing support and VTF @See InstanceBatchHW_VTF
                InstancingTechniquesCount
            };

            /** Virtual destructor. */
            ~IInstanceManager() override;

            /**
             * Loads an instance manager configuration from a file.
             *
             * @param fileName The name of the file to load from.
             */
            virtual void load( const String &fileName ) = 0;

            /**
             * Sets the number of custom parameters that will be used.
             *
             * @param numCustomParams The number of custom parameters to set.
             */
            virtual void setNumCustomParams( u8 numCustomParams ) = 0;

            /**
             * Returns the number of custom parameters being used.
             *
             * @return The number of custom parameters.
             */
            virtual u8 getNumCustomParams() const = 0;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // IInstanceManager_h__
