#ifndef ToggleEditorCamera_h__
#define ToggleEditorCamera_h__

#include <commands/Command.hpp>

namespace workphone
{
    namespace editor
    {
        /**
         * Command to toggle the editor camera on or off.
         */
        class ToggleEditorCamera : public Command
        {
        public:
            /** Constructor.
             */
            ToggleEditorCamera();

            /** Destructor.
             */
            ~ToggleEditorCamera() override;

            void undo() override;
            void redo() override;
            void execute() override;

            bool getToggleValue() const;

            void setToggleValue( bool toggleValue );

            WP_CLASS_REGISTER_DECL;

        protected:
            bool m_toggleValue = false;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // ToggleEditorCamera_h__
