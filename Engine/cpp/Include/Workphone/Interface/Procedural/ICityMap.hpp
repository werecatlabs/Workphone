#ifndef ICityMap_h__
#define ICityMap_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <map>

namespace workphone
{
    namespace procedural
    {
        class WPCore_API ICityMap : public ISharedObject
        {
        public:
            ~ICityMap() override;

            /**
             * @brief Sets a value in the city map at the given index.
             * @param index The coordinate in the map.
             * @param value The value to set.
             * @param addMapPoint Whether to add the point if it does not exist.
             */
            virtual void setCityMapValue( const Vector2F &index, bool value,
                                          bool addMapPoint = false ) = 0;

            /**
             * @brief Gets the value in the city map at the given index.
             * @param index The coordinate in the map.
             * @return The value at the coordinate.
             */
            virtual bool getCityMapValue( const Vector2F &index ) const = 0;

            /**
             * @brief Checks if the city map has an entry at the given index.
             * @param index The coordinate in the map.
             * @return True if the entry exists, false otherwise.
             */
            virtual bool hasCityMapEntry( const Vector2F &index ) const = 0;

            /**
             * @brief Gets the axis-aligned bounding box of the map.
             * @return The bounding box.
             */
            virtual AABB3F getMapAABB() const = 0;

            /**
             * @brief Gets the entire city map as a map of coordinates to values.
             * @return The city map.
             */
            virtual std::map<Vector2F, bool> getCityMap() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // ICityMap_h__
