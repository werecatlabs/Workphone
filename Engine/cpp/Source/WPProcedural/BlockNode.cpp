#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/BlockNode.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace procedural
    {
        WP_CLASS_REGISTER_DERIVED( workphone, BlockNode, CProceduralNode<IProceduralNode> );

        BlockNode::BlockNode() = default;
        BlockNode::~BlockNode() = default;

        Vector3<real_Num> BlockNode::getPosition() const
        {
            return m_point;
        }

        void BlockNode::setPosition( const Vector3<real_Num> &point )
        {
            m_point = point;
            m_worldTransform.setPosition( point );
        }

        Transform3<real_Num> BlockNode::getWorldTransform() const
        {
            auto transform = m_worldTransform;
            transform.setPosition( m_point );
            return transform;
        }

        void BlockNode::setWorldTransform( Transform3<real_Num> transform )
        {
            m_worldTransform = transform;
            m_point = transform.getPosition();
        }

        String BlockNode::getConnectionType() const
        {
            return "BlockNode";
        }

        SmartPtr<ISharedObject> BlockNode::clone()
        {
            auto node = workphone::make_ptr<BlockNode>();
            node->setName( getName() );
            node->setWorldTransform( getWorldTransform() );
            node->setGraphId( getGraphId() );
            node->setRoadId( getRoadId() );
            node->setIsConnection( isConnection() );
            // Topology and scene ownership are intentionally detached in a point clone.
            if( m_properties )
            {
                *node->m_properties = *m_properties;
            }
            return node;
        }

        void BlockNode::unload( SmartPtr<ISharedObject> data )
        {
            CProceduralNode<IProceduralNode>::unload( data );
        }
    }  // namespace procedural
}  // namespace workphone
