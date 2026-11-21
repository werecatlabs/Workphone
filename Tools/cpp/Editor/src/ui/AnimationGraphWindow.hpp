#ifndef AnimationGraphWindow_h__
#define AnimationGraphWindow_h__

#include <ui/EditorWindow.hpp>
#include <Workphone/Interface/UI/IUIAnimationGraph.hpp>
#include <mutex>

namespace workphone::editor
{

    class AnimationGraphWindow : public EditorWindow
    {
    public:
        ~AnimationGraphWindow() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;

        void update() override;

        WP_CLASS_REGISTER_DECL;

    private:
        using Model = ui::IUIAnimationGraph::Model;

        SmartPtr<scene::Animator> findSelectedAnimator() const;
        void loadFromAnimator( SmartPtr<scene::Animator> animator, Model &model );
        void applyGraph( Model &model );
        void updatePlayback( Model &model );

        std::mutex m_windowMutex;
        SmartPtr<ui::IUIAnimationGraph> m_graphUI;
        SmartPtr<scene::Animator> m_animator;
    };

}  // namespace workphone::editor

#endif  // AnimationGraphWindow_h__
