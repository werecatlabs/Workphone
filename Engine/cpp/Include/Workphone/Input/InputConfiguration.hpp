#ifndef InputConfiguration_h__
#define InputConfiguration_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/System/FSMListener.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Input/AxisConfigurationData.hpp>
#include <Workphone/Input/InputEvent.hpp>

namespace workphone
{
    /**
     * @class InputConfiguration
     * @brief Finite-state machine driven configuration of input axes.
     *
     * InputConfiguration tracks the calibration state for a set of axes
     * (centering, finding extents, and the normal idle state) and exposes
     * helpers to drive transitions between them.
     */
    class InputConfiguration : public ISharedObject
    {
    public:
        enum State
        {
            Normal,
            Center,
            FindExtents,
        };

        class InputConfigurationFSMListener : public FSMListener
        {
        public:
            InputConfigurationFSMListener();
            InputConfigurationFSMListener( RawPtr<InputConfiguration> inputConfiguration );
            ~InputConfigurationFSMListener() override;

            virtual void preUpdateFSM( SmartPtr<IFSM> fsm, TaskId task, const double &t,
                                       const double &dt );
            virtual void postUpdateFSM( SmartPtr<IFSM> fsm, TaskId task, const double &t,
                                        const double &dt );
            virtual void updateFSM( SmartPtr<IFSM> fsm, TaskId task, const double &t, const double &dt );
            virtual s32 handleFSMEvent( SmartPtr<IFSM> fsm, s32 eventType );
            virtual void newState( s32 state );

            RawPtr<InputConfiguration> m_inputConfiguration;

            WP_CLASS_REGISTER_DECL;
        };

        InputConfiguration();
        ~InputConfiguration() override;

        virtual void preUpdate( TaskId task, const double &t, const double &dt );
        virtual void update( TaskId task, const double &t, const double &dt );
        virtual void postUpdate( TaskId task, const double &t, const double &dt );

        void centerAxes();
        void finishCenterAxes();

        void calculateStickExtents();
        void finishCalculateStickExtents();

        String getDirectionLabel( const String &input );

        SmartPtr<IFSM> &getFSM();
        const SmartPtr<IFSM> &getFSM() const;
        void setFSM( SmartPtr<IFSM> fsm );

        Array<SmartPtr<AxisConfigurationData>> getChannels() const;
        void setChannels( Array<SmartPtr<AxisConfigurationData>> channels );

        WP_CLASS_REGISTER_DECL;

    protected:
        virtual void enterStateNormal();
        virtual void preUpdateStateNormal( TaskId task, const double &t, const double &dt );
        virtual void updateModelNormal( TaskId task, const double &t, const double &dt );
        virtual void postUpdateNormal( TaskId task, const double &t, const double &dt );
        virtual void leaveStateNormal();

        virtual void enterStateCenterAxis();
        virtual void preUpdateStateCenterAxis( TaskId task, const double &t, const double &dt );
        virtual void updateModelCenterAxis( TaskId task, const double &t, const double &dt );
        virtual void postUpdateCenterAxis( TaskId task, const double &t, const double &dt );
        virtual void leaveStateCenterAxis();

        virtual void enterStateFindExtents();
        virtual void preUpdateStateFindExtents( TaskId task, const double &t, const double &dt );
        virtual void updateModelFindExtents( TaskId task, const double &t, const double &dt );
        virtual void postUpdateFindExtents( TaskId task, const double &t, const double &dt );
        virtual void leaveStateFindExtents();

        void resetDBOffset();

        void safeApply();

        SmartPtr<IFSM> m_fsm;
        SmartPtr<FSMListener> m_listener;
        ConcurrentArray<SmartPtr<AxisConfigurationData>> m_channels;
    };
}  // namespace workphone

#endif  // InputConfiguration_h__
