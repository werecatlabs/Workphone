#ifndef SceneDropJob_h__
#define SceneDropJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class SceneDropJob : public Job
        {
        public:
            static const String resourceUUIDStr;
            static const String filePathStr;
            static const String prefabExtStr;

            SceneDropJob();
            ~SceneDropJob() override;

            void execute() override;

            String getData() const;

            void setData( const String &data );

            String getFilePath() const;

            void setFilePath( const String &filePath );

            SmartPtr<ui::IUIElement> getSender() const;

            void setSender( SmartPtr<ui::IUIElement> sender );

            SmartPtr<ICommand> getDragDropActorCmd() const;

            void setDragDropActorCmd( SmartPtr<ICommand> dragDropActorCmd );

            SmartPtr<ui::IUITreeCtrl> getTree() const;

            void setTree( SmartPtr<ui::IUITreeCtrl> tree );

            SmartPtr<SceneWindow> getOwner() const;

            void setOwner( SmartPtr<SceneWindow> owner );

            s32 getSiblingIndex() const;
            void setSiblingIndex( s32 siblingIndex );

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<SceneWindow> m_owner;
            SmartPtr<ui::IUITreeCtrl> m_tree;
            SmartPtr<ui::IUIElement> m_sender;
            SmartPtr<ICommand> m_dragDropActorCmd;
            String m_data;
            String m_filePath;
            s32 m_siblingIndex = -1;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // SceneDropJob_h__
