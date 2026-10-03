#ifndef ILot_h__
#define ILot_h__

#include <Workphone/Interface/Procedural/IProceduralObject.hpp>
#include <Workphone/Interface/Procedural/ICityBlock.hpp>
#include <Workphone/Interface/Procedural/IProceduralNode.hpp>
#include <Workphone/Math/Polygon2.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    namespace procedural
    {
        /**
         * Interface for a lot in the procedural system.
         * A lot can represent a piece of land or a specific area within a city.
         */
        class WPCore_API ILot : public IProceduralObject
        {
        public:
            ~ILot() override;

            /** Get the parent block of this lot. */
            virtual SmartPtr<ICityBlock> getBlock() const = 0;

            /** Set the parent block of this lot. */
            virtual void setBlock( SmartPtr<ICityBlock> block ) = 0;

            /** Get the nodes (corners) of this lot. */
            Array<SmartPtr<IProceduralNode>> getNodes() const override = 0;

            /** Set the nodes (corners) of this lot. */
            virtual void setNodes( Array<SmartPtr<IProceduralNode>> nodes ) = 0;

            /** Get the polygon representing this lot. */
            virtual Polygon2<real_Num> getPolygon() const = 0;

            /** Get the area of this lot. */
            virtual real_Num getArea() const = 0;

            /** Get the axis-aligned bounding box of this lot. */
            virtual AABB3F getBounds() const = 0;

            /** Build the lot geometry or data. */
            void build() override = 0;

            /** Update the bounds of the lot. */
            void updateBounds() override = 0;

            // Optional: Update the position of the lot's object (if applicable)
            // virtual void updatePosition() = 0;

            // Optional: Update the rotation of the lot's object (if applicable)
            // virtual void updateRotation() = 0;

            // Optional: Find the nearest road to this lot
            // virtual SmartPtr<IRoad> findNearestRoad() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace procedural
}  // namespace workphone

#endif  // ILot_h__
