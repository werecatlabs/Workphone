#ifndef IRoadElement_h__
#define IRoadElement_h__

#include <Workphone/Interface/Procedural/IProceduralObject.hpp>
#include <Workphone/Interface/Procedural/IRoad.hpp>

namespace workphone
{
    namespace procedural
    {
        class WPCore_API IRoadElement : public IProceduralObject
        {
        public:
            ~IRoadElement() override;

            virtual SmartPtr<IRoadSection> getParentSection() const = 0;
            virtual void setParentSection( SmartPtr<IRoadSection> parentSection ) = 0;

            virtual Array<SmartPtr<IRoadNode>> getRoadNodes() const = 0;

            virtual Array<SmartPtr<IRoadNode>> getSidewalks() const = 0;
            virtual void setSidewalks( Array<SmartPtr<IRoadNode>> sidewalks ) = 0;

            virtual IRoad::RoadType getRoadType() const = 0;
            virtual void setRoadType( IRoad::RoadType roadType ) = 0;

            virtual IRoad::LaneType getLaneType() const = 0;
            virtual void setLaneType( IRoad::LaneType laneType ) = 0;

            virtual bool isLit() const = 0;
            virtual void setIsLit( bool isLit ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IRoadElement_h__
