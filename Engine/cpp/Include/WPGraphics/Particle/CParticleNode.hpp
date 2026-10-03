#ifndef CParticleNode_h__
#define CParticleNode_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Core/Handle.hpp>
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

            ~CParticleNode() override
            {
            }

            virtual void initialise( SmartPtr<IBuildDirector> objectTemplate )
            {
            }

            virtual void initialise( SmartPtr<IBuildDirector> objectTemplate,
                                     SmartPtr<Properties> instanceProperties )
            {
            }

            void update() override
            {
            }

            virtual void addChild( SmartPtr<IParticleNode> child )
            {
                m_children.push_back( child );
            }

            virtual void addChild( SmartPtr<IParticleNode> child, int index )
            {
                m_children[index] = child;
            }

            virtual void removeChild( SmartPtr<IParticleNode> child )
            {
                // m_children.erase(child);
                child->setParent( nullptr );
            }

            virtual void remove()
            {
                // if (m_parent)
                //{
                //	m_parent->removeChild(this);
                // }
            }

            virtual u32 getNumChildren() const
            {
                return static_cast<u32>( m_children.size() );
            }

            virtual SmartPtr<IParticleNode> getChildByIndex( u32 index ) const
            {
                return m_children[index];
            }

            virtual SmartPtr<IParticleNode> getChildById( hash32 id ) const
            {
                static SmartPtr<IParticleNode> pNullPtr;
                return pNullPtr;
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

            virtual void setPosition( const Vector3<real_Num> &position )
            {
                m_position = position;
            }

            virtual Vector3<real_Num> getPosition() const
            {
                return m_position;
            }

            virtual Vector3<real_Num> getAbsolutePosition() const
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
            Vector3<real_Num> m_position;
            Vector3<real_Num> m_absolutePosition;
            SmartPtr<IParticleNode> m_parent;
            SmartPtr<IParticleSystem> m_particleSystem;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CParticleNode_h__
