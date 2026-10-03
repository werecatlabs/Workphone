#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxMaterial.hpp>
#include <Workphone/Workphone.hpp>
#include <PxMaterial.h>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysxMaterial, PhysicsMaterial3 );

    PhysxMaterial::PhysxMaterial() = default;

    PhysxMaterial::~PhysxMaterial() = default;

    auto PhysxMaterial::getMaterial() const -> RawPtr<physx::PxMaterial>
    {
        return m_material;
    }

    void PhysxMaterial::setMaterial( RawPtr<physx::PxMaterial> material )
    {
        m_material = material;
    }

    auto PhysxMaterial::getProperties() const -> SmartPtr<Properties>
    {
        try
        {
            auto properties = workphone::make_ptr<Properties>();

            if( m_material )
            {
                auto dynamicFriction = m_material->getDynamicFriction();
                properties->setProperty( "dynamicFriction", dynamicFriction );

                auto staticFriction = m_material->getStaticFriction();
                properties->setProperty( "staticFriction", staticFriction );
            }

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void PhysxMaterial::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            if( m_material )
            {
                auto dynamicFriction = m_material->getDynamicFriction();
                properties->getPropertyValue( "dynamicFriction", dynamicFriction );
                m_material->setDynamicFriction( dynamicFriction );

                auto staticFriction = m_material->getStaticFriction();
                properties->getPropertyValue( "staticFriction", staticFriction );
                m_material->setStaticFriction( staticFriction );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }
} // namespace workphone::physics
