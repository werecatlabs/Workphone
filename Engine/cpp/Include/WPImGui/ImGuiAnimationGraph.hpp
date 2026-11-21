#ifndef ImGuiAnimationGraph_h__
#define ImGuiAnimationGraph_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIAnimationGraph.hpp>
#include <array>
#include <optional>

namespace workphone
{
    namespace ui
    {
        class ImGuiAnimationGraph : public ImGuiElement<IUIAnimationGraph>
        {
        public:
            void update() override;

            WP_CLASS_REGISTER_DECL;

        private:
            void draw();
            void drawToolbar();
            void drawCanvas( f32 width, f32 height );
            void drawInspector( f32 width, f32 height );
            void drawStateInspector();
            void drawTransitionInspector();
            void drawParameters();
            void drawPoseNodes();
            void drawPoseNodeInspector();
            bool chooseClip( const char *label, String &id );
            bool chooseParameter( const char *label, String &name, std::optional<bool> isBoolean );
            bool chooseState( const char *label, String &id, const String &excluded );
            void addState( const String &clipId, const String &poseNodeId, Vector2F position );
            void addTransition( const String &from, const String &to );
            void drawAddStateMenu( Vector2F position );
            void selectState( const String &id );
            void selectTransition( size_t index );
            void fitCanvas( Vector2F size );
            void markEdited();

            bool m_inputScope = false;
            String m_selectedState;
            String m_selectedPoseNode;
            std::optional<animation::AnimationGraphPoseNodeDefinition> m_poseDraft;
            std::optional<size_t> m_selectedTransition;
            bool m_showInspector = false;
            std::array<char, 128> m_newParameterName{};
            bool m_newParameterIsBoolean = false;
            Vector2F m_camera = Vector2F::zero();
            f32 m_zoom = 1.0f;
            bool m_fitOnDraw = true;
            bool m_panning = false;
            String m_draggedState;
            String m_linkSource;
            Vector2F m_dragOrigin = Vector2F::zero();
            Vector2F m_dragStart = Vector2F::zero();
            Vector2F m_addPosition = Vector2F::zero();
        };
    }  // namespace ui
}  // namespace workphone

#endif  // ImGuiAnimationGraph_h__
