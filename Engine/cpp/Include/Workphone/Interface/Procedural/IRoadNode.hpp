#ifndef IRoadNode_h__
#define IRoadNode_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Procedural/IProceduralNode.hpp>

namespace workphone
{
    namespace procedural
    {

        /** A node in the procedural generation system that represents a road.
         * This interface allows for the management of roads within a procedural context.
         * Acts as a handle in a spline.
         */
        class WPCore_API IRoadNode : public IProceduralNode
        {
        public:
            ~IRoadNode() override;

            virtual SmartPtr<IRoadNode> getRoadNodeFromMerged( SmartPtr<IRoad> road ) const = 0;

            virtual void addRoad( const SmartPtr<IRoad> &road ) = 0;
            virtual void removeRoad( const SmartPtr<IRoad> &road ) = 0;
            virtual Array<SmartPtr<IRoad>> getRoads() const = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // IRoadNode_h__
