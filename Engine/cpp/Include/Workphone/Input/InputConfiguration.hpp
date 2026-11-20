#ifndef InputConfiguration_h__
#define InputConfiguration_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/System/FSMListener.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Input/AxisConfigurationData.hpp>
#include <Workphone/Input/InputEvent.hpp>

/**
 * @file InputConfiguration.hpp
 * @brief State machine driving axis centering and stick-extent calibration.
 */

namespace workphone
{
    /**
     * @class InputConfiguration
     * @brief Finite-state machine driven calibration of input axes.
     *
     * InputConfiguration owns a list of AxisConfigurationData channels and an IFSM with the states
     * of InputConfiguration::State: Normal (idle), Center (axis centering) and FindExtents
     * (recording the minimum/maximum stick throw). centerAxes(), calculateStickExtents() and
     * finishCalculateStickExtents() switch between the states, and an internal
     * InputConfigurationFSMListener forwards the FSM callbacks to the per-state hooks of this
     * class. Leaving FindExtents writes the resulting multipliers and offsets to the rx_map
     * database table.
     *
     * @note As currently implemented, the constructor registers its listener on the FSM member
     *       without creating the FSM itself (creation is commented out in the source), so an FSM
     *       must already be available at that point.
     */
    class InputConfiguration : public ISharedObject
    {
    public:
        /**
         * @brief States of the configuration state machine.
         */
        enum State
        {
            Normal,       ///< Idle; no calibration in progress.
            Center,       ///< Centering the axes.
            FindExtents,  ///< Recording the minimum and maximum throw of the axes.
        };

        /**
         * @brief FSM listener that forwards FSM callbacks to the owning InputConfiguration.
         *
         * For the FSM's current state the callbacks are dispatched to the matching hook of the
         * owner: preUpdateFSM() to preUpdateState*(), updateFSM() to updateModel*(), postUpdateFSM()
         * to postUpdate*(), and the Enter/Leave events of handleFSMEvent() to enterState*() and
         * leaveState*().
         */
        class InputConfigurationFSMListener : public FSMListener
        {
        public:
            /**
             * @brief Constructs a listener that is not attached to an InputConfiguration.
             * @note Its callbacks must not be invoked until m_inputConfiguration is set.
             */
            InputConfigurationFSMListener();

            /**
             * @brief Constructs a listener forwarding to the given InputConfiguration.
             * @param inputConfiguration Non-owning pointer to the configuration to forward to.
             */
            InputConfigurationFSMListener( RawPtr<InputConfiguration> inputConfiguration );

            /**
             * @brief Virtual destructor.
             */
            ~InputConfigurationFSMListener() override;

            /**
             * @brief Called before the FSM update; forwards to the owner's preUpdateState* hook for
             *        the FSM's current state.
             * @param fsm The FSM being updated.
             * @param task The task identifier of the update.
             * @param t Current time in seconds.
             * @param dt Delta time since the last update in seconds.
             */
            virtual void preUpdateFSM( SmartPtr<IFSM> fsm, TaskId task, const double &t,
                                       const double &dt );

            /**
             * @brief Called after the FSM update; forwards to the owner's postUpdate* hook for the
             *        FSM's current state.
             * @param fsm The FSM being updated.
             * @param task The task identifier of the update.
             * @param t Current time in seconds.
             * @param dt Delta time since the last update in seconds.
             */
            virtual void postUpdateFSM( SmartPtr<IFSM> fsm, TaskId task, const double &t,
                                        const double &dt );

            /**
             * @brief Called during the FSM update; forwards to the owner's updateModel* hook for the
             *        FSM's current state.
             * @param fsm The FSM being updated.
             * @param task The task identifier of the update.
             * @param t Current time in seconds.
             * @param dt Delta time since the last update in seconds.
             */
            virtual void updateFSM( SmartPtr<IFSM> fsm, TaskId task, const double &t, const double &dt );

            /**
             * @brief Handles FSM events; Enter and Leave call the owner's enterState* and
             *        leaveState* hook for the FSM's current state. Other events are ignored.
             * @param fsm The FSM that raised the event.
             * @param eventType The event, an FSMEvent value cast to s32.
             * @return Always 0.
             */
            virtual s32 handleFSMEvent( SmartPtr<IFSM> fsm, s32 eventType );

            /**
             * @brief Called when the FSM switches to a new state. Does nothing.
             * @param state The new state.
             */
            virtual void newState( s32 state );

            /// Non-owning pointer to the configuration that receives the forwarded callbacks.
            RawPtr<InputConfiguration> m_inputConfiguration;

            WP_CLASS_REGISTER_DECL;
        };

        /**
         * @brief Constructs the configuration and registers its FSM listener on the FSM.
         * @see InputConfiguration for a note about FSM creation.
         */
        InputConfiguration();

        /**
         * @brief Virtual destructor.
         */
        ~InputConfiguration() override;

        /**
         * @brief Pre-update hook. Currently does nothing (FSM forwarding is disabled).
         * @param task The task identifier of the update.
         * @param t Current time in seconds.
         * @param dt Delta time since the last update in seconds.
         */
        virtual void preUpdate( TaskId task, const double &t, const double &dt );

        /**
         * @brief Update hook. Currently does nothing (FSM forwarding is disabled).
         * @param task The task identifier of the update.
         * @param t Current time in seconds.
         * @param dt Delta time since the last update in seconds.
         */
        virtual void update( TaskId task, const double &t, const double &dt );

        /**
         * @brief Post-update hook. Currently does nothing (FSM forwarding is disabled).
         * @param task The task identifier of the update.
         * @param t Current time in seconds.
         * @param dt Delta time since the last update in seconds.
         */
        virtual void postUpdate( TaskId task, const double &t, const double &dt );

