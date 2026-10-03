#ifndef ProjectTreeData_h__
#define ProjectTreeData_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace editor
    {
        class ProjectTreeData : public ISharedObject
        {
        public:
            ProjectTreeData();

            ProjectTreeData( String ownerType, String objectType, SmartPtr<ISharedObject> ownerData,
                             SmartPtr<ISharedObject> objectData );

            void unload( SmartPtr<ISharedObject> data ) override;

            SmartPtr<ISharedObject> getOwnerData() const;
            void setOwnerData( SmartPtr<ISharedObject> ownerData );

            SmartPtr<ISharedObject> getObjectData() const;
            void setObjectData( SmartPtr<ISharedObject> objectData );

            String getOwnerType() const;
            void setOwnerType( const String &ownerType );

            String getObjectType() const;
            void setObjectType( const String &objectType );

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<ISharedObject> m_ownerData;
            SmartPtr<ISharedObject> m_objectData;

            String m_ownerType;

            String m_objectType;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // ProjectTreeData_h__
