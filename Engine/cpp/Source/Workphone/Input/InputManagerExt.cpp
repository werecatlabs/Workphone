#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Input/InputManagerExt.hpp"
#include <Workphone/Input/InputEvent.hpp>
#include "Workphone/Input/InputConfiguration.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, InputManagerExt, IInputManager );

    static const hash_type interlink_hash = StringUtil::getHash( "interlink" );
    static const hash_type vbar_hash = StringUtil::getHash( "vbar" );
    static const hash_type gamepad_hash = StringUtil::getHash( "gamepad" );
    static const hash_type jeti_usb_hash = StringUtil::getHash( "jeti_usb" );

    static const hash_type NORMAL_HASH = StringUtil::getHash( "Normal" );
    static const hash_type THROTTLE_HOLD_HASH = StringUtil::getHash( "ThrottleHold" );
    static const hash_type IDLE_UP_HASH = StringUtil::getHash( "IdleUp" );

    bool InputManagerExt::isDongleConnected()
    {
        return isDongleOk();
    }

    Array<String> InputManagerExt::getConnectedDevices() const
    {
        return {};
    }

    hash_type InputManagerExt::getAdapterModeHash() const
    {
        return m_adapterModeHash;
    }

    InputManagerExt::InputManagerExt() : InputManagerExt( nullptr, true, true )
    {
    }

    InputManagerExt::InputManagerExt( render::IGraphicsWindow *window, bool bufferedKeys,
                                      bool bufferedMouse ) :
        m_dongleCheckTime( 0.0 ),
        m_lastInputTime( 0.0 ),
        m_inputTime( 0.0 ),
        m_lastClickTime( 0.0 ),
        m_doubleClickInterval( 0.25 ),
        m_nextDongleCheckTime( 0.0 ),
        m_inputIndex( 0 ),
        m_btnID( 0 ),
        m_dongleOk( false ),
        m_txOk( false ),
        m_keyboardInputEnabled( true ),
        m_status( 0 ),
        m_isAssigning( false ),
        m_isShiftPressed( false ),
        m_bUseOverride( false ),
        m_bEnableInputLog( false ),
        m_bRunInputLog( false ),
        m_bufferedKeys( bufferedKeys ),
        m_bufferedMouse( bufferedMouse ),
        m_enableInternalInputCapture( true ),
        m_isLeftPressed( false ),
        m_isRightPressed( false ),
        m_isMiddlePressed( false ),
        m_numButtons( 0 ),
        m_updateState( UpdateIdle ),
        m_previousButtons( 0 ),
        m_currentButtons( 0 ),
        m_jsThrottleChannel( 0 ),
        m_adapterModeHash( 0 ),
        m_nextJoystickCheck( 0.0 ),
        m_gyroOverride( 0.0f )
    {
        (void)window;
        m_timer = SmartPtr<Timer>( new TimerBoost );
        setCurrentTxModel( "heli" );
        createChannelData();
    }

    std::array<unsigned char, 65> InputManagerExt::getDataArray()
    {
        return {};
    }

    bool InputManagerExt::isDeviceUSB() const
    {
        return ( m_adapterModeHash == interlink_hash || m_adapterModeHash == vbar_hash ||
                 m_adapterModeHash == gamepad_hash || m_adapterModeHash == jeti_usb_hash );
    }

    SmartPtr<IGameInput> InputManagerExt::addGameInput( hash32 id )
    {
        return nullptr;
    }

    SmartPtr<IGameInput> InputManagerExt::findGameInput( hash32 id ) const
    {
        return nullptr;
    }

    Array<SmartPtr<IGameInput>> InputManagerExt::getGameInputs() const
    {
        return Array<SmartPtr<IGameInput>>();
    }

    bool InputManagerExt::isCursorVisible() const
    {
        return false;
    }

    void InputManagerExt::setCursorVisible( bool visible )
    {
    }

    bool InputManagerExt::postEvent( SmartPtr<IInputEvent> event )
    {
        return false;
    }

    bool InputManagerExt::checkAdaptorMode( void )
    {
        return false;
    }

    InputManagerExt::~InputManagerExt()
    {
        unload( 0 );
    }

    void InputManagerExt::sendKeyEvent( IInputEvent::EventType evnt, s32 keycode )
    {
        //SmartPtr<InputEvent> inputEvent; // = new InputEvent();
        //inputEvent->setEventType( evnt );
        //inputEvent->setKeyCode( keycode );
        //inputEvent->setNativeKeyCode( keycode );
        //inputEvent->setShiftPressed( isShiftPressed() );
        //m_inputEvents.push( inputEvent );
    }

    void InputManagerExt::preUpdate( TaskId task, const double &t, const double &dt )
    {
        if( getLoadingState() != LoadingState::Loaded )
        {
            return;
        }

        try
        {
            switch( task )
            {
            case TaskId::Primary:
            {
                updateState( task, t, dt );
            }
            break;
            case TaskId::Physics:
            {
            }
            break;
            case TaskId::Controls:
            {
            }
            break;
            default:
            {
            }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( e.what() );
        }
        catch( ... )
        {
            WP_LOG_ERROR( "Unhandled exception." );
        }
    }

    void InputManagerExt::updateState( TaskId task, const double &t, const double &dt )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto timer = applicationManager->getTimer();

        // if (!systemSettings->isValid())
        //{
        //	auto ret = applicationManager->verifyContent();
        //	if (ret != 0)
        //	{
        //		applicationManager->verifyAssets();
        //	}

        //	systemSettings->setValid(ret == 0);
        //}

        if( m_nextDongleCheckTime < t )
        {
            m_nextDongleCheckTime = t + 5.0;
        }

        // s32 status = adaptorTxStatusInt();
        // if (status != applicationManager->getAdapterStatus())
        //{
        //	applicationManager->setAdapterStatus(status);
        // }

        // SmartPtr<DebugTextManager> debugTextManager = applicationManager->getDebugTextManager();
        // if (debugTextManager)
        //{
        //	debugTextManager->displayText(16156165, 0, 0, getCurrentTxModel());
        // }

        if( m_inputConfiguration )
        {
            m_inputConfiguration->preUpdate( task, t, dt );
        }

        // if (!m_inputEvents.empty())
        //{
        //	SmartPtr<IEventListener> inputEvent;
        //	while (m_inputEvents.try_pop(inputEvent))
        //	{
        //		triggerEvent(inputEvent);
        //	}
        // }

        // if (!m_inputEventQueue.empty())
        //{
        //	SmartPtr<IInputEvent> inputEvent;
        //	while (m_inputEventQueue.try_pop(inputEvent))
        //	{
        //		triggerEvent(inputEvent);
        //	}
        // }

        //        auto appFSM = applicationManager->getFSM();
        //        auto appState = appFSM->getState<
        //            ApplicationTypes::
        //            ApplicationState>(); // static_cast<SaracenTypes::ApplicationState>(appFSM->
        //        //    getCurrentState());
        //        if(appFSM->getStateTime() > 1.0)
        //        {
        //            if(appState != ApplicationTypes::WP_STATE_LOADING)
        //            {
        //                addInputTime( dt );
        //
        //                // if (getRunInputLog())
        //                //{
        //                //	if (m_inputIndex < m_inputEvents.size())
        //                //	{
        //                //		SmartPtr<InputEvent> inputEvent = m_inputEvents[m_inputIndex];
        //                //		if (inputEvent->getTimeStamp() < getInputTime())
        //                //		{
        //                //			triggerEvent(inputEvent);
        //                //			++m_inputIndex;
        //                //		}
        //                //	}
        //                // }
        //            }
        //
        //            updateListenersPriority();
        //
        //            if(applicationManager->isPauseMenuActive())
        //            {
        //                updateDongle( t, dt );
        //            }
        //            else if(!( m_adapterModeHash == interlink_hash || m_adapterModeHash == vbar_hash ||
        //                       m_adapterModeHash == gamepad_hash || m_adapterModeHash == jeti_usb_hash ))
        //            {
        //                updateDongle( t, dt );
        //            }
        //
        //            if(m_adapterModeHash == interlink_hash || m_adapterModeHash == vbar_hash ||
        //               m_adapterModeHash == gamepad_hash || m_adapterModeHash == jeti_usb_hash)
        //            {
        //                if(appFSM->getStateTime() > 3.0)
        //                {
        //                    if(m_nextJoystickCheck < timer->now())
        //                    {
        //                        // if (isDongleOk())
        //                        {
        //                            auto applicationManager = core::IApplicationManager::instance();
        //                            auto pluginInterface = applicationManager->getPluginInterface();
        //                            if(!pluginInterface->getEnableDirectInput())
        //                            {
        //#if WP_ENABLE_LIBST
        //                                if( !m_joystickWinMM )
        //                                {
        //                                    createJoystick();
        //                                }
        //#endif
        //                            }
        //                        }
        //
        //                        m_nextJoystickCheck = timer->now() + 3.0;
        //                    }
        //                }
        //
        //                if(pluginInterface->getEnableDirectInput())
        //                {
        //                    updateJoystick();
        //                }
        //                else if(appState != ApplicationTypes::WP_STATE_FLIGHT &&
        //                        appState != ApplicationTypes::WP_STATE_WORK_BENCH)
        //                {
        //                    updateJoystick();
        //                }
        //
        //                updateInputState();
        //            }
        //            else
        //            {
        //                updateInputState();
        //            }
        //
        //            UpdateMiscFunctions();
        //        }
    }

    void InputManagerExt::updateInputState()
    {
        //try
        //{
        //    auto applicationManager = core::IApplicationManager::instance();
        //    auto appFSM = applicationManager->getFSM();
        //    auto appState =
        //        static_cast<ApplicationTypes::ApplicationState>(appFSM->getCurrentState());

        //    switch(appState)
        //    {
        //    case ApplicationTypes::WP_STATE_PLAYBACK:
        //    {
        //    }
        //    break;
        //    default:
        //    {
        //        auto pPlayerInputState = getPlayerInputState();
        //        auto &playerInputState = *pPlayerInputState;

        //        playerInputState.numButtons = getNumButtons();
        //        // playerInputState.buttonValues = getCurrentButtons();
        //        // playerInputState.prevButtonValues = getPreviousButtons();

        //        playerInputState.prevButtonValues = playerInputState.buttonValues;
        //        playerInputState.buttonValues = getCurrentButtons();

        //        // OIS::JoyStickState joystickState = getJoyStickState();
        //        // for (size_t i = 0; i < joystickState.mAxes.size(); ++i)
        //        //{
        //        //	f32 channelValue = (float)joystickState.mAxes[i].abs;
        //        //	playerInputState.rawAxes[i] = channelValue / 32768.0f;
        //        // }

        //        const size_t numAxes = 128;
        //        for(size_t i = 0; i < numAxes; ++i)
        //        {
        //            f32 channelValue = getChannel( static_cast<s32>(i) );
        //            playerInputState.axes[i] = channelValue;

        //            playerInputState.prevRawAxes[i] = playerInputState.rawAxes[i];

        //            f32 rawChannelValue = getChannelRaw( static_cast<int>(i) );
        //            playerInputState.rawAxes[i] = rawChannelValue;
        //        }

        //        playerInputState.axisCount = numAxes;

        //        boost::shared_ptr<ConcurrentArray<SmartPtr<ChannelData>>> pChannelData =
        //            getChannelData();
        //        if(pChannelData)
        //        {
        //            ConcurrentArray<SmartPtr<ChannelData>> &channelData = *pChannelData;

        //            for(size_t i = 0; i < channelData.size(); ++i)
        //            {
        //                const SmartPtr<ChannelData> &channelInstance = channelData[i];
        //                if(channelInstance)
        //                {
        //                    if(channelInstance->getFunctionHash() == 0)
        //                    {
        //                        continue;
        //                    }

        //                    s32 channel = channelInstance->getChannel() - 1;
        //                    s32 cmap = channelInstance->getCmap();

        //                    f32 channelValue = getChannel( cmap );

        //                    if(channel >= 0 && channel < PlayerInputStateData::NUM_AXES)
        //                    {
        //                        playerInputState.values[channel] = channelInstance->getFunctionHash();
        //                        playerInputState.channel[channel] = channelInstance->getChannel();
        //                        playerInputState.map[channel] = channelInstance->getCmap();
        //                        playerInputState.reverse[channel] =
        //                            channelInstance->isReversed() ? 1 : 0;

        //                        // f32 chValue = channelValue;// +channelInstance->getOffset();
        //                        // f32 offset = 0.0f;// channelInstance->getOffset() * (1.0f / 0.8f);
        //                        // if (chValue < 0.0f)
        //                        //{
        //                        //	chValue = chValue * (channelInstance->getLowMultiplier() + offset);
        //                        // }
        //                        // else
        //                        //{
        //                        //	chValue = chValue * (channelInstance->getHighMultiplier() + offset);
        //                        // }

        //                        playerInputState.functions[channel] = channelValue;
        //                    }
        //                }
        //            }

        //            // for (size_t i = 0; i < SaracenPlayerInputState::NUM_BUTTONS; ++i)
        //            //{
        //            //	playerInputState.buttons[i] = state.mButtons[i] == true ? 1 : 0;
        //            // }

        //            setPlayerInputState( pPlayerInputState );
        //        }
        //    }
        //    }
        //}
        //catch(std::exception &e)
        //{
        //    WP_LOG_ERROR( e.what() );
        //}
    }

    void InputManagerExt::updateDongle( const double &t, const double &dt )
    {
        if( m_dongleCheckTime < t && m_updateState == UpdateIdle )
        {
            if( !isDongleOk() )
            {
#if WP_USE_THREADED_DONGLE_POLLING
                // SmartPtr<IJob> startDongleJob(WP_NEW StartDongleJob(this));

                // auto& jobQueue = applicationManager->getJobQueue();
                // if ( jobQueue )
                //{
                //	jobQueue->queueJob(startDongleJob);
                // }

                startDongle();
#else
                startDongle();
#endif
            }

            // bool bHasTx = this->adaptorTxStatusInt() != 0xF3 ? false : true;
            // if (!bHasTx)
            //{
            //	if (!m_joystick)
            //	{
            //		createJoystick();
            //	}
            // }
            // else
            //{
            //	if (m_joystick)
            //	{
            //		destroyJoystick();
            //	}
            // }

            // adaptorIndicator();
            // txIndicator();
            m_dongleCheckTime = t + 1.0;
        }

        if( m_updateState > UpdateIdle )
        {
            updateFirmware( t, dt );
        }
    }

    void InputManagerExt::updateJoystick()
    {
        // Platform input backends feed normalized values through setAxisValue().
    }

    bool InputManagerExt::isFunction( SmartPtr<InputFunction> &func )
    {
        u32 previousButtons = getPreviousButtons();
        u32 currentButtons = getCurrentButtons();
        u32 changedButtons = previousButtons ^ currentButtons;

        if( func )
        {
            if( func->isAxis() )
            {
                s32 channel = func->getMapId();
                f32 channelValue = getAxisValue( channel );

                func->setChannelValue( channelValue );
                const bool hasChanged =
                    !MathF::equals( func->getPrevChannelValue(), func->getChannelValue() );
                if( hasChanged )
                {
                    if( !func->isReversed() )
                    {
                        if( channelValue > 0.7f )
                        {
                            hash_type functionHash = func->getFunctionHash();
                            if( functionHash == NORMAL_HASH )
                            {
                                return true;
                            }
                            if( functionHash == THROTTLE_HOLD_HASH )
                            {
                                return true;
                            }
                            if( functionHash == IDLE_UP_HASH )
                            {
                                return true;
                            }
                        }
                        else
                        {
                            hash_type functionHash = func->getFunctionHash();
                            if( functionHash == NORMAL_HASH )
                            {
                                return false;
                            }
                            if( functionHash == THROTTLE_HOLD_HASH )
                            {
                                return false;
                            }
                            if( functionHash == IDLE_UP_HASH )
                            {
                                return false;
                            }
                        }
                    }
                    else
                    {
                        if( MathF::equals( channelValue, 0.0f ) )
                        {
                            hash_type functionHash = func->getFunctionHash();
                            if( functionHash == NORMAL_HASH )
                            {
                                return true;
                            }
                            if( functionHash == THROTTLE_HOLD_HASH )
                            {
                                return true;
                            }
                            if( functionHash == IDLE_UP_HASH )
                            {
                                return true;
                            }
                        }
                        else
                        {
                            hash_type functionHash = func->getFunctionHash();
                            if( functionHash == NORMAL_HASH )
                            {
                                return false;
                            }
                            if( functionHash == THROTTLE_HOLD_HASH )
                            {
                                return false;
                            }
                            if( functionHash == IDLE_UP_HASH )
                            {
                                return false;
                            }
                        }
                    }
                }
            }
            else
            {
                s32 buttonIdx = func->getMapId();
                if( buttonIdx >= 0 )
                {
                    // if (((changedButtons >> buttonIdx) & 0x01) == 1)
                    {
                        if( ( ( m_currentButtons >> buttonIdx ) & 0x01 ) == 1 )
                        {
                            if( !func->isReversed() )
                            {
                                hash_type functionHash = func->getFunctionHash();
                                if( functionHash == NORMAL_HASH )
                                {
                                    return true;
                                }
                                if( functionHash == THROTTLE_HOLD_HASH )
                                {
                                    return true;
                                }
                                if( functionHash == IDLE_UP_HASH )
                                {
                                    return true;
                                }
                            }
                            else
                            {
                                hash_type functionHash = func->getFunctionHash();
                                if( functionHash == NORMAL_HASH )
                                {
                                    return false;
                                }
                                if( functionHash == THROTTLE_HOLD_HASH )
                                {
                                    return false;
                                }
                                if( functionHash == IDLE_UP_HASH )
                                {
                                    return false;
                                }
                            }
                        }
                        else
                        {
                            if( func->isReversed() )
                            {
                                hash_type functionHash = func->getFunctionHash();
                                if( functionHash == NORMAL_HASH )
                                {
                                    return true;
                                }
                                if( functionHash == THROTTLE_HOLD_HASH )
                                {
                                    return true;
                                }
                                if( functionHash == IDLE_UP_HASH )
                                {
                                    return true;
                                }
                            }
                            else
                            {
                                hash_type functionHash = func->getFunctionHash();
                                if( functionHash == NORMAL_HASH )
                                {
                                    return false;
                                }
                                if( functionHash == THROTTLE_HOLD_HASH )
                                {
                                    return false;
                                }
                                if( functionHash == IDLE_UP_HASH )
                                {
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    void InputManagerExt::UpdateMiscFunctions()
    {
        // return;

        // auto applicationManager = core::IApplicationManager::instance();
        // auto systemSettings =
        // fb::static_pointer_cast<SystemSettings>(applicationManager->getSystemSettings());
        // SmartPtr<Saracen> app = applicationManager->getApplication();

        // u32 previousButtons = getPreviousButtons();
        // u32 currentButtons = getCurrentButtons();
        // u32 changedButtons = previousButtons ^ currentButtons;

        // bool isThrottleHold = false;
        // bool isNormalMode = false;
        // bool isIdleUp = false;

        // if (m_throttleHoldFunction)
        //{
        //	isThrottleHold = isFunction(m_throttleHoldFunction);
        // }

        // if (m_normalModeFunction)
        //{
        //	isNormalMode = isFunction(m_normalModeFunction);
        // }

        // if (m_idleUpFunction)
        //{
        //	isIdleUp = isFunction(m_idleUpFunction);
        // }

        // s32 switchValue = 1;

        // if (isThrottleHold)
        //{
        //	switchValue = 0;
        // }
        // else if (isIdleUp)
        //{
        //	switchValue = 1;
        // }
        // else if (isNormalMode)
        //{
        //	switchValue = 2;
        // }

        // switch (switchValue)
        //{
        // case 0:
        //{
        //	setGyroOverride(-0.8);
        // }
        // break;
        // case 1:
        //{
        //	setGyroOverride(0.0f);
        // }
        // break;
        // case 2:
        //{
        //	setGyroOverride(0.8);
        // }
        // break;
        // }

        // if (isThrottleHold)
        //{
        //	//if (!app->isThrottleHold())
        //	//{
        //	//	app->setupThrottleHold();
        //	//}

        //	setGyroOverride(-0.8);
        //}
        // else if (isNormalMode)
        //{
        //	//if (!app->isNormalMode())
        //	//{
        //	//	app->setupNormalMode();
        //	//}

        //	setGyroOverride(0.0f);
        //}
        // else if (isIdleUp)
        //{
        //	//if (!app->isIdleUp())
        //	//{
        //	//	app->setupIdleUp();
        //	//}

        //	setGyroOverride(0.8f);
        //}

        // ConcurrentArray<SmartPtr<InputManager::Function>> functions = getMiscFunctions();
        // ConcurrentArray<SmartPtr<InputManager::Function>>::iterator it = functions.begin();
        // for (; it != functions.end(); ++it)
        //{
        //	SmartPtr<InputManager::Function>& func = *it;
        //	if (func)
        //	{
        //		if (func->isAxis())
        //		{
        //			s32 channel = func->cmap();
        //			f32 channelValue = getChannel(channel);

        //			func->setChannelValue(channelValue);
        //			bool hasChanged = !MathUtil<real_Num>::equals(func->getPrevChannelValue(),
        //func->getChannelValue()); 			if (hasChanged)
        //			{
        //				if (!func->isReversed())
        //				{
        //					if (channelValue > 0.7f)
        //					{
        //						s32 functionHash = func->getFunctionHash();
        //						if (functionHash == NORMAL_HASH)
        //						{
        //							sendKeyEvent(InputEvent::ET_KEY_DOWN, s32::KC_N);
        //						}
        //						else if (functionHash == THROTTLE_HOLD_HASH)
        //						{
        //							sendKeyEvent(InputEvent::ET_KEY_DOWN, s32::KC_H);
        //						}
        //						else if (functionHash == IDLE_UP_HASH)
        //						{
        //							sendKeyEvent(InputEvent::ET_KEY_DOWN, s32::KC_I);
        //						}
        //					}
        //				}
        //				else
        //				{
        //					if (MathUtil<real_Num>::equals(channelValue, 0.0f))
        //					{
        //						s32 functionHash = func->getFunctionHash();
        //						if (functionHash == NORMAL_HASH)
        //						{
        //							sendKeyEvent(InputEvent::ET_KEY_DOWN, s32::KC_N);
        //						}
        //						else if (functionHash == THROTTLE_HOLD_HASH)
        //						{
        //							sendKeyEvent(InputEvent::ET_KEY_DOWN, s32::KC_H);
        //						}
        //						else if (functionHash == IDLE_UP_HASH)
        //						{
        //							sendKeyEvent(InputEvent::ET_KEY_DOWN, s32::KC_I);
        //						}
        //					}
        //				}
        //			}
        //		}
        //		else
        //		{
        //			s32 buttonIdx = func->cmap();
        //			if (((changedButtons >> buttonIdx) & 0x01) == 1)
        //			{
        //				if (((m_currentButtons >> buttonIdx) & 0x01) == 1)
        //				{
        //					if (!func->isReversed())
        //					{
        //						s32 functionHash = func->getFunctionHash();
        //						if (functionHash == NORMAL_HASH)
        //						{
        //							sendKeyEvent(InputEvent::ET_KEY_DOWN, s32::KC_N);
        //						}
        //						else if (functionHash == THROTTLE_HOLD_HASH)
        //						{
        //							sendKeyEvent(InputEvent::ET_KEY_DOWN, s32::KC_H);
        //						}
        //						else if (functionHash == IDLE_UP_HASH)
        //						{
        //							sendKeyEvent(InputEvent::ET_KEY_DOWN, s32::KC_I);
        //						}
        //					}
        //				}
        //				else
        //				{
        //					if (func->isReversed())
        //					{
        //						s32 functionHash = func->getFunctionHash();
        //						if (functionHash == NORMAL_HASH)
        //						{
        //							sendKeyEvent(InputEvent::ET_KEY_DOWN, s32::KC_N);
        //						}
        //						else if (functionHash == THROTTLE_HOLD_HASH)
        //						{
        //							sendKeyEvent(InputEvent::ET_KEY_DOWN, s32::KC_H);
        //						}
        //						else if (functionHash == IDLE_UP_HASH)
        //						{
        //							sendKeyEvent(InputEvent::ET_KEY_DOWN, s32::KC_I);
        //						}
        //					}
        //				}
        //			}
        //		}
        //	}
        //}
    }

    void InputManagerExt::setupTxFunctions()
    {
    }

    void InputManagerExt::initDongle( const String &param )
    {
    }

    void InputManagerExt::getSignature( SmartPtr<IDatabaseManager> database )
    {
        WP_LOG_WARNING(
            "InputManagerExt::getSignature - Method deprecated. Use Properties for configuration "
            "instead." );
        // This method has been deprecated. Previously used for hardware authentication.
        // For production use, consider implementing a callback-based authentication system
        // or configure authentication parameters through the Properties interface.
    }

    void InputManagerExt::getChipId( char *hex, SmartPtr<IDatabaseManager> database )
    {
        if( !hex )
        {
            WP_LOG_ERROR( "InputManagerExt::getChipId - Null hex buffer pointer provided" );
            return;
        }

        WP_LOG_WARNING(
            "InputManagerExt::getChipId - Method deprecated. Use Properties for configuration "
            "instead." );
        // This method has been deprecated. Previously used for hardware identification.
        // For production use, implement chip ID retrieval through device-specific APIs
        // and expose the result through Properties or a dedicated callback interface.
    }

    void InputManagerExt::setupChannelMap( const String &param )
    {
    }

    String InputManagerExt::getModelTypeFromParam( const String &param )
    {
        WP_ASSERT( !StringUtil::isNullOrEmpty( param ) );

        auto applicationManager = core::IApplicationManager::instance();

        String modelType = "";

        if( param.find( "Heli" ) != String::npos || param.find( "heli" ) != String::npos )
        {
            modelType = "heli";
        }
        else if( param.find( "Plane" ) != String::npos || param.find( "plane" ) != String::npos )
        {
            modelType = "plane";
        }
        else if( param.find( "Drone" ) != String::npos || param.find( "drone" ) != String::npos )
        {
            modelType = "drone";
        }
        else if( param.find( "Car" ) != String::npos || param.find( "car" ) != String::npos )
        {
            modelType = "car";
        }
        else if( param.find( "Truck" ) != String::npos || param.find( "truck" ) != String::npos )
        {
            modelType = "truck";
        }
        else
        {
            //SmartPtr<Pilot> pilot = applicationManager->getComponent<Pilot>();
            //if( pilot )
            //{
            //    SmartPtr<Model> model = pilot->getVehicle();
            //    if( model )
            //    {
            //        modelType = model->getModelTypeAsString();
            //    }
            //    else
            //    {
            //        modelType = "heli";
            //    }
            //}
            //else
            //{
            //    modelType = "heli";
            //}
        }

        if( StringUtil::isNullOrEmpty( modelType ) )
        {
            modelType = "heli";
        }

        WP_ASSERT( !StringUtil::isNullOrEmpty( modelType ) );
        return modelType;
    }

    void InputManagerExt::auth()
    {
    }

    void InputManagerExt::setupButtons( String type )
    {
        try
        {
            if( type.empty() )
            {
                WP_LOG_ERROR( "InputManagerExt::setupButtons - Empty type string provided" );
                return;
            }

            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager )
            {
                WP_LOG_ERROR( "InputManagerExt::setupButtons - Application manager is null" );
                return;
            }

            SmartPtr<DatabaseManager> db = applicationManager->getDatabase();
            if( !db )
            {
                WP_LOG_WARNING(
                    "InputManagerExt::setupButtons - Database not available. Button mapping will use "
                    "defaults. Consider setting via Properties instead." );
                // Initialize with default empty button arrays
                m_buttonsON.clear();
                m_buttonsOFF.clear();
                m_buttonsON.resize( 7, 0 );
                m_buttonsOFF.resize( 7, 0 );
                return;
            }

            auto sql = "select param, value from settings where param like '" + type + "%'";
            auto result = db->executeQuery( sql );

            m_buttonsON.clear();
            m_buttonsOFF.clear();
            m_buttonsON.resize( 7, 0 );
            m_buttonsOFF.resize( 7, 0 );

            if( result )
            {
                while( !result->eof() )
                {
                    String param = result->getFieldValue( "param" );
                    String value = result->getFieldValue( "value" );

                    if( param.length() <= type.length() )
                    {
                        WP_LOG_WARNING( "InputManagerExt::setupButtons - Invalid param length: " +
                                        param );
                        result->nextRow();
                        continue;
                    }

                    param = param.substr( type.length() );

                    if( value.empty() )
                    {
                        WP_LOG_WARNING( "InputManagerExt::setupButtons - Empty value for param: " +
                                        param );
                        result->nextRow();
                        continue;
                    }

                    char keyChar = value.c_str()[0];
                    s32 keyCode = getKeyCode( keyChar );

                    // Map button on states
                    if( !param.compare( "B1on" ) && m_buttonsON.size() > 0 )
                        m_buttonsON[0] = keyCode;
                    else if( !param.compare( "B2on" ) && m_buttonsON.size() > 1 )
                        m_buttonsON[1] = keyCode;
                    else if( !param.compare( "B3on" ) && m_buttonsON.size() > 2 )
                        m_buttonsON[2] = keyCode;
                    else if( !param.compare( "B4on" ) && m_buttonsON.size() > 3 )
                        m_buttonsON[3] = keyCode;
                    else if( !param.compare( "B5on" ) && m_buttonsON.size() > 4 )
                        m_buttonsON[4] = keyCode;
                    else if( !param.compare( "B27on" ) && m_buttonsON.size() > 5 )
                        m_buttonsON[5] = keyCode;
                    else if( !param.compare( "B29on" ) && m_buttonsON.size() > 6 )
                        m_buttonsON[6] = keyCode;
                    // Map button off states
                    else if( !param.compare( "B1off" ) && m_buttonsOFF.size() > 0 )
                        m_buttonsOFF[0] = keyCode;
                    else if( !param.compare( "B2off" ) && m_buttonsOFF.size() > 1 )
                        m_buttonsOFF[1] = keyCode;
                    else if( !param.compare( "B3off" ) && m_buttonsOFF.size() > 2 )
                        m_buttonsOFF[2] = keyCode;
                    else if( !param.compare( "B4off" ) && m_buttonsOFF.size() > 3 )
                        m_buttonsOFF[3] = keyCode;
                    else if( !param.compare( "B5off" ) && m_buttonsOFF.size() > 4 )
                        m_buttonsOFF[4] = keyCode;
                    else if( !param.compare( "B27off" ) && m_buttonsOFF.size() > 5 )
                        m_buttonsOFF[5] = keyCode;
                    else if( !param.compare( "B29off" ) && m_buttonsOFF.size() > 6 )
                        m_buttonsOFF[6] = keyCode;

                    result->nextRow();
                }
            }
            else
            {
                WP_LOG_WARNING(
                    "InputManagerExt::setupButtons - No button configuration found for type: " + type );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "InputManagerExt::setupButtons - Exception: " + String( e.what() ) );
            // Initialize with safe defaults on error
            m_buttonsON.clear();
            m_buttonsOFF.clear();
            m_buttonsON.resize( 7, 0 );
            m_buttonsOFF.resize( 7, 0 );
        }
    }

    s32 InputManagerExt::getKeyCode( char c )
    {
        // switch (c)
        //{
        // case 'D':
        //	return OIS::KC_D;
        //	break;
        // case 'H':
        //	return OIS::KC_H;
        //	break;
        // case 'I':
        //	return OIS::KC_I;
        //	break;
        // case 'N':
        //	return OIS::KC_N;
        //	break;
        // case 'R':
        //	return OIS::KC_R;
        //	break;
        // default:
        //	break;
        // }

        return 0;
    }

    //
    // bool InputManager::keyPressed(const OIS::KeyEvent& e)
    //{
    //	try
    //	{
    //		if (!m_keyboardInputEnabled)
    //		{
    //			return true;
    //		}

    //		setLastInputTime(m_timer->now());

    //		if (OIS::KC_LSHIFT == e.key || OIS::KC_RSHIFT == e.key)
    //		{
    //			setShiftPressed(true);
    //		}

    //		SmartPtr<InputEvent> inputEvent = new InputEvent();
    //		inputEvent->setEventType(InputEvent::ET_KEY_DOWN);
    //		inputEvent->setKeyCode(e.key);
    //		inputEvent->setNativeKeyCode(e.text);
    //		inputEvent->setShiftPressed(isShiftPressed());

    //		triggerEvent(inputEvent);

    //		return true;
    //	}
    //	catch (std::exception& e)
    //	{
    //		WP_LOG_ERROR(e.what());
    //	}
    //	catch (...)
    //	{
    //		WP_LOG_ERROR("Unhandled exception.");
    //	}

    //	return false;
    //}

    //
    // bool InputManager::keyReleased(const OIS::KeyEvent& e)
    //{
    //	try
    //	{
    //		if (!m_keyboardInputEnabled)
    //			return true;

    //		setLastInputTime(m_timer->now());

    //		if (OIS::KC_LSHIFT == e.key || OIS::KC_RSHIFT == e.key)
    //		{
    //			setShiftPressed(false);
    //		}

    //		SmartPtr<InputEvent> inputEvent = new InputEvent();
    //		inputEvent->setEventType(InputEvent::ET_KEY_UP);
    //		inputEvent->setKeyCode(e.key);
    //		inputEvent->setNativeKeyCode(e.text);
    //		inputEvent->setShiftPressed(isShiftPressed());

    //		triggerEvent(inputEvent);

    //		return true;
    //	}
    //	catch (std::exception& e)
    //	{
    //		WP_LOG_ERROR(e.what());
    //	}
    //	catch (...)
    //	{
    //		WP_LOG_ERROR("Unhandled exception.");
    //	}

    //	return false;
    //}

    //
    // bool InputManager::mouseMoved(const OIS::MouseEvent& e)
    //{
    //	try
    //	{
    //		auto applicationManager = core::IApplicationManager::instance();
    //		auto systemSettings =
    //fb::static_pointer_cast<SystemSettings>(applicationManager->getSystemSettings()); 		SmartPtr<Timer>
    //timer = applicationManager->getTimer();

    //		if (systemSettings->getWindowActivationTime() + 0.1 > timer->now())
    //		{
    //			return false;
    //		}

    //		setLastInputTime(m_timer->now());

    //		SmartPtr<InputEvent> inputEvent = getMouseEvent(InputEvent::ET_MOUSE_MOVED);

    //		const OIS::MouseState& ms = e.state;
    //		inputEvent->setRelativePos(Vector2F((float)ms.X.rel, (float)ms.Y.rel));
    //		inputEvent->setAbsolutePos(Vector2F((float)ms.X.abs, (float)ms.Y.abs));

    //		inputEvent->setWheel((float)ms.Z.rel);

    //		inputEvent->setShiftPressed(isShiftPressed());

    //		triggerEvent(inputEvent);

    //		return true;
    //	}
    //	catch (std::exception& e)
    //	{
    //		WP_LOG_ERROR(e.what());
    //	}
    //	catch (...)
    //	{
    //		WP_LOG_ERROR("Unhandled exception.");
    //	}

    //	return false;
    //}

    //
    // bool InputManager::mousePressed(const OIS::MouseEvent& e, s32 id)
    //{
    //	try
    //	{
    //		auto applicationManager = core::IApplicationManager::instance();
    //		auto systemSettings =
    //fb::static_pointer_cast<SystemSettings>(applicationManager->getSystemSettings()); 		SmartPtr<Timer>
    //timer = applicationManager->getTimer();

    //		if (systemSettings->getWindowActivationTime() + 0.1 > timer->now())
    //		{
    //			return false;
    //		}

    //		setLastInputTime(m_timer->now());

    //		SmartPtr<InputEvent> inputEvent = getMouseEvent(InputEvent::ET_MOUSE_BTN_PRESSED);
    //		inputEvent->setMouseButtonID(id);
    //		inputEvent->setDoubleClick(m_lastClickTime + m_doubleClickInterval > timer->now());
    //		inputEvent->setShiftPressed(isShiftPressed());

    //		const OIS::MouseState& ms = e.state;
    //		inputEvent->setRelativePos(Vector2F((float)ms.X.rel, (float)ms.Y.rel));
    //		inputEvent->setAbsolutePos(Vector2F((float)ms.X.abs, (float)ms.Y.abs));
    //		//m_inputEvent->setLeftPressed(ms.buttonDown(OIS::MB_Left));
    //		//m_inputEvent->setMiddlePressed(ms.buttonDown(OIS::MB_Middle));
    //		//m_inputEvent->setRightPressed(ms.buttonDown(OIS::MB_Right));
    //		inputEvent->setWheel((float)ms.Z.rel);

    //		switch (id)
    //		{
    //		case OIS::MB_Left:
    //			setLeftPressed(true);
    //			break;
    //		case OIS::MB_Right:
    //			setRightPressed(true);
    //			break;
    //		case OIS::MB_Middle:
    //			setMiddlePressed(true);
    //			break;
    //		};

    //		m_lastClickTime = timer->now();

    //		triggerEvent(inputEvent);

    //		return true;
    //	}
    //	catch (std::exception& e)
    //	{
    //		WP_LOG_ERROR(e.what());
    //	}
    //	catch (...)
    //	{
    //		WP_LOG_ERROR("Unhandled exception.");
    //	}

    //	return false;
    //}

    //
    // bool InputManager::mouseReleased(const OIS::MouseEvent& e, s32 id)
    //{
    //	try
    //	{
    //		auto applicationManager = core::IApplicationManager::instance();
    //		auto systemSettings =
    //fb::static_pointer_cast<SystemSettings>(applicationManager->getSystemSettings()); 		SmartPtr<Timer>
    //timer = applicationManager->getTimer();

    //		if (systemSettings->getWindowActivationTime() + 0.1 > timer->now())
    //		{
    //			return false;
    //		}

    //		setLastInputTime(m_timer->now());

    //		SmartPtr<InputEvent> inputEvent = getMouseEvent(InputEvent::ET_MOUSE_BTN_RELEASED);
    //		inputEvent->setShiftPressed(isShiftPressed());
    //		inputEvent->setMouseButtonID(id);

    //		const OIS::MouseState& ms = e.state;
    //		inputEvent->setRelativePos(Vector2F((float)ms.X.rel, (float)ms.Y.rel));
    //		inputEvent->setAbsolutePos(Vector2F((float)ms.X.abs, (float)ms.Y.abs));

    //		switch (id)
    //		{
    //		case OIS::MB_Left:
    //			setLeftPressed(false);
    //			break;
    //		case OIS::MB_Right:
    //			setRightPressed(false);
    //			break;
    //		case OIS::MB_Middle:
    //			setMiddlePressed(false);
    //			break;
    //		};

    //		inputEvent->setWheel((float)ms.Z.rel);

    //		triggerEvent(inputEvent);

    //		return true;
    //	}
    //	catch (std::exception& e)
    //	{
    //		WP_LOG_ERROR(e.what());
    //	}
    //	catch (...)
    //	{
    //		WP_LOG_ERROR("Unhandled exception.");
    //	}

    //	return false;
    //}

    void InputManagerExt::reset()
    {
        try
        {
            WP_ASSERT( Thread::getCurrentTask() == TaskId::Primary );

            auto inputEvent = getResetEvent( static_cast<u32>( IInputEvent::InputType::Reset ) );

            auto inputListeners = getInputListeners();
            for( size_t i = 0; i < inputListeners.size(); ++i )
            {
                try
                {
                    SmartPtr<IEventListener> inputListener = inputListeners[i];
                    if( inputListener )
                    {
                        //if(inputListener->inputEvent( inputEvent ))
                        //{
                        //    break;
                        //}
                    }
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void InputManagerExt::resetDongle()
    {
        //auto applicationManager = core::IApplicationManager::instance();
        //SmartPtr<Pilot> pilot = applicationManager->getComponent<Pilot>();
        //if( pilot )
        //{
        //    SmartPtr<Model> model = pilot->getVehicle();
        //    if( model )
        //    {
        //        String modelTypeStr = model->getModelTypeAsString();
        //        String currentModelTypeStr = getCurrentTxModel();
        //        if( modelTypeStr != currentModelTypeStr )
        //        {
        //            initDongle( "setTxData" );
        //        }
        //    }
        //}
    }

    void InputManagerExt::resetDongleAuthenticate()
    {
    }

    void InputManagerExt::getDongleVersion()
    {
    }

    void InputManagerExt::setupBind( const String &param )
    {
    }

    void InputManagerExt::setWindowExtents( s32 width, s32 height )
    {
        // try
        //{
        //	// Set mouse region (if window resizes, we should alter this to reflect as well)
        //	if (m_mouse)
        //	{
        //		const OIS::MouseState& mouseState = m_mouse->getMouseState();
        //		mouseState.width = width;
        //		mouseState.height = height;
        //	}
        // }
        // catch (std::exception& e)
        //{
        //	WP_LOG_EXCEPTION(e);
        // }
    }

    SmartPtr<InputEvent> InputManagerExt::getMouseEvent( u32 eventType )
    {
        // try
        //{
        //	const OIS::MouseState& ms = m_mouse->getMouseState();

        //	SmartPtr<InputEvent> inputEvent = new InputEvent();
        //	inputEvent->setEventType(eventType);
        //	inputEvent->setRelativePos(Vector2F((float)ms.X.rel, (float)ms.Y.rel));
        //	inputEvent->setAbsolutePos(Vector2F((float)ms.X.abs, (float)ms.Y.abs));
        //	//m_inputEvent->setLeftPressed(ms.buttonDown(OIS::MB_Left));
        //	//m_inputEvent->setMiddlePressed(ms.buttonDown(OIS::MB_Middle));
        //	//m_inputEvent->setRightPressed(ms.buttonDown(OIS::MB_Right));
        //	inputEvent->setWheel((float)ms.Z.rel);

        //	return inputEvent;
        //}
        // catch (std::exception& e)
        //{
        //	WP_LOG_EXCEPTION(e);
        //}

        return nullptr;
    }

    SmartPtr<InputEvent> InputManagerExt::getResetEvent( u32 eventType )
    {
        // try
        //{
        //	SmartPtr<InputEvent> inputEvent = new InputEvent();
        //	inputEvent->setEventType(eventType);

        //	if (m_mouse)
        //	{
        //		const OIS::MouseState& ms = m_mouse->getMouseState();

        //		inputEvent->setRelativePos(Vector2F((float)ms.X.rel, (float)ms.Y.rel));
        //		inputEvent->setAbsolutePos(Vector2F((float)ms.X.abs, (float)ms.Y.abs));
        //		/*inputEvent->setLeftPressed(ms.buttonDown(OIS::MB_Left));
        //		inputEvent->setMiddlePressed(ms.buttonDown(OIS::MB_Middle));
        //		inputEvent->setRightPressed(ms.buttonDown(OIS::MB_Right));*/
        //		inputEvent->setWheel((float)ms.Z.rel);
        //	}

        //	return inputEvent;
        //}
        // catch (std::exception& e)
        //{
        //	WP_LOG_ERROR(e.what());
        //}
        // catch (...)
        //{
        //	WP_LOG_ERROR("Unhandled exception.");
        //}

        return nullptr;
    }

    void InputManagerExt::startDongle()
    {
    }

    void InputManagerExt::stopDongle()
    {
    }

    void InputManagerExt::setAxisValue( s32 channel, f32 value )
    {
        if( channel < 0 || channel >= NUM_CHANNELS )
        {
            WP_LOG_ERROR( "InputManagerExt::setAxisValue - Channel out of range [0, " +
                          StringUtil::toString( NUM_CHANNELS ) +
                          "): " + StringUtil::toString( channel ) );
            return;
        }

        if( std::isnan( value ) || std::isinf( value ) )
        {
            WP_LOG_ERROR( "InputManagerExt::setAxisValue - Invalid value (NaN or Inf) for channel " +
                          StringUtil::toString( channel ) );
            return;
        }

        m_externalChannelInput[channel] = value;
    }

    void InputManagerExt::setChannelData( s32 channel, f32 offset, f32 multiplier )
    {
    }

    void InputManagerExt::setChannelData( const ConcurrentArray<SmartPtr<AxisData>> &channelData )
    {
        m_channelData = channelData;
    }

    SmartPtr<InputConfiguration> InputManagerExt::getInputConfiguration() const
    {
        return m_inputConfiguration;
    }

    void InputManagerExt::setInputConfiguration( SmartPtr<InputConfiguration> inputConfiguration )
    {
        m_inputConfiguration = inputConfiguration;
    }

    ConcurrentArray<SmartPtr<AxisData>> InputManagerExt::getChannelDataMap() const
    {
        return m_channelDataMap;
    }

    void InputManagerExt::setChannelDataMap( const ConcurrentArray<SmartPtr<AxisData>> &channelDataMap )
    {
        auto msg = String( "InputManager::setChannelDataMap size: " ) +
                   StringUtil::toString( static_cast<s32>( channelDataMap.size() ) );
        WP_LOG( msg );
        m_channelDataMap = channelDataMap;
    }

    bool InputManagerExt::isAssigning() const
    {
        return m_isAssigning;
    }

    void InputManagerExt::setAssigning( bool assigning )
    {
        m_isAssigning = assigning;
    }

    f32 InputManagerExt::getAxisValue( s32 channel )
    {
        if( !getUseOverride() )
        {
            if( isDongleOk() && !isDeviceUSB() )
            {
                // m_txOk = CheckTxConnected();
                if( channel < 0 && channel > 7 )
                {
                    return 0.0f;
                }
                f32 modifiedChannelValue = 0.0f;

                if( channel >= 0 && channel < 8 )
                {
                    SmartPtr<AxisData> &channelInstance = getChannelDataByIndex( channel );
                    if( !channelInstance )
                    {
                        return 0.0f;
                    }

                    if( channelInstance->getFunctionHash() == StringUtil::getHash( "Gyro" ) )
                    {
                        if( channelInstance->getDeviceMap() == -1 )
                        {
                            return getGyroOverride();
                        }
                    }

                    // if (adapterModeHash == interlink_hash ||
                    //	adapterModeHash == vbar_hash ||
                    //	adapterModeHash == gamepad_hash ||
                    //	adapterModeHash == jeti_usb_hash)
                    {
                        modifiedChannelValue = modifiedChannelValue + channelInstance->getOffset();
                    }

                    modifiedChannelValue *= channelInstance->getMultiplier();

                    if( modifiedChannelValue < 0.0f )
                    {
                        modifiedChannelValue =
                            modifiedChannelValue * channelInstance->getLowMultiplier();
                    }
                    else
                    {
                        modifiedChannelValue =
                            modifiedChannelValue * channelInstance->getHighMultiplier();
                    }

                    return modifiedChannelValue;
                }
            }
            else
            {
            }
        }
        else
        {
            return static_cast<f32>( m_externalChannelInput[channel] );
        }

        return 0.0f;
    }

    f32 InputManagerExt::getAxisValueRaw( s32 channel )
    {
        if( !getUseOverride() )
        {
            if( isDongleOk() && !isDeviceUSB() )
            {
                // m_txOk = CheckTxConnected();
                if( channel < 0 && channel > 7 )
                {
                    return 0.0f;
                }
            }
            else
            {
                if( channel < 0 || channel > 7 )
                {
                    return 0.0f;
                }
            }
        }
        else
        {
            return static_cast<f32>( m_externalChannelInput[channel] );
        }

        return 0.0f;
    }

    void InputManagerExt::addListener( SmartPtr<IEventListener> listener )
    {
        if( !listener )
        {
            WP_LOG_ERROR( "InputManagerExt::addListener - Null listener pointer provided" );
            return;
        }

        try
        {
            auto inputListeners = getInputListeners();
            auto it = std::find( inputListeners.begin(), inputListeners.end(), listener );
            if( it == inputListeners.end() )
            {
                inputListeners.push_back( listener );
                setInputListeners( inputListeners );
            }
            else
            {
                WP_LOG_WARNING( "InputManagerExt::addListener - Listener already registered, skipping" );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "InputManagerExt::addListener - Exception: " + String( e.what() ) );
        }
    }

    void InputManagerExt::removeListener( SmartPtr<IEventListener> listener )
    {
        if( !listener )
        {
            WP_LOG_ERROR( "InputManagerExt::removeListener - Null listener pointer provided" );
            return;
        }

        try
        {
            auto initialSize = m_inputListeners.size();
            m_inputListeners.erase(
                std::remove( m_inputListeners.begin(), m_inputListeners.end(), listener ),
                m_inputListeners.end() );

            if( m_inputListeners.size() == initialSize )
            {
                WP_LOG_WARNING( "InputManagerExt::removeListener - Listener not found in list" );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "InputManagerExt::removeListener - Exception: " + String( e.what() ) );
        }
    }

    void InputManagerExt::removeListeners()
    {
        m_inputListeners.clear();
    }

    void InputManagerExt::triggerEvent( SmartPtr<IEventListener> inputEvent )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            // bool bEnableTextEntry = applicationManager->getEnableTextEntry();

            // applicationManager->handleTextEntryShortcuts(inputEvent);

            // inputEvent->setTimeStamp(getInputTime());

            if( getEnableInputLog() )
            {
                // String xmlStr = inputEvent->toXML();
                // m_log << xmlStr << std::endl;
            }

            auto inputListeners = getInputListeners();
            for( size_t i = 0; i < inputListeners.size(); ++i )
            {
#if !WP_FINAL
                double startTime = m_timer->now();
#endif

                SmartPtr<IEventListener> &elem = inputListeners[i];
                if( elem )
                {
                    // if (inputEvent->getEventType() == InputEvent::ET_KEY_DOWN ||
                    //	inputEvent->getEventType() == InputEvent::ET_KEY_UP)
                    //{
                    //	//if (bEnableTextEntry == elem->getEnableTextEntry())
                    //	//{
                    //	//	if (elem->inputEvent(inputEvent))
                    //	//	{
                    //	//		break;
                    //	//	}
                    //	//}
                    // }
                    // else
                    //{
                    //	//if (elem->inputEvent(inputEvent))
                    //	//{
                    //	//	break;
                    //	//}
                    // }
                }

                //#if !WP_FINAL
                //				double endTime = m_timer->now();
                //				double timeTaken = endTime - startTime;
                //
                //				String message = "Input listener time taken: " +
                //					StringUtil::toString((float)timeTaken) + " Index: " +
                //					StringUtil::toString(i) + " class type: " +
                //StringUtil::toString(elem->getClassTypeHash());
                //
                //				LogManager::getInstance().logMessage("input_profile", message);
                //#endif
            }
        }
        catch( std::exception &e )
        {
            String message = e.what();
            WP_LOG_ERROR( e.what() );
        }
        catch( ... )
        {
            WP_LOG_ERROR( "Unhandled exception." );
        }
    }

    void InputManagerExt::triggerEvent( SmartPtr<IInputEvent> inputEvent )
    {
        if( !inputEvent )
        {
            WP_LOG_ERROR( "InputManagerExt::triggerEvent - Null input event pointer provided" );
            return;
        }

        try
        {
            auto inputListeners = getInputListeners();
            if( inputListeners.empty() )
            {
                // No listeners registered, this is normal during initialization
                return;
            }

            for( auto &listener : inputListeners )
            {
                if( listener )
                {
                    try
                    {
                        // Call handleEvent with proper signature
                        // IEventListener expects: EventType, hash_type, Array<Parameter>, sender, object, IEvent
                        Array<Parameter> emptyArgs;
                        listener->handleEvent( EventType::Input, 0, emptyArgs, nullptr, nullptr,
                                               inputEvent );
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_ERROR( "InputManagerExt::triggerEvent - Listener exception: " +
                                      String( e.what() ) );
                        // Continue processing other listeners despite error
                    }
                }
                else
                {
                    WP_LOG_WARNING( "InputManagerExt::triggerEvent - Null listener in list" );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "InputManagerExt::triggerEvent - Exception: " + String( e.what() ) );
        }
    }

    void InputManagerExt::setShiftPressed( bool shiftPressed )
    {
        m_isShiftPressed = shiftPressed;
    }

    bool InputManagerExt::isShiftPressed() const
    {
        return m_isShiftPressed;
    }

    s32 InputManagerExt::getBtnID()
    {
        return m_btnID;
    }

    void InputManagerExt::setKeyboardInputEnabled( bool keyboardInputEnabled )
    {
        m_keyboardInputEnabled = keyboardInputEnabled;
    }

    bool InputManagerExt::getKeyboardInputEnabled() const
    {
        return m_keyboardInputEnabled;
    }

    String InputManagerExt::adaptorTxStatus()
    {
        String result;

#if ACCURX_NEW_VERSION
        s32 status = getStatus();

        switch( status )
        {
        case 0xf0:
            // no addaptor
            result = "<switch1>off</switch1><switch2>off</switch2>";
            setDongleOk( false );
            break;
        case 0xf1:
            // invalid cant have data without adaptor
            result = "<switch1>off</switch1><switch2>on</switch2>";
            setDongleOk( false );
            break;
        case 0xf2:
            // adaptor found
            result = "<switch1>on</switch1><switch2>off</switch2>";
            break;
        case 0xf3:
            // both adaptor and data
            result = "<switch1>on</switch1><switch2>on</switch2>";
            break;
        }
#endif

        return result;
    }

    s32 InputManagerExt::adaptorTxStatusInt()
    {
        return getStatus();
    }

    double InputManagerExt::getLastInputTime() const
    {
        return m_lastInputTime;
    }

    void InputManagerExt::setLastInputTime( double lastInputTime )
    {
        m_lastInputTime = lastInputTime;
    }

    bool InputManagerExt::getUseOverride() const
    {
        return m_bUseOverride;
    }

    void InputManagerExt::setUseOverride( bool useOverride )
    {
        m_bUseOverride = useOverride;
    }

    void InputManagerExt::StartDongleJob::execute()
    {
        m_inputManager->startDongle();
    }

    InputManagerExt::StartDongleJob::~StartDongleJob()
    {
        m_inputManager = nullptr;
    }

    InputManagerExt::StartDongleJob::StartDongleJob( InputManagerExt *inputManager ) :
        m_inputManager( inputManager )
    {
    }

    void InputManagerExt::updateFirmware( void )
    {
#if WP_UNITY_BUILD
#else
        m_updateState = StartUpdate;
#endif
    }

    void InputManagerExt::updateFirmware( const double &t, const double &dt )
    {
        /*
        static double counter = 0.0;
        s32 result = 0;
        s32 step = t * 1000;
        if (!(step % 60))
        {
            auto applicationManager = core::IApplicationManager::instance();
            String broadrcastMsgStart = "<broadcast tag='firmwareStatusUpdate' status='";
            String broadrcastMsgEnd = "' ></broadcast>";

            switch (m_updateState)
            {
            case StartUpdate: // put adapter into no_mode
            {
                static unsigned char data1[65] = { 0x00, no_mode };
                m_accurxProxy->Authenticate(data1);
                m_updateState = SendBootLoad;
                m_msg = "Starting Firmware Update";
                //applicationManager->getUI()->broadcastMessage(broadrcastMsgStart + m_msg +
        broadrcastMsgEnd);
                //applicationManager->getUI()->updateFirmwareStatus(m_msg);
                counter = t;
            }
            break;
            case SendBootLoad: // put adapter into bootload
            {
                if ((counter + 2.0) > t)
                {
                    printf("Wait for BootLoad: %f", (float)counter);
                    return;
                }

                static unsigned char data2[65] = { 0x00, bootload };
                m_accurxProxy->Authenticate(data2);
                m_msg = "Starting Bootloader";
                //applicationManager->getUI()->broadcastMessage(broadrcastMsgStart + m_msg +
        broadrcastMsgEnd);
                //applicationManager->getUI()->updateFirmwareStatus(m_msg);
                m_updateState = WaitingForBootload;
                counter = t;
            }
            break;
            case WaitingForBootload:
            {
                if (m_accurxProxy->CheckStatus() == 0xf0)
                {
                    // send broadcast msg
                    printf("Wait for BootLoad: %f", (float)counter);
                    if ((counter + 10.0) < t)
                    {
                        m_msg = "Update failed. Bootloader timeout!";
                        //applicationManager->getUI()->broadcastMessage(broadrcastMsgStart + m_msg +
        broadrcastMsgEnd);
                        //applicationManager->getUI()->updateFirmwareStatus(m_msg);

                        m_updateState = UpdateIdle;
                        setDongleOk(false);

                        return;
                    }
                    else
                    {
                        m_msg += ".";
                        //applicationManager->getUI()->broadcastMessage(broadrcastMsgStart + m_msg +
        broadrcastMsgEnd);
                        //applicationManager->getUI()->updateFirmwareStatus(m_msg);
                        counter = t;
                    }
                }
                else
                {
                    if ((counter + 1.0) > t)
                    {
                        return;
                    }
                    m_updateState = SendUpdate;
                    m_msg = "Updating Firmware";
                    //applicationManager->getUI()->broadcastMessage(broadrcastMsgStart + m_msg +
        broadrcastMsgEnd);
                    //applicationManager->getUI()->updateFirmwareStatus(m_msg);
                }
            }
            break;
            case SendUpdate:
            {
                result = m_accurxProxy->Update("firmware.enc");
                if (result > 0)
                {
                    // send broadcast msg
                    m_updateState = WaitingForUpdate;
                }
                else
                {
                    m_msg = "Update Failed";
                    //applicationManager->getUI()->broadcastMessage(broadrcastMsgStart + m_msg +
        broadrcastMsgEnd);
                    //applicationManager->getUI()->updateFirmwareStatus(m_msg);
                    m_updateState = UpdateFinished;
                }
            }
            break;
            case WaitingForUpdate:
            {
                if (m_accurxProxy->CheckStatus() == 0x55)
                {
                    // send broadcast msg
                    m_msg += ".";
                    //applicationManager->getUI()->broadcastMessage(broadrcastMsgStart + m_msg +
        broadrcastMsgEnd);
                    //applicationManager->getUI()->updateFirmwareStatus(m_msg);
                }
                else
                {
                    m_msg = "Update Complete";
                    //applicationManager->getUI()->broadcastMessage(broadrcastMsgStart + m_msg +
        broadrcastMsgEnd);
                    //applicationManager->getUI()->updateFirmwareStatus(m_msg);
                    m_updateState = UpdateFinished;

                }
            }
            break;
            case UpdateFinished:
                m_updateState = UpdateIdle;
                setDongleOk(false);
                break;
            }
        }
        */
    }

    void InputManagerExt::updateListenersPriority()
    {
        auto inputListeners = getInputListeners();
        Util::sortByPriority( inputListeners.begin(), inputListeners.end() );
    }

    void InputManagerExt::setRunInputLog( bool runInputLog )
    {
        m_bRunInputLog = runInputLog;
    }

    bool InputManagerExt::getRunInputLog() const
    {
        return m_bRunInputLog;
    }

    void InputManagerExt::addInputTime( double inputTime )
    {
        m_inputTime += inputTime;
    }

    void InputManagerExt::setInputTime( double inputTime )
    {
        m_inputTime = inputTime;
    }

    double InputManagerExt::getInputTime() const
    {
        return m_inputTime;
    }

    void InputManagerExt::setEnableInputLog( bool enableInputLog )
    {
        m_bEnableInputLog = enableInputLog;
    }

    bool InputManagerExt::getEnableInputLog() const
    {
#if WP_UNITY_BUILD
        return false;
#else
        return m_bEnableInputLog;
#endif
    }

    void InputManagerExt::setMiddlePressed( bool middlePressed )
    {
        m_isMiddlePressed = middlePressed;
    }

    bool InputManagerExt::isMiddlePressed() const
    {
        return m_isMiddlePressed;
    }

    void InputManagerExt::setRightPressed( bool rightPressed )
    {
        m_isRightPressed = rightPressed;
    }

    bool InputManagerExt::isRightPressed() const
    {
        return m_isRightPressed;
    }

    void InputManagerExt::setLeftPressed( bool leftPressed )
    {
        m_isLeftPressed = leftPressed;
    }

    bool InputManagerExt::isLeftPressed() const
    {
        return m_isLeftPressed;
    }

    Array<SmartPtr<IEventListener>> InputManagerExt::getInputListeners() const
    {
        return m_inputListeners.snapshot();
    }

    void InputManagerExt::setInputListeners( const Array<SmartPtr<IEventListener>> &p )
    {
        m_inputListeners = { p.begin(), p.end() };
    }

    void InputManagerExt::load()
    {
        try
        {
            if( getLoadingState() == LoadingState::Loaded )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            setDongleOk( false );
            m_txOk = false;

            for( size_t i = 0; i < 12; ++i )
            {
                m_externalChannelInput[static_cast<int>( i )] = 0.0;
            }

            setEnableInputLog( false );
            setRunInputLog( false );

            createRecorderListener();

            initDongle( "load" );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            WP_LOG( String( __FILE__ ) + String( " line: " ) + StringUtil::toString( __LINE__ ) );
        }
    }

    void InputManagerExt::createChannelData()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager )
            {
                WP_LOG_ERROR( "InputManagerExt::createChannelData - Application manager is null" );
                return;
            }

            auto factoryManager = applicationManager->getFactoryManager();
            if( !factoryManager )
            {
                WP_LOG_ERROR( "InputManagerExt::createChannelData - Factory manager is null" );
                return;
            }

            m_channelData.clear();
            m_channelData.resize( NUM_CHANNELS );

            for( s32 i = 0; i < NUM_CHANNELS; i++ )
            {
                SmartPtr<AxisData> channelInstance = factoryManager->make_ptr<AxisData>();
                if( !channelInstance )
                {
                    WP_LOG_ERROR(
                        "InputManagerExt::createChannelData - Failed to create AxisData for channel " +
                        StringUtil::toString( i ) );
                    continue;  // Skip this channel but continue with others
                }

                channelInstance->setOffset( 0.0 );
                channelInstance->setMultiplier( 1.0 );
                channelInstance->setLowMultiplier( 1.0 );
                channelInstance->setHighMultiplier( 1.0 );
                m_channelData[i] = channelInstance;
            }

            m_channelDataMap.clear();

            WP_LOG( "InputManagerExt::createChannelData - Successfully initialized " +
                    StringUtil::toString( NUM_CHANNELS ) + " channels" );
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "InputManagerExt::createChannelData - Exception: " + String( e.what() ) );
        }
    }

    void InputManagerExt::createJoystick()
    {
        // Device backends are provided by Workphone and publish normalized input events.
    }

    void InputManagerExt::createMouse()
    {
    }

    void InputManagerExt::createKeyboard()
    {
    }

    void InputManagerExt::unload( u32 flags )
    {
        try
        {
            stopDongle();
            m_inputListeners.clear();
            m_channelData.clear();
            m_channelDataMap.clear();
            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void InputManagerExt::destroyJoystick()
    {
    }

    void InputManagerExt::setEnableInternalInputCapture( bool enableInternalInputCapture )
    {
        m_enableInternalInputCapture = enableInternalInputCapture;
    }

    bool InputManagerExt::getEnableInternalInputCapture() const
    {
        return m_enableInternalInputCapture;
    }

    void InputManagerExt::setBufferedMouse( bool bufferedMouse )
    {
        m_bufferedMouse = bufferedMouse;
    }

    bool InputManagerExt::getBufferedMouse() const
    {
        return m_bufferedMouse;
    }

    void InputManagerExt::setBufferedKeys( bool bufferedKeys )
    {
        m_bufferedKeys = bufferedKeys;
    }

    bool InputManagerExt::getBufferedKeys() const
    {
        return m_bufferedKeys;
    }

    void InputManagerExt::setDoubleClickInterval( double doubleClickInterval )
    {
        m_doubleClickInterval = doubleClickInterval;
    }

    double InputManagerExt::getDoubleClickInterval() const
    {
        return m_doubleClickInterval;
    }

    void InputManagerExt::setLastClickTime( double lastClickTime )
    {
        m_lastClickTime = lastClickTime;
    }

    double InputManagerExt::getLastClickTime() const
    {
        return m_lastClickTime;
    }

    String InputManagerExt::getAsString( s32 kc )
    {
        // if (m_keyboard)
        //{
        //	return m_keyboard->getAsString(kc);
        // }

        return std::string( "" );

        // char temp[256];

        // DIPROPSTRING prop;
        // prop.diph.dwSize = sizeof(DIPROPSTRING);
        // prop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        // prop.diph.dwObj = static_cast<DWORD>(kc);
        // prop.diph.dwHow = DIPH_BYOFFSET;

        // if ( SUCCEEDED(mKeyboard->GetProperty(DIPROP_KEYNAME, &prop.diph)) )
        //{
        //	// convert the WCHAR in "wsz" to multibyte
        //	if ( WideCharToMultiByte(CP_ACP, 0, prop.wsz, -1, temp, sizeof(temp), NULL, NULL) )
        //		return mGetString.assign(temp);
        // }

        // std::stringstream ss;
        // ss << (int)kc;
        // return ss.str();
    }

    bool InputManagerExt::isKeyDown( s32 kc )
    {
        // if (m_keyboard)
        //{
        //	return m_keyboard->isKeyDown(kc);
        // }

        return false;
    }

    void InputManagerExt::TxChannels2db()
    {
        WP_LOG_WARNING(
            "InputManagerExt::TxChannels2db - Method deprecated. Channel data can be accessed via "
            "getChannelData() and persisted through Properties or custom serialization." );
        // This method has been deprecated. Previously used to persist channel data to database.
        // For production use, serialize channel configuration using Properties::getProperties()
        // and save to file, or implement a custom persistence callback system.
    }

    void InputManagerExt::StartChannelMonitor()
    {
    }

    void InputManagerExt::EndChannelMonitor()
    {
    }

    void InputManagerExt::queueEvent( SmartPtr<IInputEvent> inputEvent )
    {
        m_inputEventQueue.push( inputEvent );
    }

    s32 InputManagerExt::getStatus() const
    {
        return m_status;
    }

    void InputManagerExt::setStatus( s32 status )
    {
        m_status = status;
    }

    void InputManagerExt::joystickConnected()
    {
        destroyJoystick();
    }

    void InputManagerExt::joystickPreDisconnect()
    {
    }

    void InputManagerExt::joystickDisconnected()
    {
    }

    ConcurrentArray<SmartPtr<AxisData>> InputManagerExt::getChannelData() const
    {
        return m_channelData;
    }

    String InputManagerExt::getCurrentTxModel() const
    {
        return m_currentTxMode;
    }

    void InputManagerExt::setCurrentTxModel( const String &currentTxModel )
    {
        m_currentTxMode = currentTxModel;
    }

    void InputManagerExt::setCurrentButtons( u32 currentButtons )
    {
        m_currentButtons = currentButtons;
    }

    void InputManagerExt::setPreviousButtons( u32 previousButtons )
    {
        m_previousButtons = previousButtons;
    }

    s32 InputManagerExt::getNumButtons() const
    {
        return m_numButtons;
    }

    void InputManagerExt::setNumButtons( s32 numButtons )
    {
        m_numButtons = numButtons;
    }

    void InputManagerExt::setDongleOk( bool dongleOk )
    {
        m_dongleOk = dongleOk;
    }

    f32 InputManagerExt::getGyroOverride() const
    {
        return m_gyroOverride;
    }

    void InputManagerExt::setGyroOverride( f32 gyroOverride )
    {
        m_gyroOverride = gyroOverride;
    }

    void InputManagerExt::setupAdapterModeValue()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager )
            {
                WP_LOG_ERROR( "InputManagerExt::setupAdapterModeValue - Application manager is null" );
                return;
            }

            SmartPtr<DatabaseManager> database = applicationManager->getDatabase();
            if( !database )
            {
                WP_LOG_WARNING(
                    "InputManagerExt::setupAdapterModeValue - Database not available. Using default "
                    "adapter mode. Consider setting via Properties instead." );
                // Set default adapter mode if database is unavailable
                if( m_adapterMode.empty() )
                {
                    m_adapterMode = "gamepad";  // Default to generic gamepad mode
                    m_adapterModeHash = StringUtil::getHash( m_adapterMode );
                }
                return;
            }

            static const String VALUE_STR = "value";
            static const String sql = "select * from settings where param = 'adapter_mode' ";

            auto selectedRowQuery = database->executeQuery( sql );
            if( selectedRowQuery )
            {
                String newAdapterMode = selectedRowQuery->getFieldValue( VALUE_STR );
                if( !newAdapterMode.empty() )
                {
                    m_adapterMode = newAdapterMode;
                    m_adapterModeHash = StringUtil::getHash( m_adapterMode );
                    WP_LOG( "InputManagerExt::setupAdapterModeValue - Adapter mode set to: " +
                            m_adapterMode );
                }
                else
                {
                    WP_LOG_WARNING(
                        "InputManagerExt::setupAdapterModeValue - Empty adapter mode value in "
                        "database" );
                }
            }
            else
            {
                WP_LOG_WARNING(
                    "InputManagerExt::setupAdapterModeValue - No adapter_mode setting found in "
                    "database. Using current value: " +
                    m_adapterMode );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "InputManagerExt::setupAdapterModeValue - Exception: " + String( e.what() ) );
            // Continue with existing adapter mode value on error
        }
    }

    SmartPtr<AxisData> &InputManagerExt::getChannelDataByIndex( s32 index )
    {
        try
        {
            auto channelDataMap = getChannelDataMap();

            if( index < 0 )
            {
                WP_LOG_ERROR( "InputManagerExt::getChannelDataByIndex - Negative index: " +
                              StringUtil::toString( index ) );
                static SmartPtr<AxisData> nullChannelDataPtr;
                return nullChannelDataPtr;
            }

            if( index >= static_cast<s32>( channelDataMap.size() ) )
            {
                WP_LOG_WARNING( "InputManagerExt::getChannelDataByIndex - Index out of range: " +
                                StringUtil::toString( index ) + " (size: " +
                                StringUtil::toString( static_cast<s32>( channelDataMap.size() ) ) +
                                ")" );
                static SmartPtr<AxisData> nullChannelDataPtr;
                return nullChannelDataPtr;
            }

            return channelDataMap[index];
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "InputManagerExt::getChannelDataByIndex - Exception: " + String( e.what() ) );
            static SmartPtr<AxisData> nullChannelDataPtr;
            return nullChannelDataPtr;
        }
    }

    const SmartPtr<AxisData> &InputManagerExt::getChannelDataByIndex( s32 index ) const
    {
        try
        {
            auto channelDataMap = getChannelDataMap();

            if( index < 0 )
            {
                WP_LOG_ERROR( "InputManagerExt::getChannelDataByIndex (const) - Negative index: " +
                              StringUtil::toString( index ) );
                static SmartPtr<AxisData> nullChannelDataPtr;
                return nullChannelDataPtr;
            }

            if( index >= static_cast<s32>( channelDataMap.size() ) )
            {
                WP_LOG_WARNING( "InputManagerExt::getChannelDataByIndex (const) - Index out of range: " +
                                StringUtil::toString( index ) + " (size: " +
                                StringUtil::toString( static_cast<s32>( channelDataMap.size() ) ) +
                                ")" );
                static SmartPtr<AxisData> nullChannelDataPtr;
                return nullChannelDataPtr;
            }

            return channelDataMap[index];
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "InputManagerExt::getChannelDataByIndex (const) - Exception: " +
                          String( e.what() ) );
            static SmartPtr<AxisData> nullChannelDataPtr;
            return nullChannelDataPtr;
        }
    }

    InputManagerExt::CreateJobstickJob::CreateJobstickJob( RawPtr<InputManagerExt> inputManager ) :
        m_inputManager( inputManager ),
        m_retries( 0 )
    {
    }

    InputManagerExt::CreateJobstickJob::~CreateJobstickJob()
    {
    }

    void InputManagerExt::CreateJobstickJob::execute()
    {
    }

    void InputManagerExt::CreateJobstickJob::coroutine_execute_step( SmartPtr<ICoroutineData> &rYield )
    {
        m_inputManager->createJoystick();
    }

    void InputManagerExt::createRecorderListener()
    {
    }

    SmartPtr<Properties> InputManagerExt::getProperties() const
    {
        try
        {
            // Get parent properties first
            auto properties = IInputManager::getProperties();
            if( !properties )
            {
                WP_LOG_ERROR(
                    "InputManagerExt::getProperties - Failed to get parent properties, creating new" );
                properties = make_ptr<Properties>();
            }

            // Timing properties
            properties->setProperty( "doubleClickInterval", static_cast<f64>( m_doubleClickInterval ) );
            properties->setProperty( "inputTime", static_cast<f64>( m_inputTime ) );
            properties->setProperty( "lastClickTime", static_cast<f64>( m_lastClickTime ) );
            properties->setProperty( "lastInputTime", static_cast<f64>( m_lastInputTime ) );
            properties->setProperty( "nextJoystickCheck", m_nextJoystickCheck );

            // Input state flags
            properties->setProperty( "keyboardInputEnabled",
                                     static_cast<bool>( m_keyboardInputEnabled ) );
            properties->setProperty( "dongleOk", static_cast<bool>( m_dongleOk ) );
            properties->setProperty( "txOk", static_cast<bool>( m_txOk ) );
            properties->setProperty( "isAssigning", m_isAssigning );
            properties->setProperty( "isShiftPressed", m_isShiftPressed );
            properties->setProperty( "useOverride", m_bUseOverride );
            properties->setProperty( "enableInputLog", m_bEnableInputLog );
            properties->setProperty( "runInputLog", m_bRunInputLog );
            properties->setProperty( "bufferedKeys", m_bufferedKeys );
            properties->setProperty( "bufferedMouse", m_bufferedMouse );
            properties->setProperty( "enableInternalInputCapture", m_enableInternalInputCapture );

            // Mouse button state (read-only for diagnostic purposes)
            properties->setProperty( "isLeftPressed", m_isLeftPressed, true );
            properties->setProperty( "isRightPressed", m_isRightPressed, true );
            properties->setProperty( "isMiddlePressed", m_isMiddlePressed, true );

            // Button state
            properties->setProperty( "numButtons", m_numButtons );
            properties->setProperty( "currentButtons", static_cast<u32>( m_currentButtons ) );
            properties->setProperty( "previousButtons", static_cast<u32>( m_previousButtons ) );

            // Device configuration
            properties->setProperty( "adapterMode", m_adapterMode );
            properties->setProperty( "currentTxMode", m_currentTxMode );
            properties->setProperty( "status", static_cast<s32>( m_status ) );

            // Channel configuration
            properties->setProperty( "jsThrottleChannel", m_jsThrottleChannel );
            properties->setProperty( "gyroOverride", m_gyroOverride );

            // Diagnostic/read-only properties
            properties->setProperty( "updateState", m_updateState, true );
            properties->setProperty( "inputIndex", m_inputIndex, true );

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "InputManagerExt::getProperties - Exception: " + String( e.what() ) );
            // Return a valid empty Properties object rather than nullptr
            return make_ptr<Properties>();
        }
    }

    void InputManagerExt::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            WP_LOG_ERROR( "InputManagerExt::setProperties - Null properties pointer provided" );
            return;
        }

        try
        {
            // Call parent implementation first
            IInputManager::setProperties( properties );

            // Timing properties with validation
            f64 doubleClickInterval = static_cast<f64>( m_doubleClickInterval );
            if( properties->getPropertyValue( "doubleClickInterval", doubleClickInterval ) )
            {
                if( doubleClickInterval >= 0.0 && doubleClickInterval <= 2.0 )
                {
                    m_doubleClickInterval = doubleClickInterval;
                }
                else
                {
                    WP_LOG_WARNING(
                        "InputManagerExt::setProperties - doubleClickInterval out of range [0.0, "
                        "2.0]: " +
                        StringUtil::toString( doubleClickInterval ) );
                }
            }

            f64 inputTime = static_cast<f64>( m_inputTime );
            if( properties->getPropertyValue( "inputTime", inputTime ) )
            {
                if( inputTime >= 0.0 )
                {
                    m_inputTime = inputTime;
                }
                else
                {
                    WP_LOG_WARNING( "InputManagerExt::setProperties - inputTime cannot be negative: " +
                                    StringUtil::toString( inputTime ) );
                }
            }

            f64 lastClickTime = static_cast<f64>( m_lastClickTime );
            if( properties->getPropertyValue( "lastClickTime", lastClickTime ) )
            {
                m_lastClickTime = lastClickTime;
            }

            f64 lastInputTime = static_cast<f64>( m_lastInputTime );
            if( properties->getPropertyValue( "lastInputTime", lastInputTime ) )
            {
                m_lastInputTime = lastInputTime;
            }

            properties->getPropertyValue( "nextJoystickCheck", m_nextJoystickCheck );

            // Input state flags
            bool keyboardInputEnabled = m_keyboardInputEnabled;
            if( properties->getPropertyValue( "keyboardInputEnabled", keyboardInputEnabled ) )
            {
                m_keyboardInputEnabled = keyboardInputEnabled;
            }

            bool dongleOk = m_dongleOk;
            if( properties->getPropertyValue( "dongleOk", dongleOk ) )
            {
                m_dongleOk = dongleOk;
            }

            bool txOk = m_txOk;
            if( properties->getPropertyValue( "txOk", txOk ) )
            {
                m_txOk = txOk;
            }

            properties->getPropertyValue( "isAssigning", m_isAssigning );
            properties->getPropertyValue( "isShiftPressed", m_isShiftPressed );
            properties->getPropertyValue( "useOverride", m_bUseOverride );
            properties->getPropertyValue( "enableInputLog", m_bEnableInputLog );
            properties->getPropertyValue( "runInputLog", m_bRunInputLog );
            properties->getPropertyValue( "bufferedKeys", m_bufferedKeys );
            properties->getPropertyValue( "bufferedMouse", m_bufferedMouse );
            properties->getPropertyValue( "enableInternalInputCapture", m_enableInternalInputCapture );

            // Button state with validation
            s32 numButtons = m_numButtons;
            if( properties->getPropertyValue( "numButtons", numButtons ) )
            {
                if( numButtons >= 0 && numButtons <= 128 )
                {
                    m_numButtons = numButtons;
                }
                else
                {
                    WP_LOG_WARNING(
                        "InputManagerExt::setProperties - numButtons out of range [0, 128]: " +
                        StringUtil::toString( numButtons ) );
                }
            }

            u32 currentButtons = m_currentButtons;
            if( properties->getPropertyValue( "currentButtons", currentButtons ) )
            {
                m_currentButtons = currentButtons;
            }

            u32 previousButtons = m_previousButtons;
            if( properties->getPropertyValue( "previousButtons", previousButtons ) )
            {
                m_previousButtons = previousButtons;
            }

            // Device configuration
            String adapterMode = m_adapterMode;
            if( properties->getPropertyValue( "adapterMode", adapterMode ) )
            {
                if( !adapterMode.empty() )
                {
                    m_adapterMode = adapterMode;
                    m_adapterModeHash = StringUtil::getHash( adapterMode );
                }
                else
                {
                    WP_LOG_WARNING(
                        "InputManagerExt::setProperties - adapterMode cannot be empty string" );
                }
            }

            properties->getPropertyValue( "currentTxMode", m_currentTxMode );
            //properties->getPropertyValue( "currentTxModel", m_currentTxModel );

            s32 status = m_status;
            if( properties->getPropertyValue( "status", status ) )
            {
                m_status = status;
            }

            // Channel configuration with validation
            s32 jsThrottleChannel = m_jsThrottleChannel;
            if( properties->getPropertyValue( "jsThrottleChannel", jsThrottleChannel ) )
            {
                if( jsThrottleChannel >= 0 && jsThrottleChannel < NUM_CHANNELS )
                {
                    m_jsThrottleChannel = jsThrottleChannel;
                }
                else
                {
                    WP_LOG_WARNING(
                        "InputManagerExt::setProperties - jsThrottleChannel out of range [0, " +
                        StringUtil::toString( NUM_CHANNELS ) +
                        "): " + StringUtil::toString( jsThrottleChannel ) );
                }
            }

            f32 gyroOverride = m_gyroOverride;
            if( properties->getPropertyValue( "gyroOverride", gyroOverride ) )
            {
                if( gyroOverride >= -1.0f && gyroOverride <= 1.0f )
                {
                    m_gyroOverride = gyroOverride;
                }
                else
                {
                    WP_LOG_WARNING(
                        "InputManagerExt::setProperties - gyroOverride out of range [-1.0, 1.0]: " +
                        StringUtil::toString( gyroOverride ) );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "InputManagerExt::setProperties - Exception: " + String( e.what() ) );
            // Continue execution despite error - defensive approach
        }
    }

}  // namespace workphone
