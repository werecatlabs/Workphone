#ifndef BlockNode_h__
#define BlockNode_h__

#include <WPProcedural/WPProceduralPrerequisites.hpp>
#include <WPProcedural/CProceduralNode.hpp>

namespace workphone
{
    namespace procedural
    {

        /* Used to represent points in the block. Let users control the points. */
        class WPProcedural_API BlockNode : public CProceduralNode<IProceduralNode>
        {
        public:
            BlockNode();
            ~BlockNode();

            Vector3<real_Num> m_point;
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // BlockNode_h__
