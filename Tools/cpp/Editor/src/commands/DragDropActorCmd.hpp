#ifndef _ADD_ENTITY_BODY_CMD_H
#define _ADD_ENTITY_BODY_CMD_H

#include <EditorPrerequisites.hpp>
#include <Workphone/Core/Properties.hpp>
#include <commands/Command.hpp>

namespace workphone
{
    namespace editor
    {
        /**
         * Command to add a new entity body to the scene.
         */
        class DragDropActorCmd : public Command
        {
        public:
            /**
             * Constructor.
             */
            DragDropActorCmd();

            /**
             * Destructor.
             */
            ~DragDropActorCmd() override;

            void undo() override;
            void redo() override;
            void execute() override;

            Vector2I getPosition() const;
            void setPosition( const Vector2I &position );

            SmartPtr<ui::IUIElement> getSrc() const;
            void setSrc( SmartPtr<ui::IUIElement> src );

            SmartPtr<ui::IUIElement> getDst() const;
            void setDst( SmartPtr<ui::IUIElement> dst );

            String getData() const;
            void setData( const String &data );

            /**
             * Get the sibling index of the entity body.
             * @return The sibling index.
             */
            s32 getSiblingIndex() const;

            /**
             * Set the sibling index of the entity body.
             * @param siblingIndex The sibling index.
             */
            void setSiblingIndex( s32 siblingIndex );

            WP_CLASS_REGISTER_DECL;

        private:
            Vector2I m_position = Vector2I::zero();
            SmartPtr<ui::IUIElement> m_src;
            SmartPtr<ui::IUIElement> m_dst;
            String m_data;
            s32 m_siblingIndex = -1;
            s32 m_previousSiblingIndex = -1;
            SmartPtr<scene::IGameActor> m_actor;
            SmartPtr<scene::IGameActor> m_previousParent;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // _ADD_ENTITY_BODY_CMD_H
