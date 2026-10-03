#ifndef ITerrainBlendMap_h__
#define ITerrainBlendMap_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Abstract interface for a terrain blend map.
         */
        class WPCore_API ITerrainBlendMap : public ISharedObject
        {
        public:
            /**
             * @brief Virtual destructor.
             */
            ~ITerrainBlendMap() override;

            /**
             * @brief Loads an image to use as a blend map.
             * @param fileName The name of the image file.
             * @param path The path to the image file.
             */
            virtual void loadImage( const String &fileName,
                                    const String &path = StringUtil::EmptyString ) = 0;

            /**
             * @brief Saves the blend map image to disk.
             * @param fileName The name of the image file.
             * @param path The path to the image file.
             */
            virtual void saveImage( const String &fileName,
                                    const String &path = StringUtil::EmptyString ) = 0;

            /**
             * @brief Gets the blend value at a single point.
             * @param x The x-coordinate of the point in image space.
             * @param y The y-coordinate of the point in image space.
             * @return The blend data at the given point.
             */
            virtual f32 getBlendValue( u32 x, u32 y ) = 0;

            /**
             * @brief Sets the blend value at a single point.
             * @param x The x-coordinate of the point in image space.
             * @param y The y-coordinate of the point in image space.
             * @param blendValue The blend value to set (0..1).
             */
            virtual void setBlendValue( u32 x, u32 y, f32 blendValue ) = 0;

            /**
             * @brief Gets the size of the blend map.
             * @return The size of the blend map.
             */
            virtual u32 getSize() const = 0;

            /**
             * @brief Gets the owner of the blend map.
             * @return The owner of the blend map.
             */
            virtual SmartPtr<IGraphicsTerrain> getOwner() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // ITerrainBlendMap_h__
