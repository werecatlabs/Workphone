#include <EditorPCH.hpp>
#include "ui/ProjectTreeData.hpp"
#include <Workphone/Workphone.hpp>

#include <utility>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone, ProjectTreeData, ISharedObject );

    ProjectTreeData::ProjectTreeData( String ownerType, String objectType,
                                      SmartPtr<ISharedObject> ownerData,
                                      SmartPtr<ISharedObject> objectData ) :
        m_ownerData( std::move( ownerData ) ),
        m_objectData( std::move( objectData ) ),
        m_ownerType( std::move( ownerType ) ),
        m_objectType( std::move( objectType ) )
    {
    }

    ProjectTreeData::ProjectTreeData() = default;

    void ProjectTreeData::unload( SmartPtr<ISharedObject> data )
    {
        m_ownerData = nullptr;
        m_objectData = nullptr;
    }

    SmartPtr<ISharedObject> ProjectTreeData::getOwnerData() const
    {
        return m_ownerData;
    }

    void ProjectTreeData::setOwnerData( SmartPtr<ISharedObject> ownerData )
    {
        m_ownerData = ownerData;
    }

    SmartPtr<ISharedObject> ProjectTreeData::getObjectData() const
    {
        return m_objectData;
    }

    void ProjectTreeData::setObjectData( SmartPtr<ISharedObject> objectData )
    {
        m_objectData = objectData;
    }

    String ProjectTreeData::getOwnerType() const
    {
        return m_ownerType;
    }

    void ProjectTreeData::setOwnerType( const String &ownerType )
    {
        m_ownerType = ownerType;
    }

    String ProjectTreeData::getObjectType() const
    {
        return m_objectType;
    }

    void ProjectTreeData::setObjectType( const String &objectType )
    {
        m_objectType = objectType;
    }
}  // namespace workphone::editor
