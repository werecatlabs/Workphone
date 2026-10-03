#ifndef ITerrainRayResult_h__
#define ITerrainRayResult_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface for a result of a terrain ray intersection test.
         */
        class WPCore_API ITerrainRayResult : public ISharedObject
        {
        public:
            /** Virtual destructor. */
            ~ITerrainRayResult() override = default;

            /**
             * @brief Checks whether the ray intersected with the terrain or not.
             *
             * @return True if the ray intersected with the terrain, false otherwise.
             */
            virtual bool hasIntersected() const = 0;

            /**
             * @brief Sets the flag indicating whether the ray intersected with the terrain or not.
             *
             * @param intersected True if the ray intersected with the terrain, false otherwise.
             */
            virtual void setIntersected( bool intersected ) = 0;

            /**
             * @brief Gets the terrain that was intersected.
             *
             * @return A shared pointer to the terrain that was intersected.
             */
            virtual SmartPtr<IGraphicsTerrain> getTerrain() const = 0;

            /**
             * @brief Sets the terrain that was intersected.
             *
             * @param terrain A shared pointer to the terrain that was intersected.
             */
            virtual void setTerrain( SmartPtr<IGraphicsTerrain> terrain ) = 0;

            /**
             * @brief Gets the position of the intersection point in world space.
             *
             * @return A vector representing the position of the intersection point.
             */
            virtual Vector3<real_Num> getPosition() const = 0;

            /**
             * @brief Sets the position of the intersection point in world space.
             *
             * @param position A vector representing the position of the intersection point.
             */
            virtual void setPosition( const Vector3<real_Num> &position ) = 0;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // ITerrainRayResult_h__
