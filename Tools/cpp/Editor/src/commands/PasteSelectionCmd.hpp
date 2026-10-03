#ifndef PasteSelectionCmd_h__
#define PasteSelectionCmd_h__

#include <EditorPrerequisites.hpp>
#include <commands/Command.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>

namespace workphone::editor
{
    /**
     * @brief Command that creates new actors from the edit clipboard.
     *
     * The clipboard data is captured when the command is executed so that
     * subsequent clipboard changes do not affect undo/redo. The command supports
     * undo/redo by tracking the created actors.
     */
    class PasteSelectionCmd : public Command
    {
    public:
        PasteSelectionCmd();
        ~PasteSelectionCmd() override;

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

#endif  // PasteSelectionCmd_h__
