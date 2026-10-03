#ifndef PromptCmd_h__
#define PromptCmd_h__

#include <commands/Command.hpp>

namespace workphone
{
    namespace editor
    {
        /** PromptCmd
         *  Command to set the prompt for the AI
         */
        class PromptCmd : public Command
        {
        public:
            /** Constructor
             */
            PromptCmd();

            /** Destructor
             */
            ~PromptCmd() override;

            void undo() override;
            void redo() override;
            void execute() override;

            void processAIPrompt( const String &promptStr );

            String getPrompt() const;

            void setPrompt( const String &prompt );

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_prompt;
            Array<SmartPtr<IAiGoalEvaluator>> goalEvaluators;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // PromptCmd_h__
