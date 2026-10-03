#ifndef DuplicateSelectionCmd_h__
#define DuplicateSelectionCmd_h__

#include <EditorPrerequisites.hpp>
#include <commands/Command.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>

namespace workphone::editor
{
    /**
     * @brief Command that duplicates the current editor selection.
     *
     * Each selected actor is serialized, prepared for duplication (new UUID and
     * renamed), and instantiated as a new actor in the scene. The command supports
     * undo/redo by tracking the created actors.
     */
    class DuplicateSelectionCmd : public Command
    {
    public:
        DuplicateSelectionCmd();
        ~DuplicateSelectionCmd() override;

        void undo() override;
        void redo() override;
        void execute() override;

        WP_CLASS_REGISTER_DECL;

    private:
        void createActorsFromData();

        Array<SmartPtr<Properties>> m_actorData;
        Array<SmartPtr<scene::IGameActor>> m_createdActors;
    };
}  // namespace workphone::editor

#endif  // DuplicateSelectionCmd_h__
