#ifndef CInstanceManagerOgreNext_h__
#define CInstanceManagerOgreNext_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IInstanceManager.hpp>

namespace workphone::render
{
    class CInstanceManagerOgreNext : public IInstanceManager
    {
    public:
        CInstanceManagerOgreNext( const String &customName, const String &meshName,
                                  const String &groupName, u32 technique,
                                  u32 numInstancesPerBatch, u16 flags, u16 subMeshIdx );

        void load( const String &fileName ) override;

        void setNumCustomParams( u8 numCustomParams ) override;

        u8 getNumCustomParams() const override;

        SmartPtr<Properties> getProperties() const override;

        void setProperties( SmartPtr<Properties> properties ) override;

        const String &getCustomName() const;

        const String &getMeshName() const;

        const String &getGroupName() const;

    private:
        String m_customName;
        String m_meshName;
        String m_groupName;
        String m_fileName;
        u32 m_technique = IInstanceManager::HWInstancingBasic;
        u32 m_numInstancesPerBatch = 80;
        u16 m_flags = 0;
        u16 m_subMeshIdx = 0;
        u8 m_numCustomParams = 0;
    };
}  // namespace workphone::render

#endif  // CInstanceManagerOgreNext_h__
