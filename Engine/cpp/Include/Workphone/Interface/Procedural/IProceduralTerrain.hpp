#ifndef IProceduralTerrain_h__
#define IProceduralTerrain_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/IProceduralObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief Interface for procedural terrain objects.
         *
         * This interface defines the contract for terrain objects that can be procedurally generated or
         * manipulated. It provides methods for configuring terrain size, resolution, and height data.
         */
        class WPCore_API IProceduralTerrain : public IProceduralObject
        {
        public:
            /**
             * @brief Virtual destructor.
             */
            ~IProceduralTerrain() override;

            /**
             * @brief Gets the size of the terrain in world units.
             *
             * @return The size of the terrain as a 3D vector (width, height, depth).
             */
            virtual Vector3<real_Num> getSize() const = 0;

            /**
             * @brief Sets the size of the terrain in world units.
             *
             * @param size The new size of the terrain as a 3D vector (width, height, depth).
             */
            virtual void setSize( const Vector3<real_Num> &size ) = 0;

            /**
             * @brief Gets the resolution of the heightmap.
             *
             * The heightmap resolution determines the number of samples used to represent terrain
             * elevation.
             *
             * @return The heightmap resolution as a 2D integer vector (width, height).
             */
            virtual Vector2I getHeightmapResolution() const = 0;

            /**
             * @brief Sets the resolution of the heightmap.
             *
             * @param resolution The new heightmap resolution as a 2D integer vector (width, height).
             */
            virtual void setHeightmapResolution( const Vector2I &resolution ) = 0;

            /**
             * @brief Gets the resolution of the alphamap.
             *
             * The alphamap resolution determines the number of samples used for texture blending
             * (splatting).
             *
             * @return The alphamap resolution as a 2D integer vector (width, height).
             */
            virtual Vector2I getAlphamapResolution() const = 0;

            /**
             * @brief Sets the resolution of the alphamap.
             *
             * @param resolution The new alphamap resolution as a 2D integer vector (width, height).
             */
            virtual void setAlphamapResolution( const Vector2I &resolution ) = 0;

            /**
             * @brief Gets the detail map resolution.
             *
             * The detail map resolution determines the number of samples used for fine surface details
             * (e.g., grass, small rocks).
             *
             * @return The detail map resolution as a 2D integer vector (width, height).
             */
            virtual Vector2I getDetailResolution() const = 0;

            /**
             * @brief Sets the detail map resolution.
             *
             * @param resolution The new detail map resolution as a 2D integer vector (width, height).
             */
            virtual void setDetailResolution( const Vector2I &resolution ) = 0;

            /**
             * @brief Gets the number of detail resolution samples per patch.
             *
             * This value determines the granularity of detail patches within the terrain.
             *
             * @return The number of detail resolution samples per patch.
             */
            virtual s32 getDetailResolutionPerPatch() const = 0;

            /**
             * @brief Sets the number of detail resolution samples per patch.
             *
             * @param resolutionPerPatch The new number of detail resolution samples per patch.
             */
            virtual void setDetailResolutionPerPatch( s32 resolutionPerPatch ) = 0;

            /**
             * @brief Gets the height data for the terrain.
             *
             * The height data is typically a flat array of floating-point values representing elevation
             * at each heightmap sample.
             *
             * @return An array of height values (elevation samples).
             */
            virtual Array<f32> getHeightData() const = 0;

            /**
             * @brief Sets the height data for the terrain.
             *
             * @param heightData An array of height values (elevation samples) to set for the terrain.
             */
            virtual void setHeightData( const Array<f32> &heightData ) = 0;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IProceduralTerrain_h__
