#ifndef WPGraphics_ClawDebug_hpp__
#define WPGraphics_ClawDebug_hpp__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/Debug.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    namespace ui
    {
        class IUIText;
    }

    namespace render
    {
        /** Debug drawing adapter used by the ClawHammer renderer. */
        class WPGraphics_API ClawDebug final : public Debug
        {
        public:
            ClawDebug() = default;
            ~ClawDebug() override;

            void unload( SmartPtr<ISharedObject> data ) override;
            void clear() override;

            void drawText( hash_type id, const Vector2<real_Num> &position, const String &text,
                           u32 color ) override;

        private:
            RecursiveMutex m_mutex;
            Map<hash_type, SmartPtr<ui::IUIText>> m_textElements;
        };
    }  // namespace render
}  // namespace workphone

#endif  // WPGraphics_ClawDebug_hpp__
