#ifndef CParticleNode_h__
#define CParticleNode_h__

#include <Workphone/Interface/Graphics/IParticleNode.hpp>

namespace workphone
{
    namespace render
    {

        template <class T>
        class CParticleNode : public T
        {
        public:
            CParticleNode()
            {
            }

            ~CParticleNode()
            {
            }

            virtual void update()
            {
            }

            virtual void addChild( SmartPtr<IParticleNode> child )
            {
                m_children.push_back( child );
            }

            virtual void addChild( SmartPtr<IParticleNode> child, s32 index )
            {
                m_children[index] = child;
            }

            virtual void removeChild( SmartPtr<IParticleNode> child )
            {
                m_children.erase( std::remove( m_children.begin(), m_children.end(), child ),
                                  m_children.end() );

                child->setParent( nullptr );
            }

            virtual void remove()
            {
                if( m_parent )
                {
                    m_parent->removeChild( this );
                }
            }

            virtual u32 getNumChildren() const
            {
                return m_children.size();
            }

            virtual SmartPtr<IParticleNode> getChildByIndex( u32 index ) const
            {
                return m_children[index];
            }

            virtual SmartPtr<IParticleNode> getChildById( hash32 id ) const
            {
                for( auto child : m_children )
                {
                    auto handle = child->getHandle();
                    auto childId = handle->getId();
                    if( childId == id )
                    {
                        return child;
                    }
                }

                return nullptr;
            }

            virtual Array<SmartPtr<IParticleNode>> getChildren() const
            {
                return m_children;
            }

            virtual SmartPtr<IParticleNode> getParent() const
            {
                return m_parent;
            }

            virtual void setParent( SmartPtr<IParticleNode> parent )
            {
                m_parent = parent;
            }

            virtual void setPosition( const Vector3F &position )
            {
                m_position = position;
            }

            virtual Vector3F getPosition() const
            {
                return m_position;
            }

            virtual Vector3F getAbsolutePosition() const
            {
                return m_absolutePosition;
            }

            virtual SmartPtr<IParticleSystem> getParticleSystem() const
            {
                return m_particleSystem;
            }

            virtual void setParticleSystem( SmartPtr<IParticleSystem> particleSystem )
            {
                m_particleSystem = particleSystem;
            }

        protected:
            Array<SmartPtr<IParticleNode>> m_children;
            Vector3F m_position;
            Vector3F m_absolutePosition;
            SmartPtr<IParticleNode> m_parent;
            SmartPtr<IParticleSystem> m_particleSystem;
        };

    }  // end namespace render
}  // namespace workphone

#endif  // CParticleNode_h__
