#ifndef WPGraphics_ClawDebug_hpp__
#define WPGraphics_ClawDebug_hpp__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/Debug.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>
#include <chrono>

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

            SmartPtr<IDebugLine> drawLine( hash_type id, const Vector3<real_Num> &start,
                                           const Vector3<real_Num> &end, u32 colour ) override;
            void renderLines( IRenderer3 &renderer );

            void drawText( hash_type id, const Vector2<real_Num> &position, const String &text,
                           u32 color ) override;

        private:
            struct TimedLine
            {
                SmartPtr<IDebugLine> line;
                std::chrono::steady_clock::time_point refreshed;
            };

            RecursiveMutex m_mutex;
            Map<hash_type, TimedLine> m_lines;
            Map<hash_type, SmartPtr<ui::IUIText>> m_textElements;
        };
    }  // namespace render
}  // namespace workphone

#endif  // WPGraphics_ClawDebug_hpp__
