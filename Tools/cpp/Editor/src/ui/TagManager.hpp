#ifndef TagManager_h__
#define TagManager_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace editor
    {
        class TagManager : public ISharedObject
        {
        public:
            TagManager();
            ~TagManager() override;

            Array<String> getTags() const;
            void setTags( const Array<String> &tags );

            String normaliseTag( const String &tag ) const;
            bool hasTag( const String &tag ) const;
            bool addTag( const String &tag );
            bool removeTag( const String &tag );

            void refreshFromScene();

            WP_CLASS_REGISTER_DECL;

        protected:
            void addActorTagsRecursive( SmartPtr<scene::IGameActor> actor );
            void removeActorTagRecursive( SmartPtr<scene::IGameActor> actor, const String &tag );

            Array<String> m_tags;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // TagManager_h__
