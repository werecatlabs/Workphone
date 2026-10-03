#ifndef ILightmapper_h__
#define ILightmapper_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Core/ColourF.hpp>

namespace workphone
{
    namespace render
    {

        /** Lightmapper configuration settings */
        struct LightmapperSettings
        {
            /** Resolution of the lightmap texture */
            uint32_t resolution = 512;

            /** Number of samples per pixel for quality */
            uint32_t samplesPerPixel = 16;

            /** Maximum number of light bounces for indirect illumination */
            uint32_t maxBounces = 3;

            /** Enable ambient occlusion */
            bool enableAO = true;

            /** Ambient occlusion distance */
            float aoDistance = 1.0f;

            /** Enable shadows */
            bool enableShadows = true;

            /** Gamma correction value */
            float gamma = 2.2f;

            /** Texture padding in pixels */
            uint32_t padding = 2;
        };

        /** Lightmapping progress callback */
        class ILightmapperCallback
        {
        public:
            virtual ~ILightmapperCallback() = default;

            /** Called when progress is updated
             * @param progress Progress value between 0.0 and 1.0
             * @param message Status message
             */
            virtual void onProgress( float progress, const String &message ) = 0;

            /** Called when lightmapping is complete */
            virtual void onComplete() = 0;

            /** Called when an error occurs
             * @param error Error message
             */
            virtual void onError( const String &error ) = 0;
        };

        class WPCore_API ILightmapper : public ISharedObject
        {
        public:
            /** Virtual destructor. */
            ~ILightmapper() override;

            /** Initialize the lightmapper
             * @return True if initialization was successful
             */
            virtual bool initialize() = 0;

            /** Set lightmapper configuration settings
             * @param settings The settings to apply
             */
            virtual void setSettings( const LightmapperSettings &settings ) = 0;

            /** Get current lightmapper settings
             * @return Current settings
             */
            virtual LightmapperSettings getSettings() const = 0;

            /** Set progress callback
             * @param callback Callback interface for progress updates
             */
            virtual void setCallback( ILightmapperCallback *callback ) = 0;

            /** Bake lightmaps for a scene
             * @param scene The scene to bake
             * @return Shared pointer to the generated lightmap
             */
            virtual SmartPtr<ILightmap> bakeScene( SmartPtr<IGraphicsScene> scene ) = 0;

            /** Bake lightmap for a specific mesh
             * @param mesh The mesh to bake
             * @param lights Array of lights to consider
             * @return Shared pointer to the generated lightmap
             */
            virtual SmartPtr<ILightmap> bakeMesh( SmartPtr<IMesh> mesh,
                                                  const Array<SmartPtr<IGraphicsLight>> &lights ) = 0;

            /** Check if lightmapping is currently in progress
             * @return True if baking is in progress
             */
            virtual bool isBaking() const = 0;

            /** Cancel current baking operation */
            virtual void cancel() = 0;

            /** Generate UV coordinates for lightmapping if mesh doesn't have them
             * @param mesh The mesh to generate UVs for
             * @return True if UV generation was successful
             */
            virtual bool generateLightmapUVs( SmartPtr<IMesh> mesh ) = 0;

            /** Save lightmap to file
             * @param lightmap The lightmap to save
             * @param filePath Path to save the lightmap
             * @return True if save was successful
             */
            virtual bool saveLightmap( SmartPtr<ILightmap> lightmap, const String &filePath ) = 0;

            /** Load lightmap from file
             * @param filePath Path to the lightmap file
             * @return Loaded lightmap or nullptr on failure
             */
            virtual SmartPtr<ILightmap> loadLightmap( const String &filePath ) = 0;

            /** Clear cached data and reset state */
            virtual void reset() = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // ILightmapper_h__
