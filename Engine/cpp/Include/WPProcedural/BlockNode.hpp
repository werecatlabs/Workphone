#ifndef BlockNode_h__
#define BlockNode_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <WPProcedural/CProceduralNode.hpp>

namespace workphone
{
    namespace procedural
    {

        class WPProcedural_API BlockNode : public CProceduralNode<IProceduralNode>
        {
        public:
            BlockNode();
            ~BlockNode();
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // BlockNode_h__
