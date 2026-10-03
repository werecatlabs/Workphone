#ifndef ImGuiDropdown_h__
#define ImGuiDropdown_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIDropdown.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiOutputConsole
        {
        public:
            ImGuiOutputConsole();

            void Log( const std::string &message );

            void Clear();

            void Show();

        private:
            std::vector<std::string> m_log;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiDropdown_h__
