#ifndef CParticleNode_h__
#define CParticleNode_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Graphics/IParticleNode.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <algorithm>

namespace workphone
{
    namespace render
    {

        template <class T>
        class CParticleNode : public T
        {
        public:
            CParticleNode();

            ~CParticleNode();

            virtual void addChild( SmartPtr<IParticleNode> child );

            virtual void addChild( SmartPtr<IParticleNode> child, int index );

            virtual void removeChild( SmartPtr<IParticleNode> child );

            virtual void remove();

            virtual u32 getNumChildren() const;

            virtual SmartPtr<IParticleNode> getChildByIndex( u32 index ) const;

            virtual SmartPtr<IParticleNode> getChildById( hash32 id ) const
            {
                return nullptr;
            }

            virtual Array<SmartPtr<IParticleNode>> getChildren() const;

            virtual SmartPtr<IParticleNode> getParent() const;

            virtual void setParent( SmartPtr<IParticleNode> parent );

            virtual void setPosition( const Vector3F &position );

            virtual Vector3F getPosition() const;

            virtual Vector3F getAbsolutePosition() const;

            virtual SmartPtr<IParticleSystem> getParticleSystem() const;

            virtual void setParticleSystem( SmartPtr<IParticleSystem> particleSystem );

            WP_CLASS_REGISTER_TEMPLATE_DECL( CParticleNode, T );

        protected:
            Array<SmartPtr<IParticleNode>> m_children;
            Vector3F m_position;
            Vector3F m_absolutePosition;
            SmartPtr<IParticleNode> m_parent;
            SmartPtr<IParticleSystem> m_particleSystem;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, CParticleNode, T, T );

        template <class T>
        CParticleNode<T>::CParticleNode()
        {
        }

        template <class T>
        CParticleNode<T>::~CParticleNode()
        {
        }

        template <class T>
        void CParticleNode<T>::addChild( SmartPtr<IParticleNode> child )
        {
            if( !child )
            {
                return;
            }

            child->setParent( this );
            child->setParticleSystem( getParticleSystem() );
            m_children.push_back( child );
        }

        template <class T>
        void CParticleNode<T>::addChild( SmartPtr<IParticleNode> child, int index )
        {
            if( !child )
            {
                return;
            }

            child->setParent( this );
            child->setParticleSystem( getParticleSystem() );

            if( index < 0 || static_cast<size_t>( index ) >= m_children.size() )
            {
                m_children.push_back( child );
                return;
            }

            m_children.insert( m_children.begin() + index, child );
        }

        template <class T>
        void CParticleNode<T>::removeChild( SmartPtr<IParticleNode> child )
        {
            if( !child )
            {
                return;
            }

            m_children.erase( std::remove( m_children.begin(), m_children.end(), child ),
                              m_children.end() );
            child->setParent( nullptr );
        }

        template <class T>
        void CParticleNode<T>::remove()
        {
            if( auto parent = getParent() )
            {
                parent->removeChild( this );
            }
        }

        template <class T>
        u32 CParticleNode<T>::getNumChildren() const
        {
            return (u32)m_children.size();
        }

        template <class T>
        SmartPtr<IParticleNode> CParticleNode<T>::getChildByIndex( u32 index ) const
        {
            if( index >= m_children.size() )
            {
                return nullptr;
            }

            return m_children[index];
        }

        template <class T>
        Array<SmartPtr<IParticleNode>> CParticleNode<T>::getChildren() const
        {
            return m_children;
        }

        template <class T>
        SmartPtr<IParticleNode> CParticleNode<T>::getParent() const
        {
            return m_parent;
        }

        template <class T>
        void CParticleNode<T>::setParent( SmartPtr<IParticleNode> parent )
        {
            m_parent = parent;
        }

        template <class T>
        void CParticleNode<T>::setPosition( const Vector3F &position )
        {
            m_position = position;
        }

        template <class T>
        Vector3F CParticleNode<T>::getPosition() const
        {
            return m_position;
        }

        template <class T>
        Vector3F CParticleNode<T>::getAbsolutePosition() const
        {
            if( auto parent = getParent() )
            {
                return parent->getAbsolutePosition() + getPosition();
            }

            return getPosition();
        }

        template <class T>
        SmartPtr<IParticleSystem> CParticleNode<T>::getParticleSystem() const
        {
            return m_particleSystem;
        }

        template <class T>
        void CParticleNode<T>::setParticleSystem( SmartPtr<IParticleSystem> particleSystem )
        {
            m_particleSystem = particleSystem;

            for( auto &child : m_children )
            {
                if( child )
                {
                    child->setParticleSystem( particleSystem );
                }
            }
        }

    }  // end namespace render
}  // namespace workphone

#endif  // CParticleNode_h__
