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
            ~BlockNode() override;

            Vector3<real_Num> getPosition() const override;
            void setPosition( const Vector3<real_Num> &point ) override;
            Transform3<real_Num> getWorldTransform() const override;
            void setWorldTransform( Transform3<real_Num> transform ) override;
            String getConnectionType() const override;
            SmartPtr<ISharedObject> clone() override;
            void unload( SmartPtr<ISharedObject> data ) override;

            WP_CLASS_REGISTER_DECL;

            /// Legacy editable point; position and transform accessors read this value.
            Vector3<real_Num> m_point = Vector3<real_Num>::zero();
        };
    }  // end namespace procedural
}  // namespace workphone

#endif  // BlockNode_h__