        /**
         * @brief Starts centering the axes by switching the FSM to the Center state immediately.
         */
        void centerAxes();

        /**
         * @brief Finishes axis centering. Currently does nothing; the FSM stays in its state.
         */
        void finishCenterAxes();

        /**
         * @brief Starts recording the stick extents by switching the FSM to the FindExtents state
         *        immediately.
         */
        void calculateStickExtents();

        /**
         * @brief Finishes recording the stick extents by switching the FSM back to Normal
         *        immediately, which applies the results (see leaveStateFindExtents()).
         */
        void finishCalculateStickExtents();

        /**
         * @brief Gets the stick direction label for a control name.
         * @param input Control name, e.g. "Throttle", "Aileron", "Elevator", "Rudder" or
         *        "Collective Pitch" (case-sensitive).
         * @return "vertical" for Throttle, Elevator and Collective Pitch; "horizontal" for Aileron
         *         and Rudder; "none" for any other name.
         */
        String getDirectionLabel( const String &input );

        /**
         * @brief Gets the state machine.
         * @return Reference to the FSM pointer.
         */
        SmartPtr<IFSM> &getFSM();

        /**
         * @brief Gets the state machine.
         * @return Const reference to the FSM pointer.
         */
        const SmartPtr<IFSM> &getFSM() const;

        /**
         * @brief Replaces the state machine.
         * @param fsm The new FSM.
         * @note The internal listener is not registered on the new FSM.
         */
        void setFSM( SmartPtr<IFSM> fsm );

        /**
         * @brief Gets the axis channels being configured.
         * @return A copy (snapshot) of the channel list.
         */
        Array<SmartPtr<AxisConfigurationData>> getChannels() const;

        /**
         * @brief Replaces the axis channels being configured.
         * @param channels The new channel list; its contents are copied.
         */
        void setChannels( Array<SmartPtr<AxisConfigurationData>> channels );

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Called when the FSM enters the Normal state. Does nothing.
        virtual void enterStateNormal();
        /// Pre-update in the Normal state. Does nothing.
        virtual void preUpdateStateNormal( TaskId task, const double &t, const double &dt );
        /// Update in the Normal state. Does nothing.
        virtual void updateModelNormal( TaskId task, const double &t, const double &dt );
        /// Post-update in the Normal state. Does nothing.
        virtual void postUpdateNormal( TaskId task, const double &t, const double &dt );
        /// Called when the FSM leaves the Normal state. Does nothing.
        virtual void leaveStateNormal();

        /// Called when the FSM enters the Center state. Does nothing.
        virtual void enterStateCenterAxis();
        /**
         * @brief Pre-update in the Center state.
         * @param task The task identifier; only TaskId::Primary is processed.
         * @param t Current time in seconds.
         * @param dt Delta time since the last update in seconds.
         * @note The offset computation is currently commented out, so this has no effect on the
         *       channels. Exceptions are logged.
         */
        virtual void preUpdateStateCenterAxis( TaskId task, const double &t, const double &dt );
        /// Update in the Center state. Does nothing.
        virtual void updateModelCenterAxis( TaskId task, const double &t, const double &dt );
        /// Post-update in the Center state. Does nothing.
        virtual void postUpdateCenterAxis( TaskId task, const double &t, const double &dt );
        /// Called when the FSM leaves the Center state. Does nothing (the offset write is disabled).
        virtual void leaveStateCenterAxis();

        /// Called when the FSM enters the FindExtents state. Does nothing.
        virtual void enterStateFindExtents();
        /**
         * @brief Pre-update in the FindExtents state; widens the recorded throw of every channel.
         * @param task The task identifier; only TaskId::Primary is processed.
         * @param t Current time in seconds.
         * @param dt Delta time since the last update in seconds.
         * @note For each channel the raw axis value is read. Values with a magnitude above 0.01
         *       and below 10 extend max_throw/min_throw (and the matching multiplier) when they
         *       exceed them. Non-finite multipliers are reset to 0. Exceptions are logged.
         */
        virtual void preUpdateStateFindExtents( TaskId task, const double &t, const double &dt );
        /// Update in the FindExtents state. Does nothing.
        virtual void updateModelFindExtents( TaskId task, const double &t, const double &dt );
        /// Post-update in the FindExtents state. Does nothing.
        virtual void postUpdateFindExtents( TaskId task, const double &t, const double &dt );
        /**
         * @brief Called when the FSM leaves the FindExtents state; stores the calibration.
         *
         * For every channel, each rx_map database row whose cmap equals the channel's cmap gets
         * its multiplier set to 1, its low/high multipliers derived from the recorded throw
         * multipliers and the channel's new offset (-0.8 / (min + offset) and 0.8 / (max + offset)),
         * and its offset updated.
         */
        virtual void leaveStateFindExtents();

        /**
         * @brief Resets the offset of all rx_map rows to 0 and their multipliers to 1.
         * @note Does nothing if no database is available.
         */
        void resetDBOffset();

        /**
         * @brief Placeholder for applying raw channel values to the configuration. Currently has
         *        no effect.
         */
        void safeApply();

        /// State machine driving the calibration states.
        SmartPtr<IFSM> m_fsm;
        /// Listener registered on m_fsm that forwards the callbacks to this object.
        SmartPtr<FSMListener> m_listener;
        /// Axis channels being configured (safe for concurrent access).
        ConcurrentArray<SmartPtr<AxisConfigurationData>> m_channels;
    };
}  // namespace workphone

#endif  // InputConfiguration_h__
