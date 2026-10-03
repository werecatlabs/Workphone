#ifndef _WP_IEvent_h__
#define _WP_IEvent_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>

namespace workphone
{

    /** Interface for an event. */
    class WPCore_API IEvent : public ISharedObject
    {
    public:
        static const hash_type loadingStateChanged;
        static const hash_type loadScene;
        static const hash_type unloadScene;
        static const hash_type handlePropertyButtonClick;

        static const hash_type actorLoaded;
        static const hash_type actorUnloaded;

        static const hash_type componentLoaded;
        static const hash_type componentUnloaded;

        static const hash_type addActor;
        static const hash_type removeActor;
        static const hash_type enabled;
        static const hash_type sceneChanged;

        static const hash_type createUI;
        static const hash_type destroyUI;

        static const hash_type handleWindowClose;
        static const hash_type handleWindowResize;
        static const hash_type handleWindowMove;

        static const hash_type handleTreeSelectionActivated;
        static const hash_type handleTreeSelectionRelease;
        static const hash_type handleTreeNodeDoubleClicked;

        static const hash_type handlePropertyChanged;
        static const hash_type handleValueChanged;

        static const hash_type handleMouseClicked;
        static const hash_type handleMouseReleased;

        static const hash_type handleSelection;
        static const hash_type handleToggle;

        static const hash_type handleDrop;
        static const hash_type handleDrag;

        static const hash_type inputEvent;
        static const hash_type updateEvent;

        static const hash_type handleEnterFrame;

        static const hash_type CLICK_HASH;
        static const hash_type ACTIVATE_HASH;
        static const hash_type DEACTIVATE_HASH;

        static const hash_type UPDATE_HASH;
        static const hash_type HANDLE_MESSAGE_HASH;
        static const hash_type INITIALISE_START_HASH;
        static const hash_type INITIALISE_END_HASH;

        static const hash_type ADD_CHILD_HASH;
        static const hash_type REMOVE_CHILD_HASH;

        static const hash_type CHANGED_STATE_HASH;
        static const hash_type CHILD_CHANGED_STATE_HASH;

        static const hash_type TOGGLE_ENABLED_HASH;
        static const hash_type TOGGLE_HIGHLIGHT_HASH;

        static const hash_type VISIBLE_HASH;

        static const hash_type SHOW_HASH;
        static const hash_type HIDE_HASH;

        static const hash_type SELECT_HASH;
        static const hash_type DESELECT_HASH;

        static const hash_type GAIN_FOCUS_HASH;
        static const hash_type LOST_FOCUS_HASH;
        static const hash_type childChangedState;

        static const hash_type materialSetup;
        static const hash_type contactStart;
        static const hash_type contactEnd;
        static const hash_type contactBreak;
        static const hash_type noContact;
        static const hash_type contact;
        static const hash_type contactProcess;

        static const hash_type queued;
        static const hash_type execute;
        static const hash_type completed;

        static const hash_type stateChanged;
        static const hash_type stateMessage;

        static const hash_type transform;

        static const hash_type addSelectedObject;
        static const hash_type addSelectedObjects;
        static const hash_type deselectObjects;
        static const hash_type deselectAll;

        //! Occurs when a command is added to the command stack
        static const hash_type addCommand;

        //! Occurs when the next command it retrieved
        static const hash_type getNextCommand;

        //! Occurs when the previous command it retrieved
        static const hash_type getPreviousCommand;

        static const hash_type refreshAll;
        static const hash_type refreshPath;

        static const hash_type addUIElement;
        static const hash_type removeUIElement;
        static const hash_type transformUIElement;

        static const hash_type windowMovedOrResized;

        static const hash_type renderTargetTextureLoaded;
        static const hash_type renderTargetTextureUnloaded;

        static const hash_type meshLoaded;
        static const hash_type meshesImported;

        static const hash_type cameraManagerReset;

        static const hash_type fileAction;

        static const hash_type jobStarted;
        static const hash_type jobCompleted;

        static const hash_type addChild;
        static const hash_type removeChild;
        static const hash_type changedState;

        static const hash_type toggleEnabled;
        static const hash_type toggleVisibility;
        static const hash_type toggleHighlight;

        IEvent();

        IEvent( u32 poolTypeId );

        /** Virtual destructor. */
        ~IEvent() override;

        /**
         * @brief Gets the intended recipient of this event.
         *
         * A null
         * target means the event is a broadcast. The separate event target keeps
         * the `object`
         * passed to event listeners available as event-specific payload.
         *
         * @return
         * The target object, or nullptr for a broadcast event.
         */
        SmartPtr<ISharedObject> getTarget() const;

        /**
         * @brief Sets the intended recipient of this event.
         * @param target The
         * target object, or nullptr to broadcast the event.
         */
        void setTarget( SmartPtr<ISharedObject> target );

        /**
         * @brief Checks whether this event targets a particular object.
         * @param
         * object The prospective recipient.
         * @return True when the event has the supplied
         * target.
         */
        bool isTarget( SmartPtr<ISharedObject> object ) const;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Strong reference retained until queued event dispatch has completed.
        AtomicSmartPtr<ISharedObject> m_target;
    };

}  // namespace workphone

#endif  // IEvent_h__
