#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CInstanceManagerOgreNext.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone::render
{
    namespace
    {
        const String instanceManagerNamePropertyStr = "instanceManagerName";
        const String instanceManagerMeshNamePropertyStr = "instanceManagerMeshName";
        const String instanceManagerGroupNamePropertyStr = "instanceManagerGroupName";
        const String instanceManagerTechniquePropertyStr = "instanceManagerTechnique";
        const String instanceManagerBatchSizePropertyStr = "instanceManagerBatchSize";
        const String instanceManagerFlagsPropertyStr = "instanceManagerFlags";
        const String instanceManagerSubMeshIndexPropertyStr = "instanceManagerSubMeshIndex";
        const String instanceManagerNumCustomParamsPropertyStr = "instanceManagerNumCustomParams";
    }  // namespace

    CInstanceManagerOgreNext::CInstanceManagerOgreNext( const String &customName,
                                                        const String &meshName,
                                                        const String &groupName, u32 technique,
                                                        u32 numInstancesPerBatch, u16 flags,
                                                        u16 subMeshIdx ) :
        m_customName( customName ),
        m_meshName( meshName ),
        m_groupName( groupName ),
        m_technique( technique ),
        m_numInstancesPerBatch( numInstancesPerBatch ),
        m_flags( flags ),
        m_subMeshIdx( subMeshIdx )
    {
    }

    void CInstanceManagerOgreNext::load( const String &fileName )
    {
        m_fileName = fileName;
    }

    void CInstanceManagerOgreNext::setNumCustomParams( u8 numCustomParams )
    {
        m_numCustomParams = numCustomParams;
    }

    u8 CInstanceManagerOgreNext::getNumCustomParams() const
    {
        return m_numCustomParams;
    }

    SmartPtr<Properties> CInstanceManagerOgreNext::getProperties() const
    {
        auto properties = ISharedObject::getProperties();
        properties->setProperty( instanceManagerNamePropertyStr, m_customName );
        properties->setProperty( instanceManagerMeshNamePropertyStr, m_meshName );
        properties->setProperty( instanceManagerGroupNamePropertyStr, m_groupName );
        properties->setProperty( instanceManagerTechniquePropertyStr, m_technique );
        properties->setProperty( instanceManagerBatchSizePropertyStr, m_numInstancesPerBatch );
        properties->setProperty( instanceManagerFlagsPropertyStr, static_cast<u32>( m_flags ) );
        properties->setProperty( instanceManagerSubMeshIndexPropertyStr,
                                 static_cast<u32>( m_subMeshIdx ) );
        properties->setProperty( instanceManagerNumCustomParamsPropertyStr,
                                 static_cast<u32>( m_numCustomParams ) );
        return properties;
    }

    void CInstanceManagerOgreNext::setProperties( SmartPtr<Properties> properties )
    {
        ISharedObject::setProperties( properties );

        if( !properties )
        {
            return;
        }

        u32 flags = m_flags;
        u32 subMeshIdx = m_subMeshIdx;
        u32 numCustomParams = m_numCustomParams;

        properties->getPropertyValue( instanceManagerNamePropertyStr, m_customName );
        properties->getPropertyValue( instanceManagerMeshNamePropertyStr, m_meshName );
        properties->getPropertyValue( instanceManagerGroupNamePropertyStr, m_groupName );
        properties->getPropertyValue( instanceManagerTechniquePropertyStr, m_technique );
        properties->getPropertyValue( instanceManagerBatchSizePropertyStr, m_numInstancesPerBatch );
        properties->getPropertyValue( instanceManagerFlagsPropertyStr, flags );
        properties->getPropertyValue( instanceManagerSubMeshIndexPropertyStr, subMeshIdx );
        properties->getPropertyValue( instanceManagerNumCustomParamsPropertyStr, numCustomParams );

        m_flags = static_cast<u16>( flags );
        m_subMeshIdx = static_cast<u16>( subMeshIdx );
        m_numCustomParams = static_cast<u8>( numCustomParams );
    }

    const String &CInstanceManagerOgreNext::getCustomName() const
    {
        return m_customName;
    }

    const String &CInstanceManagerOgreNext::getMeshName() const
    {
        return m_meshName;
    }

    const String &CInstanceManagerOgreNext::getGroupName() const
    {
        return m_groupName;
    }
}  // namespace workphone::render
