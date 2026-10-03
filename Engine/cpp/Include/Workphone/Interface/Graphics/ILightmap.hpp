#ifndef ILightmap_h__
#define ILightmap_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface for a lightmap, which contains precomputed lighting information for static
         * geometry.
         */
        class WPCore_API ILightmap : public ISharedObject
        {
        public:
            /** Virtual destructor. */
            ~ILightmap() override;

            /** Get the width of the lightmap texture
             * @return Width in pixels
             */
            virtual uint32_t getWidth() const = 0;

            /** Get the height of the lightmap texture
             * @return Height in pixels
             */
            virtual uint32_t getHeight() const = 0;

            /** Get the pixel format of the lightmap
             * @return Lightmap pixel format
             */
            virtual LightmapFormat getFormat() const = 0;

            /** Get raw pixel data
             * @return Pointer to raw pixel data
             */
            virtual const void *getData() const = 0;

            /** Get the size of the data buffer in bytes
             * @return Size in bytes
             */
            virtual size_t getDataSize() const = 0;

            /** Get UV coordinates mapping
             * @param meshId Identifier for the mesh
             * @param uvCoords Output array of UV coordinates
             * @return True if UV coordinates were retrieved
             */
            virtual bool getUVMapping( const String &meshId,
                                       Array<Vector2<real_Num>> &uvCoords ) const = 0;

            /** Get the world-space bounding box covered by this lightmap
             * @return Bounding box
             */
            virtual AABB3<real_Num> getBoundingBox() const = 0;

            /** Save lightmap to file
             * @param filePath Path to save the lightmap
             * @return True if save was successful
             */
            virtual bool save( const String &filePath ) const = 0;

            /** Load lightmap from file
             * @param filePath Path to load the lightmap from
             * @return True if load was successful
             */
            virtual bool load( const String &filePath ) = 0;

            /** Check if the lightmap has valid data
             * @return True if lightmap contains valid data
             */
            bool isValid() const override = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // ILightmap_h__
