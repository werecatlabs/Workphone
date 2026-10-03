#ifndef IProceduralCity_h__
#define IProceduralCity_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Sphere3.hpp>
#include <Workphone/Interface/Procedural/ICityMap.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * @brief Interface representing a procedurally generated city.
         *
         * This interface provides access to the components and configuration of a
         * procedurally generated city such as the road network, city centres,
         * blocks and the city's geographic extent. Implementations are expected
         * to manage lifetime via `SmartPtr` and to be thread-aware where the
         * underlying types require it.
         *
         * @note Methods that return collections return copies of the container
         *       object (`Array<...>`) to avoid exposing internal storage details.
         */
        class WPCore_API IProceduralCity : public ISharedObject
        {
        public:
            /** @brief Virtual destructor. */
            ~IProceduralCity() override;

            /**
             * @brief Get the road network used by the city.
             * @return SmartPtr to the city's `IRoadNetwork`. May be null if no network is set.
             */
            virtual SmartPtr<IRoadNetwork> getRoadNetwork() const = 0;

            /**
             * @brief Set the road network for the city.
             * @param roadNetwork SmartPtr to an `IRoadNetwork` instance to assign.
             */
            virtual void setRoadNetwork( SmartPtr<IRoadNetwork> roadNetwork ) = 0;

            /**
             * @brief Get the list of city centres.
             * @return An `Array` of `SmartPtr<IProceduralCityCenter>` representing the city's centres.
             */
            virtual Array<SmartPtr<IProceduralCityCenter>> getCityCenters() const = 0;

            /**
             * @brief Replace the current list of city centres.
             * @param cityCenters An `Array` of `SmartPtr<IProceduralCityCenter>` to set.
             */
            virtual void setCityCenters( Array<SmartPtr<IProceduralCityCenter>> cityCenters ) = 0;

            /**
             * @brief Add a city centre.
             * @param center SmartPtr to the `IProceduralCityCenter` to add.
             */
            virtual void addCenter( SmartPtr<IProceduralCityCenter> center ) = 0;

            /**
             * @brief Remove a city centre.
             * @param center SmartPtr to the `IProceduralCityCenter` to remove.
             * @note Implementations should handle the case where `center` is not present.
             */
            virtual void removeCenter( SmartPtr<IProceduralCityCenter> center ) = 0;

            /**
             * @brief Add a city block.
             * @param block SmartPtr to the `ICityBlock` to add.
             */
            virtual void addBlock( SmartPtr<ICityBlock> block ) = 0;

            /**
             * @brief Remove a city block.
             * @param block SmartPtr to the `ICityBlock` to remove.
             * @note Implementations should handle the case where `block` is not present.
             */
            virtual void removeBlock( SmartPtr<ICityBlock> block ) = 0;

            /**
             * @brief Get the current collection of city blocks.
             * @return An `Array` of `SmartPtr<ICityBlock>` containing the blocks.
             */
            virtual Array<SmartPtr<ICityBlock>> getBlocks() const = 0;

            /**
             * @brief Get the world or local size of the city area.
             * @return A `Vector2<real_Num>` containing the width (x) and height (y).
             */
            virtual Vector2<real_Num> getSize() const = 0;

            /**
             * @brief Set the world or local size of the city area.
             * @param size `Vector2<real_Num>` where x = width and y = height.
             */
            virtual void setSize( const Vector2<real_Num> &size ) = 0;

            /**
             * @brief Convert geographic coordinates (latitude, longitude) to relative city coordinates.
             *
             * This function maps a latitude/longitude pair into the city's local 2D coordinate
             * system using the configured `minLatLong`, `maxLatLong`, and `size`. Returned
             * coordinates are typically in the same units as `getSize()`, with (0,0) typically
             * representing the minimum corner.
             *
             * @param lat_long `Vector2<real_Num>` containing (latitude, longitude).
             * @return `Vector2<real_Num>` relative coordinates inside the city's area.
             */
            virtual Vector2<real_Num> getRelativeCoordinates( const Vector2<real_Num> &lat_long ) = 0;

            /**
             * @brief Find a road by name.
             * @param name The name of the road to find.
             * @return `SmartPtr<IRoad>` for the road if found; otherwise a null SmartPtr.
             */
            virtual SmartPtr<IRoad> getRoadByName( const String &name ) = 0;

            /**
             * @brief Get all roads in the city.
             * @return An `Array` of `SmartPtr<IRoad>` representing the roads.
             */
            virtual Array<SmartPtr<IRoad>> getRoads() const = 0;

            /**
             * @brief Get the city's area map.
             * @return SmartPtr to an `ICityMap` describing area attributes (zones, elevation, etc.).
             */
            virtual SmartPtr<ICityMap> getArea() const = 0;

            /**
             * @brief Set the city's area map.
             * @param area SmartPtr to an `ICityMap` instance.
             */
            virtual void setArea( SmartPtr<ICityMap> area ) = 0;

            /**
             * @brief Determine whether a sphere intersects or is contained within the city's bounds.
             * @param sphere The `Sphere3F` to test (world-space).
             * @return `true` if the sphere is at least partially within the city bounds; otherwise
             * `false`.
             *
             * @note Implementations may use an internal bounding volume or test blocks/roads as needed.
             */
            virtual bool isWithin( const Sphere3F &sphere ) const = 0;

            /**
             * @brief Get the minimum geographic latitude/longitude for the city.
             * @return `Vector2<real_Num>` containing minimum (latitude, longitude).
             */
            virtual Vector2<real_Num> getMinLatLong() const = 0;

            /**
             * @brief Set the minimum geographic latitude/longitude for the city.
             * @param minLatLong `Vector2<real_Num>` containing minimum (latitude, longitude).
             */
            virtual void setMinLatLong( const Vector2<real_Num> &minLatLong ) = 0;

            /**
             * @brief Get the maximum geographic latitude/longitude for the city.
             * @return `Vector2<real_Num>` containing maximum (latitude, longitude).
             */
            virtual Vector2<real_Num> getMaxLatLong() const = 0;

            /**
             * @brief Set the maximum geographic latitude/longitude for the city.
             * @param maxLatLong `Vector2<real_Num>` containing maximum (latitude, longitude).
             */
            virtual void setMaxLatLong( const Vector2<real_Num> &maxLatLong ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IProceduralCity_h__
