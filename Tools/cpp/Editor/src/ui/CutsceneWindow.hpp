#ifndef __CutsceneWindow_h__
#define __CutsceneWindow_h__

#include <EditorPrerequisites.hpp>
#include <ui/EditorWindow.hpp>
#include <Workphone/Scene/Cutscene.hpp>
#include <Workphone/Scene/Components/CutscenePlayer.hpp>
#include <Workphone/Interface/UI/IUIWindow.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>
#include <Workphone/Interface/UI/IUITextEntry.hpp>
#include <Workphone/Interface/UI/IUITreeCtrl.hpp>
#include <Workphone/Interface/UI/IUIVector3.hpp>

namespace workphone
{
    namespace editor
    {
        /**
         * @brief Editor window for creating and editing Cutscene assets.
         *
         * Provides a simple track/keyframe editor and playback preview for
         * engine cutscenes. Cutscenes can be saved to and loaded from the
         * project's media folders.
         */
        class CutsceneWindow : public EditorWindow
        {
        public:
            /**
             * @brief Listener that forwards UI events to the CutsceneWindow.
             */
            class WindowListener : public IEventListener
            {
            public:
                WindowListener();
                ~WindowListener() override;

                Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

                SmartPtr<CutsceneWindow> getOwner() const;
                void setOwner( SmartPtr<CutsceneWindow> owner );

                WP_CLASS_REGISTER_DECL;

            private:
                AtomicWeakPtr<CutsceneWindow> m_owner;
            };

            CutsceneWindow();
            ~CutsceneWindow() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;
            void update() override;

            /** @brief Gets the cutscene asset being edited. */
            SmartPtr<scene::Cutscene> getCutscene() const;

            /** @brief Sets the cutscene asset being edited. */
            void setCutscene( SmartPtr<scene::Cutscene> cutscene );

            /** @brief Rebuilds the track tree from the current cutscene. */
            void rebuildTrackTree();

            /** @brief Rebuilds the keyframe tree for the selected track. */
            void rebuildKeyframeTree();

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Clears the edit state and creates a blank cutscene. */
            void newCutscene();

            /** @brief Opens a cutscene file from disk. */
            void openCutscene();

            /** @brief Saves the current cutscene to disk. */
            void saveCutscene();

            /** @brief Adds a new track to the cutscene. */
            void addTrack();

            /** @brief Removes the selected track from the cutscene. */
            void removeSelectedTrack();

            /** @brief Adds a new keyframe to the selected track. */
            void addKeyframe();

            /** @brief Removes the selected keyframe from the selected track. */
            void removeSelectedKeyframe();

            /** @brief Applies the current keyframe edit fields to the selected keyframe. */
            void applyKeyframeEdit();

            /** @brief Starts the in-editor preview. */
            void playPreview();

            /** @brief Stops the in-editor preview. */
            void stopPreview();

            /** @brief Advances the preview if it is running. */
            void tickPreview();

            /** @brief Updates the edit fields from the selected keyframe. */
            void syncEditFields();

            s32 getSelectedTrackIndex() const;
            s32 getSelectedKeyframeIndex() const;

            SmartPtr<ui::IUIWindow> m_window;
            SmartPtr<IEventListener> m_windowListener;

            SmartPtr<ui::IUITreeCtrl> m_trackTree;
            SmartPtr<ui::IUITreeCtrl> m_keyframeTree;

            SmartPtr<ui::IUITextEntry> m_filePathText;
            SmartPtr<ui::IUITextEntry> m_targetActorText;
            SmartPtr<ui::IUITextEntry> m_trackTypeText;
            SmartPtr<ui::IUITextEntry> m_timeText;
            SmartPtr<ui::IUIVector3> m_valueVector;
            SmartPtr<ui::IUITextEntry> m_valueScalarText;
            SmartPtr<ui::IUITextEntry> m_lengthText;

            SmartPtr<scene::Cutscene> m_cutscene;
            SmartPtr<scene::CutscenePlayer> m_previewPlayer;

            bool m_previewPlaying = false;
            f32 m_previewTime = 0.0f;

            
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // __CutsceneWindow_h__
