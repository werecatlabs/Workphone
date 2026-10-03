#ifndef CollisionMaskManager_h__
#define CollisionMaskManager_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace editor
    {
        class CollisionMaskManager : public ISharedObject
        {
        public:
            struct Option
            {
                String name;
                u32 mask = 0;
            };

            CollisionMaskManager();
            ~CollisionMaskManager() override;

            Array<Option> getOptions() const;
            Array<String> getOptionLabels() const;

            String getDefaultOptionName() const;
            u32 getDefaultMask() const;

            String normaliseOptionName( const String &name, u32 mask ) const;
            bool addOption( const String &name, u32 mask );
            bool removeOption( const String &name );

            u32 getOptionIndexByMask( u32 mask ) const;
            u32 getOptionIndexByName( const String &name ) const;
            u32 getMaskByIndex( u32 index ) const;
            String getOptionNameByIndex( u32 index ) const;

            void refreshFromScene();

            WP_CLASS_REGISTER_DECL;

        protected:
            void addActorMaskRecursive( SmartPtr<scene::IGameActor> actor );

            Array<Option> m_options;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // CollisionMaskManager_h__
