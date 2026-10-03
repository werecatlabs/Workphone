#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/ParticleAffector.hpp>
#include <Workphone/Interface/Graphics/IParticle.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/Interface/Graphics/IParticleTechnique.hpp>
#include <Workphone/Math/Math.hpp>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone, ParticleAffector, IParticleAffector );

        ParticleAffector::ParticleAffector()
        {
        }

        ParticleAffector::~ParticleAffector()
        {
        }

        void ParticleAffector::calculateState( SmartPtr<IParticle> &particle, u32 stateIndex,
                                               void *data )
        {
            if( !particle )
            {
                return;
            }

            // Base affector implementation - can be overridden by derived classes
            // This provides a no-op base that derived affectors can specialize

            // Common affector operations that derived classes might implement:
            // - Gravity: Apply constant downward force
            // - Wind: Apply directional force
            // - Color: Interpolate particle color over time
            // - Scale: Scale particle over lifetime
            // - Rotation: Rotate particle over time
            // - Attraction: Pull particles toward a point
            // - Repulsion: Push particles away from a point
            // - Vortex: Create swirling motion
            // - LinearForce: Apply constant force vector
            // - Drag: Apply velocity damping
            // - Randomizer: Add random variations
            // - Collision: Handle particle collisions
            // - Fade: Fade particle alpha over time

            // Derived classes should override this method to implement specific behaviors
            // The data parameter can be used to pass affector-specific configuration
        }

        void ParticleAffector::addChild( SmartPtr<IParticleNode> child )
        {
            if( child )
            {
                m_children.push_back( child );
                child->setParent( this );
            }
        }

        void ParticleAffector::addChild( SmartPtr<IParticleNode> child, s32 index )
        {
            if( !child )
            {
                return;
            }

            if( index < 0 || index >= static_cast<s32>( m_children.size() ) )
            {
                m_children.push_back( child );
            }
            else
            {
                m_children.insert( m_children.begin() + index, child );
            }
            child->setParent( this );
        }

        void ParticleAffector::removeChild( SmartPtr<IParticleNode> child )
        {
            if( !child )
            {
                return;
            }

            m_children.erase( std::remove( m_children.begin(), m_children.end(), child ),
                              m_children.end() );
            child->setParent( nullptr );
        }

        void ParticleAffector::remove()
        {
            if( auto parent = getParent() )
            {
                parent->removeChild( this );
            }
        }

        u32 ParticleAffector::getNumChildren() const
        {
            return static_cast<u32>( m_children.size() );
        }

        SmartPtr<IParticleNode> ParticleAffector::getChildByIndex( u32 index ) const
        {
            if( index < m_children.size() )
            {
                return m_children[index];
            }
            return nullptr;
        }

        SmartPtr<IParticleNode> ParticleAffector::getChildById( hash32 id ) const
        {
            for( const auto &child : m_children )
            {
                if( child && StringUtil::getHash( child->getName() ) == id )
                {
                    return child;
                }
            }
            return nullptr;
        }

        Array<SmartPtr<IParticleNode>> ParticleAffector::getChildren() const
        {
            return m_children;
        }

        SmartPtr<IParticleNode> ParticleAffector::getParent() const
        {
            return m_parent.lock();
        }

        void ParticleAffector::setParent( SmartPtr<IParticleNode> parent )
        {
            m_parent = parent;
        }

        void ParticleAffector::setPosition( const Vector3<real_Num> &position )
        {
            m_position = position;
        }

        Vector3<real_Num> ParticleAffector::getPosition() const
        {
            return m_position;
        }

        Vector3<real_Num> ParticleAffector::getAbsolutePosition() const
        {
            Vector3<real_Num> absPos = m_position;
            auto parent = getParent();
            if( parent )
            {
                absPos += parent->getAbsolutePosition();
            }
            return absPos;
        }

        SmartPtr<IParticleSystem> ParticleAffector::getParticleSystem() const
        {
            auto technique = getTechnique();
            if( technique )
            {
                return technique->getParticleSystem();
            }
            return nullptr;
        }

        void ParticleAffector::setParticleSystem( SmartPtr<IParticleSystem> particleSystem )
        {
            m_particleSystem = particleSystem;
        }

        SmartPtr<IParticleTechnique> ParticleAffector::getTechnique() const
        {
            auto system = m_particleSystem.lock();
            if( system )
            {
                // Find the technique that contains this affector
                for( size_t i = 0; i < system->getNumTechniques(); ++i )
                {
                    auto technique = system->getTechnique( String( "Technique" ) + std::to_string( i ) );
                    if( technique )
                    {
                        const auto &affectors = technique->getParticleAffectors();
                        for( const auto &affector : affectors )
                        {
                            if( affector.get() == this )
                            {
                                return technique;
                            }
                        }
                    }
                }
            }
            return nullptr;
        }

    }  // namespace render
}  // namespace workphone
