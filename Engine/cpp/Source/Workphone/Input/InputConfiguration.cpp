#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Input/InputConfiguration.hpp"
#include "Workphone/Input/AxisConfigurationData.hpp"
#include "Workphone/Input/InputManagerExt.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, InputConfiguration, ISharedObject );

    InputConfiguration::InputConfiguration()
    {
        //m_fsm = new FiniteStateMachine();
        m_listener = new InputConfigurationFSMListener( this );
        m_fsm->addListener( m_listener );
    }

    InputConfiguration::~InputConfiguration()
    {
    }

    void InputConfiguration::centerAxes()
    {
        m_fsm->setNewState( Center, true );
    }

    void InputConfiguration::finishCenterAxes()
    {
    }

    void InputConfiguration::calculateStickExtents()
    {
        m_fsm->setNewState( FindExtents, true );
    }

    void InputConfiguration::finishCalculateStickExtents()
    {
        m_fsm->setNewState( Normal, true );
    }

    String InputConfiguration::getDirectionLabel( const String &input )
    {
        String stickDirectionLabel;

        if( input == "Throttle" )
            stickDirectionLabel = "vertical";
        else if( input == "Aileron" )
            stickDirectionLabel = "horizontal";
        else if( input == "Elevator" )
            stickDirectionLabel = "vertical";
        else if( input == "Rudder" )
            stickDirectionLabel = "horizontal";
        else if( input == "Collective Pitch" )
            stickDirectionLabel = "vertical";
        else
            stickDirectionLabel = "none";

        return stickDirectionLabel;
    }

    const SmartPtr<IFSM> &InputConfiguration::getFSM() const
    {
        return m_fsm;
    }

    SmartPtr<IFSM> &InputConfiguration::getFSM()
    {
        return m_fsm;
    }

    void InputConfiguration::setFSM( SmartPtr<IFSM> fsm )
    {
        m_fsm = fsm;
    }

    Array<SmartPtr<AxisConfigurationData>> InputConfiguration::getChannels() const
    {
        return Array<SmartPtr<AxisConfigurationData>>( m_channels.begin(), m_channels.end() );
    }

    void InputConfiguration::setChannels( Array<SmartPtr<AxisConfigurationData>> channels )
    {
        m_channels =
            ConcurrentArray<SmartPtr<AxisConfigurationData>>( channels.begin(), channels.end() );
    }

    void InputConfiguration::preUpdate( TaskId task, const double &t, const double &dt )
    {
        //m_fsm->preUpdate(task, t, dt);
    }

    void InputConfiguration::update( TaskId task, const double &t, const double &dt )
    {
        //m_fsm->update(task, t, dt);
    }

    void InputConfiguration::postUpdate( TaskId task, const double &t, const double &dt )
    {
        //m_fsm->postUpdate(task, t, dt);
    }

    void InputConfiguration::enterStateNormal()
    {
    }

    void InputConfiguration::preUpdateStateNormal( TaskId task, const double &t, const double &dt )
    {
    }

    void InputConfiguration::updateModelNormal( TaskId task, const double &t, const double &dt )
    {
    }

    void InputConfiguration::postUpdateNormal( TaskId task, const double &t, const double &dt )
    {
    }

    void InputConfiguration::leaveStateNormal()
    {
    }

    void InputConfiguration::enterStateCenterAxis()
    {
        // auto applicationManager = core::IApplicationManager::instance();
        // SmartPtr<Database> database = applicationManager->getDatabase();
        // auto inputManager = applicationManager->getInputManager();

        // Array<SmartPtr<ChannelObject>> channels;
        // for (size_t i = 0; i < 8; ++i)
        //{
        //	SmartPtr<ChannelObject> channel = new ChannelObject();
        //	channel->cmap = (int)i;
        //	channels.push_back(channel);
        // }

        // setChannels(channels);

        // String currentTxMode = inputManager->getCurrentTxMode();
        // String queryStr = "SELECT * FROM rx_map WHERE type = '" + currentTxMode + "' ORDER BY
        // cmap"; SmartPtr<Query> query = database->executeQuery(queryStr); if (query)
        //{
        //	while (!query->eof())
        //	{
        //		SmartPtr<ChannelObject> channel = new ChannelObject();

        //		channel->id = StringUtil::parseInt(query->getFieldValue("id"));
        //		channel->channel_number = StringUtil::parseInt(query->getFieldValue("channel"));
        //		channel->cmap = StringUtil::parseInt(query->getFieldValue("cmap"));
        //		channel->value = query->getFieldValue("value");
        //		channel->db_offset = StringUtil::parseFloat(query->getFieldValue("offset"));
        //		channel->db_multiplier = StringUtil::parseFloat(query->getFieldValue("multiplier"));
        //		channel->reverse = StringUtil::parseBool(query->getFieldValue("reverse"));
        //		channel->emulation = query->getFieldValue("emulation");

        //		//String sMin_throw_multiplier = query->getFieldValue("low_multiplier");
        //		//channel->min_throw_multiplier = StringUtil::parseFloat(sMin_throw_multiplier);

        //		channels.push_back(channel);

        //		query->nextRow();
        //	}
        //}

        // setChannels(channels);

        // resetDBOffset();

        // inputManager->initDongle("setTxData");
    }

    void InputConfiguration::preUpdateStateCenterAxis( TaskId task, const double &t, const double &dt )
    {
        try
        {
            switch( task )
            {
            case TaskId::Primary:
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto input = applicationManager->getInput();
                auto inputManager = applicationManager->getInputDeviceManager();

                Array<SmartPtr<AxisConfigurationData>> channels = getChannels();

                s32 channelsLen = static_cast<int>( channels.size() );
                f32 newOffset = 0;

                s32 adapterChannelsLength = static_cast<int>( channels.size() );
                for( s32 i = 0; i < adapterChannelsLength; i++ )
                {
                    SmartPtr<AxisConfigurationData> pSimChannelValue = channels[i];
                    AxisConfigurationData &simChannelValue = *pSimChannelValue;

                    //f32 adapterChannelValue = input->getAxisValueSaturated( simChannelValue.cmap );
                    //simChannelValue.newOffset = -adapterChannelValue;
                }
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

    void InputConfiguration::updateModelCenterAxis( TaskId task, const double &t, const double &dt )
    {
    }

    void InputConfiguration::postUpdateCenterAxis( TaskId task, const double &t, const double &dt )
    {
    }

    void InputConfiguration::leaveStateCenterAxis()
    {
        // auto applicationManager = core::IApplicationManager::instance();
        // auto inputManager = applicationManager->getInputManager();
        // SmartPtr<Database> database = applicationManager->getDatabase();
        // String currentTxMode = inputManager->getCurrentTxMode();

        // Array<SmartPtr<ChannelObject>> channels = getChannels();
        // s32 adapterChannelsLength = channels.size();
        // for (s32 i = 0; i < adapterChannelsLength; i++)
        //{
        //	SmartPtr<ChannelObject> pSimChannelValue = channels[i];
        //	ChannelObject& simChannelValue = *pSimChannelValue;

        //	String offset = StringUtil::toString(simChannelValue.newOffset);
        //	String query = "UPDATE rx_map SET offset = '" + offset +
        //		"' WHERE value = '" +
        //		simChannelValue.value +
        //		"' AND type = '" + currentTxMode + "'";
        //	database->executeQuery(query);
        //}
    }

    void InputConfiguration::enterStateFindExtents()
    {
    }

    void InputConfiguration::preUpdateStateFindExtents( TaskId task, const double &t, const double &dt )
    {
        try
        {
            switch( task )
            {
            case TaskId::Primary:
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto input = applicationManager->getInput();
                auto inputManager = applicationManager->getInputDeviceManager();
                SmartPtr<DatabaseManager> database = applicationManager->getDatabase();
                // safeApply();

                Array<SmartPtr<AxisConfigurationData>> channels = getChannels();
                s32 adapterChannelsLength = static_cast<int>( channels.size() );
                for( s32 i = 0; i < adapterChannelsLength; i++ )
                {
                    SmartPtr<AxisConfigurationData> pSimChannelValue = channels[i];
                    AxisConfigurationData &simChannelValue = *pSimChannelValue;

                    f32 adapterChannelValue = input->getAxisValueRaw( simChannelValue.cmap );

                    if( adapterChannelValue < -0.01f || adapterChannelValue > 0.01f )
                    {
                        if( adapterChannelValue > -10.0f && adapterChannelValue < 10.0f )
                        {
                            if( adapterChannelValue > simChannelValue.max_throw )
                            {
                                simChannelValue.max_throw = adapterChannelValue;
                                simChannelValue.max_throw_multiplier = adapterChannelValue;
                            }

                            if( adapterChannelValue < simChannelValue.min_throw )
                            {
                                simChannelValue.min_throw = adapterChannelValue;
                                simChannelValue.min_throw_multiplier = adapterChannelValue;
                            }
                        }
                    }

                    if( !MathF::isFinite( simChannelValue.min_throw_multiplier ) )
                    {
                        simChannelValue.min_throw_multiplier = 0.0f;
                    }

                    if( !MathF::isFinite( simChannelValue.max_throw_multiplier ) )
                    {
                        simChannelValue.max_throw_multiplier = 0.0f;
                    }
                }
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

    void InputConfiguration::updateModelFindExtents( TaskId task, const double &t, const double &dt )
    {
    }

    void InputConfiguration::postUpdateFindExtents( TaskId task, const double &t, const double &dt )
    {
    }

    void InputConfiguration::leaveStateFindExtents()
    {
        auto applicationManager = core::IApplicationManager::instance();
        SmartPtr<DatabaseManager> database = applicationManager->getDatabase();
        auto inputManager = applicationManager->getInputDeviceManager();

        f32 low_m = 0.0f;
        f32 high_m = 0.0f;

        Array<SmartPtr<AxisConfigurationData>> channels = getChannels();
        for( size_t i = 0; i < channels.size(); i++ )
        {
            SmartPtr<AxisConfigurationData> pSimChannelValue = channels[i];
            AxisConfigurationData &simChannelValue = *pSimChannelValue;

            // if (simChannelValue.reverse == true)
            //{
            //	low_m = simChannelValue.max_throw_multiplier;
            //	high_m = simChannelValue.min_throw_multiplier;
            // }
            // else
            //{
            //	low_m = simChannelValue.min_throw_multiplier;
            //	high_m = simChannelValue.max_throw_multiplier;
            // }

            if( !MathF::isFinite( high_m ) )
            {
                high_m = 1.0f;
            }

            if( !MathF::isFinite( low_m ) )
            {
                low_m = 1.0f;
            }

            Array<int> functionIds;
            auto rxmapQuerySql = String( "select * from rx_map" );
            auto rxmapQuery = database->executeQuery( rxmapQuerySql );
            if( rxmapQuery )
            {
                while( !rxmapQuery->eof() )
                {
                    auto id = rxmapQuery->getFieldValue( "id" );
                    auto cmap = rxmapQuery->getFieldValue( "cmap" );

                    s32 iId = StringUtil::parseInt( id );

                    s32 iChMap = -1;
                    if( StringUtil::isNumber( cmap ) )
                    {
                        iChMap = StringUtil::parseInt( cmap );
                    }
                    else
                    {
                        //data::CMapData data;
                        //DataUtil::parse( cmap, &data );

                        //if(data.axis != -1)
                        //{
                        //    iChMap = data.axis;
                        //}
                    }

                    if( iChMap == simChannelValue.cmap )
                    {
                        functionIds.push_back( iId );
                    }

                    rxmapQuery->nextRow();
                }
            }

            for( size_t funcIdx = 0; funcIdx < functionIds.size(); ++funcIdx )
            {
                s32 iId = functionIds[funcIdx];
                auto cmapQuerySql =
                    "select * from rx_map where id = '" + StringUtil::toString( iId ) + "';";
                auto cmapQuery = database->executeQuery( cmapQuerySql );
                if( cmapQuery )
                {
                    auto id = cmapQuery->getFieldValue( "id" );
                    bool isReversed = StringUtil::parseBool( cmapQuery->getFieldValue( "reverse" ) );

                    f32 range =
                        simChannelValue.max_throw_multiplier - simChannelValue.min_throw_multiplier;
                    f32 offset = simChannelValue.newOffset;

                    if( !isReversed )
                    {
                        low_m = simChannelValue.min_throw_multiplier;
                        high_m = simChannelValue.max_throw_multiplier;
                    }
                    else
                    {
                        low_m = simChannelValue.min_throw_multiplier;
                        high_m = simChannelValue.max_throw_multiplier;
                    }

                    low_m = -0.8f / ( low_m + offset );
                    high_m = 0.8f / ( high_m + offset );

                    auto querySql = "UPDATE rx_map SET multiplier = '1', low_multiplier = '" +
                                    StringUtil::toString( low_m ) + "', high_multiplier = '" +
                                    StringUtil::toString( high_m ) + "' WHERE id = '" + id + "';";
                    database->executeQuery( querySql.c_str() );

                    auto sOffset = StringUtil::toString( offset );
                    auto offsetQuerySql = String( "UPDATE rx_map SET offset = '" ) + sOffset.c_str() +
                                          "' WHERE id = '" + id + "';";
                    database->executeQuery( offsetQuerySql.c_str() );

                    cmapQuery->nextRow();
                }
            }
        }

        //String currentTxMode = inputManager->getCurrentTxModel();
        //inputManager->initDongle( "setTxData" + currentTxMode );
    }

    void InputConfiguration::resetDBOffset()
    {
        auto applicationManager = core::IApplicationManager::instance();

        SmartPtr<DatabaseManager> database = applicationManager->getDatabase();
        if( database )
        {
            auto inputManager = applicationManager->getInputDeviceManager();
            String query =
                "UPDATE rx_map SET offset = '0', multiplier = '1', low_multiplier = '1', "
                "high_multiplier = '1'";
            database->executeQuery( query.c_str() );
        }
    }

    void InputConfiguration::safeApply()
    {
        s32 adapterChannelsLength = 8;
        for( s32 i = 0; i < adapterChannelsLength; i++ )
        {
            // auto adapterChannelValue = adapterChannels[i];

            //	/*var adapterChannelValue = adapterChannels[simChannels[i].cmap];
            //	if (adapterChannelValue === undefined) { adapterChannelValue = 0; }*/
            //	var simChannelValue = simChannels[i];

            //	if (initChannelValues == = true) {
            //		$scope.initialChannelValues.push(adapterChannelValue);
            //	}

            //	simChannelValue.realValue = adapterChannelValue;

            //	if (adapterChannelValue < -0.4 || adapterChannelValue > 0.4) {
            //		if (adapterChannelValue > -1.2 && adapterChannelValue < 1.2) {
            //			if (adapterChannelValue >(simChannelValue.max_throw * 1)) {
            //				simChannelValue.max_throw = adapterChannelValue;
            //				simChannelValue.max_throw_multiplier = (0.8 / adapterChannelValue).toFixed(4) *
            //1;
            //			}
            //			if (adapterChannelValue < (simChannelValue.min_throw * 1)) {
            //				simChannelValue.min_throw = adapterChannelValue;
            //				simChannelValue.min_throw_multiplier = (-0.8 / adapterChannelValue).toFixed(4) *
            //1;
            //			}
            //		}
            //	}

            //	simChannelValue.displayValue = Math.round(adapterChannelValue * 125);

            //	if (adapterChannelValue > -1.2 && adapterChannelValue < 1.2) {
            //		simChannelValue.progressValue = Math.round((((adapterChannelValue * 1) + 1) / 2) *
            //100);
            //	}
            //	else {
            //		if (adapterChannelValue > 0) {
            //			simChannelValue.progressValue = 100;
            //			simChannelValue.displayValue = "Out of Range";
            //		}
            //		else {
            //			simChannelValue.progressValue = -100;
            //			simChannelValue.displayValue = "Out of Range";
            //		}
            //	}

            //	if (adapterChannelValue < -0.81 || adapterChannelValue > 0.81) {
            //		simChannelValue.color = '#bd0000';
            //	}
            //	else {
            //		simChannelValue.color = '#4D8325';
            //	}

            //	if (initChannelValues == = false) {
            //		if (adapterChannelValue < -0.65 || adapterChannelValue > 0.65) {
            //			var diff = adapterChannelValue - $scope.initialChannelValues[i];
            //			if (diff < 0) {
            //				diff = diff * -1;
            //			}

            //			if (diff > maxChannelValue) {
            //				maxChannelValue = diff;
            //				maxChannel = i;
            //			}

            //			if ($scope.currentMaxedChannel == (i + 1)) {
            //				if (adapterChannelValue < 0) {
            //					maxChannelReversed = true;
            //					$scope.currentMaxedChannelReversed = maxChannelReversed;
            //				}
            //				else {
            //					maxChannelReversed = false;
            //					$scope.currentMaxedChannelReversed = maxChannelReversed;
            //				}
            //			}
            //		}
            //	}
            //}

            // if (maxChannelValue > $scope.currentMaxedChannelValue) {
            //	$scope.currentMaxedChannel = maxChannel + 1;
            //	$scope.currentMaxedChannelValue = maxChannelValue;
            // }

            // if ($scope.currentMaxedChannel > 0) {
            //	$scope.currentDetectedChannel = $scope.currentMaxedChannel;
        }
    }

    InputConfiguration::InputConfigurationFSMListener::InputConfigurationFSMListener(
        RawPtr<InputConfiguration> inputConfiguration ) :
        m_inputConfiguration( inputConfiguration )
    {
    }

    InputConfiguration::InputConfigurationFSMListener::InputConfigurationFSMListener()
    {
    }

    InputConfiguration::InputConfigurationFSMListener::~InputConfigurationFSMListener()
    {
    }

    u32 InputConfiguration::InputConfigurationFSMListener::getTypeInfo() const
    {
        return static_cast<u32>( 0 );
    }

    void InputConfiguration::InputConfigurationFSMListener::setTypeInfo( u32 type )
    {
        (void)type;
    }

    void InputConfiguration::InputConfigurationFSMListener::preUpdateFSM( SmartPtr<IFSM> fsm,
                                                                          TaskId task, const double &t,
                                                                          const double &dt )
    {
        auto state = static_cast<InputConfiguration::State>( fsm->getCurrentState() );
        switch( state )
        {
        case Normal:
        {
            m_inputConfiguration->preUpdateStateNormal( task, t, dt );
        }
        break;
        case Center:
        {
            m_inputConfiguration->preUpdateStateCenterAxis( task, t, dt );
        }
        break;
        case FindExtents:
        {
            m_inputConfiguration->preUpdateStateFindExtents( task, t, dt );
        }
        break;
        }
    }

    void InputConfiguration::InputConfigurationFSMListener::postUpdateFSM( SmartPtr<IFSM> fsm,
                                                                           TaskId task, const double &t,
                                                                           const double &dt )
    {
        auto state = static_cast<InputConfiguration::State>( fsm->getCurrentState() );
        switch( state )
        {
        case Normal:
        {
            m_inputConfiguration->postUpdateNormal( task, t, dt );
        }
        break;
        case Center:
        {
            m_inputConfiguration->postUpdateCenterAxis( task, t, dt );
        }
        break;
        case FindExtents:
        {
            m_inputConfiguration->postUpdateFindExtents( task, t, dt );
        }
        break;
        }
    }

    void InputConfiguration::InputConfigurationFSMListener::updateFSM( SmartPtr<IFSM> fsm, TaskId task,
                                                                       const double &t,
                                                                       const double &dt )
    {
        auto state = static_cast<InputConfiguration::State>( fsm->getCurrentState() );
        switch( state )
        {
        case Normal:
        {
            m_inputConfiguration->updateModelNormal( task, t, dt );
        }
        break;
        case Center:
        {
            m_inputConfiguration->updateModelCenterAxis( task, t, dt );
        }
        break;
        case FindExtents:
        {
            m_inputConfiguration->updateModelFindExtents( task, t, dt );
        }
        break;
        }
    }

    s32 InputConfiguration::InputConfigurationFSMListener::handleFSMEvent( SmartPtr<IFSM> fsm,
                                                                           s32 eventType )
    {
        switch( static_cast<FSMEvent>( eventType ) )
        {
        case FSMEvent::Enter:
        {
            auto state = static_cast<InputConfiguration::State>( fsm->getCurrentState() );
            switch( state )
            {
            case Normal:
            {
                m_inputConfiguration->enterStateNormal();
            }
            break;
            case Center:
            {
                m_inputConfiguration->enterStateCenterAxis();
            }
            break;
            case FindExtents:
            {
                m_inputConfiguration->enterStateFindExtents();
            }
            break;
            }
        }
        break;
        case FSMEvent::Leave:
        {
            auto state = static_cast<InputConfiguration::State>( fsm->getCurrentState() );
            switch( state )
            {
            case Normal:
            {
                m_inputConfiguration->leaveStateNormal();
            }
            break;
            case Center:
            {
                m_inputConfiguration->leaveStateCenterAxis();
            }
            break;
            case FindExtents:
            {
                m_inputConfiguration->leaveStateFindExtents();
            }
            break;
            }
        }
        break;
        }

        return 0;
    }

    void InputConfiguration::InputConfigurationFSMListener::newState( s32 state )
    {
    }
}  // namespace workphone
