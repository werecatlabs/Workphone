#ifndef IProceduralCityCenter_h__
#define IProceduralCityCenter_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/IProceduralObject.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Math/Sphere3.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/Grid2.hpp>
#include <Workphone/Core/Handle.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Interface/Procedural/IRoadNode.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPCore_API IProceduralCityCenter : public IProceduralObject
        {
        public:
            ~IProceduralCityCenter() override;

            // Transform
            virtual Transform3<real_Num> getTransform() const = 0;
            virtual void setTransform( Transform3<real_Num> transform ) = 0;

            // Name
            String getName() const override = 0;
            void setName( const String &name ) override = 0;

            // Radius
            virtual f32 getRadius() const = 0;
            virtual void setRadius( f32 radius ) = 0;

            // Roads (by handle)
            virtual void addRoad( Handle handle ) = 0;
            virtual void removeRoad( Handle handle ) = 0;
            virtual Array<Handle> &getRoads() = 0;
            virtual const Array<Handle> &getRoads() const = 0;
            virtual void setRoads( Array<Handle> roads ) = 0;

            // Grid
            virtual Grid2 &getRoadNodeGrid() = 0;
            virtual const Grid2 &getRoadNodeGrid() const = 0;
            virtual void setRoadNodeGrid( Grid2 roadNodeGrid ) = 0;

            // Road nodes from cell
            virtual Array<SmartPtr<IRoadNode>> getRoadNodesFromCell( const Vector2I &cellIndex ) = 0;

            // Properties
            SmartPtr<Properties> getProperties() const override = 0;
            void setProperties( SmartPtr<Properties> properties ) override = 0;

            // Clone
            SmartPtr<ISharedObject> clone() override = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IProceduralCityCenter_h__
