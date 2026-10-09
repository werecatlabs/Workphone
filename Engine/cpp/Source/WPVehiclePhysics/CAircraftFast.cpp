#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CAircraftFast.hpp>
#include <WPVehiclePhysics/CAircraftWing.hpp>
#include <WPVehiclePhysics/CAircraftWingFast.hpp>
#include <WPVehiclePhysics/CAircraftBody.hpp>
#include <WPVehiclePhysics/EngineSimple.hpp>
#include <WPVehiclePhysics/CAircraftControlSurface.hpp>
#include <WPVehiclePhysics/CAircraftEngine.hpp>
#include <WPVehiclePhysics/CAircraftMotor.hpp>
#include <WPVehiclePhysics/CBatteryPackStandard.hpp>
#include <WPVehiclePhysics/CESController.hpp>
#include <WPVehiclePhysics/CAircraftPropeller.hpp>
#include <WPVehiclePhysics/CAircraftPropellerUnit.hpp>
#include <WPVehiclePhysics/InputController.hpp>
#include <WPVehiclePhysics/CAerodymanicsWind.hpp>
#include <WPVehiclePhysics/CAircraftPropWash.hpp>
#include <WPVehiclePhysics/WheelControllerArcade.hpp>
#include <WPVehiclePhysics/WheelControllerPacejka.hpp>
#include <Workphone/Workphone.hpp>
#include <fstream>

#ifdef WP_PLATFORM_WIN32
#    include <execution>
#else
// #include <tbb/parallel_for_each.hpp>
#endif

namespace workphone::vehicle
{
    constexpr double FourPi = 12.5664;
    const double OneOver4Pi = 1.0f / FourPi;
    constexpr double AirViscosity = 18.325E-6;
    constexpr char WingFileName[] = "WingData.csv";
    // const int MaxCPs = 200;
    // const int GET_TX_CHANNEL = 1;
    // const int GET_ANGULAR_VELOCITY = 2;
    // const int GET_LINEAR_VELOCITY = 3;
    // const int ADD_LOCAL_FORCE = 4;
    // const int ADD_LOCAL_TORQUE = 5;
    // const int DISPLAY_LOCAL_VECTOR = 6;
    // const int SET_MASS_PROPS = 7;
    // const int GET_GLOBAL_POSITION = 8;
    // const int GET_GLOBAL_ORIENTATION = 9;
    // const int CAST_LOCAL_RAY = 10;
    // const int GET_CONTROL_ANGLES = 11;
    // const int GET_GYRO_OUTPUT = 12;
    // const int GET_GOVERNOR_OUTPUT = 13;
    // const int GET_ENGINE_OUTPUT = 14;

    // const int CB_MODEL = 0;
    // const int CB_MODEL_ORIENTATION = 0;

    using CWheelControllerDefault = WheelControllerPacejka;

    real_Num CAircraftFast::getEngineRPM(int idx) const
    {
        if(idx < static_cast<s32>(m_engines.size()))
        {
            const SmartPtr<IAircraftPowerUnit> engine = m_engines[idx];
            return engine->getRPM();
        }

        return 0.0;
    }

    void CAircraftFast::update(const double &t, const double &fDT)
    {
        // WP_LOG(String("Delta: ") + StringUtil::toString(1.0 / fDT));

        if(!m_rigidbody)
        {
            return;
        }

        clearForces();

        auto dt = fDT;

        WP_ASSERT(dt > 0.0);

        if(dt > 1.0 / 50.0)
        {
            dt = 1.0 / 50.0;
        }

        WP_ASSERT(m_rigidbody);

        auto controlAngles = m_callback->getControlAngles();
        for(auto &c : m_controlSurfaces)
        {
#if 1
            auto id = c->getSurfaceId();
            if(id >= 0 && id < controlAngles.size())
            {
                auto angle = controlAngles[id];
                auto angleRads = Math<real_Num>::DegToRad(angle);
                c->setCurrentDeflection(angleRads / Math<real_Num>::half_pi());
            }
#else
            auto id = c->getSurfaceId();
            if(id >= 0 && id < controlAngles.size())
            {
                auto angle = (real_Num)controlAngles[id];
                c->setCurrentDeflection(angle);
            }
#endif
        }

#if 0
        Vector3<real_Num> p[5];
        p[0] = Vector3<real_Num>(1.0f, 0.0f, 1.0f);
        p[1] = Vector3<real_Num>(-1.0f, 0.0f, 1.0f);
        p[2] = Vector3<real_Num>(-1.0f, 0.0f, -1.0f);
        p[3] = Vector3<real_Num>(1.0f, 0.0f, -1.0f);
        p[4] = Vector3<real_Num>(1.0f, 0.0f, 1.0f);

        for(size_t i = 0; i < 5; ++i)
        {
            m_worldTransform->transformPoint(p[i]);
        }

        for(size_t i = 0; i < 4; ++i)
        {
            displayVector(0, i, p[i], p[i + 1]);
        }
#endif

        if(getDisplayDebugData())
        {
            drawPoint(0, static_cast<s32>(1665165416554), m_cg, 0x00AFAF);
        }

        // if (m_rigidbody)
        //{
        //	m_rigidbody->update(t, dt);
        // }

        getAllTxData();
        updateTransform();

        // for (auto& p : m_propellers)
        //{
        //	WP_ASSERT(p);
        //	p->update(t, dt);
        // }

        // for (auto& e : m_escs)
        //{
        //	WP_ASSERT(e);
        //	e->update(t, dt);
        // }

        // for (auto& p : m_propellerUnits)
        //{
        //	WP_ASSERT(p);
        //	p->update(t, dt);
        // }

        // for (auto& e : m_engines)
        //{
        //	WP_ASSERT(e);
        //	e->update(t, dt);
        // }

#ifdef WP_PLATFORM_WIN32
        for(size_t i = 0; i < m_numWings; ++i)
        {
            auto &w = m_wings[i];
            w.update(t, dt);
        }

        // std::for_each(
        //	std::execution::par,
        //	m_wings.begin(),
        //	m_wings.end(),
        //	[t, dt](auto&& w)
        //{
        //	WP_ASSERT(w);
        //	w->update(t, dt);
        // });

        // tbb::parallel_for_each(
        //	m_wings.begin(),
        //	m_wings.end(),
        //	[t, dt](auto&& w)
        //{
        //	try
        //	{
        //		WP_ASSERT(w);
        //		w->update(t, dt);
        //	}
        //	catch (std::exception& e)
        //	{
        //		WP_LOG(e);
        //	}
        //	catch (...)
        //	{
        //		WP_LOG_ERROR("Unhandled exception");
        //	}
        //	});
#else
        for(auto &w : m_wings)
        {
            w.update(t, dt);
        }
#endif

        auto steeringAngle = controlAngles[5];

        for(auto &w : m_wheels)
        {
            if(w->isSteeringWheel())
            {
                w->setSteeringAngle(steeringAngle);
            }
        }

#ifdef WP_PLATFORM_WIN32
        // for (auto& w : m_wheels)
        //{
        //	WP_ASSERT(w);
        //	w->update(t, dt);
        // }

        // std::for_each(
        //	std::execution::par,
        //	m_wheels.begin(),
        //	m_wheels.end(),
        //	[t, dt](auto&& w)
        //{
        //	WP_ASSERT(w);
        //	w->update(t, dt);
        // });
#else
        for(auto &w : m_wheels)
        {
            WP_ASSERT(w);
            // w->update(t, dt);
        }
#endif

        updateDrag(t, dt);

        if(m_callback)
        {
            m_callback->addForce(2, m_force);

            static bool bEnableTorque = true;
            if(bEnableTorque)
            {
                m_callback->addTorque(2, m_torque);
            }

            if(getDisplayDebugData())
            {
                real_Num debugLineScale = 300.0;
                auto p = m_worldTransform.transformPoint(m_cg);
                // displayVector(0, 214343220, p, p + (m_force * debugLineScale * dt), 0xFF0000);

                displayVector(
                    0, 214343241, p,
                    p + ((m_worldTransform.getOrientation() * Vector3<real_Num>::UNIT_X) *
                         m_torque.X() * debugLineScale * static_cast<real_Num>(dt)),
                    0xFFFF00);
                displayVector(
                    0, 214343242, p,
                    p + ((m_worldTransform.getOrientation() * Vector3<real_Num>::UNIT_Y) *
                         m_torque.Y() * debugLineScale * static_cast<real_Num>(dt)),
                    0xFFFF00);
                displayVector(
                    0, 214343243, p,
                    p + ((m_worldTransform.getOrientation() * Vector3<real_Num>::UNIT_Z) *
                         m_torque.Z() * debugLineScale * static_cast<real_Num>(dt)),
                    0xFFFF00);
            }
        }
    }

    //-----------------------------------------------
    Vector3<real_Num> CAircraftFast::getDrag() const
    {
        return m_drag;
    }

    //-----------------------------------------------
    void CAircraftFast::updateDrag(const double &t, const double &dt)
    {
        WP_ASSERT(MathD::isFinite( t ));
        WP_ASSERT(MathD::isFinite( dt ));

        Vector3<real_Num> drag = getDrag();
        WP_ASSERT(drag.length() < 1e8);

        real_Num Xcoef = drag.X();
        real_Num Ycoef = drag.Y();
        real_Num Zcoef = drag.Z();

        if(!getEnablePowerUnit())
        {
            Xcoef = static_cast<real_Num>(100.0);
            Ycoef = static_cast<real_Num>(100.0);
            Zcoef = static_cast<real_Num>(100.0);
        }

        Vector3<real_Num> modelLocalVelocity = getLocalLinearVelocity();
        WP_ASSERT(modelLocalVelocity.length() < 1e8);

        Vector3<real_Num> localVelocity = Vector3<real_Num>::ZERO;
        localVelocity.X() =
            Math<real_Num>::Abs(modelLocalVelocity.X()) * modelLocalVelocity.X() * -Xcoef;
        localVelocity.Y() =
            Math<real_Num>::Abs(modelLocalVelocity.Y()) * modelLocalVelocity.Y() * -Ycoef;
        localVelocity.Z() =
            Math<real_Num>::Abs(modelLocalVelocity.Z()) * modelLocalVelocity.Z() * -Zcoef;

        WP_ASSERT(localVelocity.length() < 1e5);

        if(localVelocity.length() > modelLocalVelocity.length())
        {
            localVelocity = localVelocity.normaliseCopy() * modelLocalVelocity.length();
        }

        if(localVelocity.length() < 1e5)
        {
            addLocalForce(0, localVelocity, Vector3<real_Num>::zero());
        }
    }

    SmartPtr<IVehicleBody> CAircraftFast::getBody() const
    {
        return m_rigidbody;
    }

    CAircraftFast::CAircraftFast()
    {
    }

    CAircraftFast::~CAircraftFast() = default;

    bool CAircraftFast::isValid() const
    {
        for(auto &p : m_propellers)
        {
            if(!p->isValid())
            {
                return false;
            }
        }

        for(auto &p : m_propellerUnits)
        {
            if(!p->isValid())
            {
                return false;
            }
        }

        return m_worldTransform.isValid() && m_localTransform.isValid();
    }

    void CAircraftFast::load(const String &data)
    {
        try
        {
            // auto worldTransform = fb::make_ptr<Transform3<real_Num>>();
            // setWorldTransform(worldTransform);

            // auto localTransform = fb::make_ptr<Transform3<real_Num>>();
            // m_localTransform = localTransform;

            // auto bodyTransform = fb::make_ptr<Transform3<real_Num>>();
            // m_bodyTransform = bodyTransform;

            m_properties = workphone::make_ptr<Properties>();

            SmartPtr<IAerodymanicsWind> wind = workphone::make_ptr<CAerodymanicsWind>();
            setWind(wind);

            SmartPtr<CAircraftFast> pThis = getSharedFromThis<CAircraftFast>();

            SmartPtr<CAircraftBody> body = workphone::make_ptr<CAircraftBody>();
            body->setParentAircraft(pThis);
            setBody(body);

            loadProperties();

            bool isElectric = true;
            if(m_properties->getPropertyValue("FlEqElectricPower", isElectric))
            {
                setElectric(isElectric);
            }

            float mass = 0.0f;
            if(m_properties->getPropertyValue("FlEqModelMass", mass))
            {
                body->setMass(mass);
            }

            // loadDefaults();

            if(!StringUtil::isNullOrEmpty(data))
            {
                loadFromString(data);
            }

            // for (auto& e : m_engines)
            //{
            //	e->load(m_properties);
            // }

            for(auto &w : m_wings)
            {
                w.load(m_properties);
            }

            for(auto &w : m_wings)
            {
                w.updateBodyTransform();
            }
        }
        catch(std::exception &e)
        {
            WP_LOG_EXCEPTION(e);
        }
    }

    void CAircraftFast::loadFromString(const String &data)
    {
        // m_batteryPack = nullptr;
        // m_escs.clear();
        // m_engines.clear();
        // m_controlSurfaces.clear();
        // m_propellers.clear();
        // m_propellerUnits.clear();
        // m_wheels.clear();
        // m_propwashes.clear();

        // data::aircraft_data aircraftData;
        // DataUtil::parse( data, &aircraftData );

        // m_cg = Vector3<real_Num>( aircraftData.cgPosition.x, aircraftData.cgPosition.y,
        //                           aircraftData.cgPosition.z );
        // m_drag = Vector3<real_Num>( aircraftData.drag.x, aircraftData.drag.y, aircraftData.drag.z
        // ); m_rollwiseDamping = aircraftData.rollwiseDamping; m_sectionMultiplier =
        // aircraftData.sectionMultiplier;

        // data::vec4 p = aircraftData.localTransform.position;
        // data::vec4 q = aircraftData.localTransform.orientation;
        // data::vec4 s = aircraftData.localTransform.scale;

        // auto vPos = Vector3<real_Num>( p.x, p.y, -p.z );
        // auto vScale = Vector3<real_Num>( s.x, s.y, s.z );
        // auto qRot = Quaternion<real_Num>( q.w, -q.x, -q.y, q.z );

        // auto localTransform = getLocalTransform();
        // localTransform.setPosition( vPos );
        // localTransform.setScale( vScale );
        // localTransform.setOrientation( qRot );

        // updateTransform();

        // SmartPtr<CAircraftFast> pThis = getSharedFromThis<CAircraftFast>();

        // SmartPtr<IVehicleBody> body = getBody();
        ////body->setParentAircraft(pThis);
            ////setBody(body);

        // if(isElectric())
        //{
        //     SmartPtr<CBatteryPack> b = fb::make_ptr<CBatteryPack>();
        //     b->load( m_properties );
        //     b->setParentAircraft( pThis );
        //     b->setParent( body );
        //     setBatteryPack( b );

        //    SmartPtr<CESController> esc = fb::make_ptr<CESController>();
        //    esc->load( m_properties );
        //    esc->setParentAircraft( pThis );
        //    esc->setParent( body );
        //    m_escs.push_back( esc );

        //    esc->setBatteryPack( b );

        //    s32 count = 0;
        //    for(auto &data : aircraftData.engineData)
        //    {
        //        if(count < static_cast<s32>(m_powerChannels.size()))
        //        {
        //            SmartPtr<Properties> properties = m_powerChannels[count];

        //            SmartPtr<CAircraftMotor> e = fb::make_ptr<CAircraftMotor>();
        //            SmartPtr<CAircraftPropeller> p = fb::make_ptr<CAircraftPropeller>();
        //            SmartPtr<CAircraftPropellerUnit> propellerUnit = fb::make_ptr<
        //                CAircraftPropellerUnit>();

        //            p->setParentAircraft( pThis );
        //            p->setParent( body );

        //            e->setName( data.name );
        //            e->setParentAircraft( pThis );
        //            e->setParent( body );
        //            e->setESC( esc );
        //            e->setBatteryPack( b );
        //            e->load( &data );
        //            e->setPropeller( p );

        //            p->load( properties );
        //            e->load( properties );

        //            propellerUnit->setPowerUnit( e );
        //            propellerUnit->setPropeller( p );
        //            propellerUnit->setBatteryPack( b );
        //            propellerUnit->setESC( esc );
        //            propellerUnit->setParentAircraft( pThis );
        //            propellerUnit->setParent( body );

        //            esc->setMotor( e );

        //            m_engines.push_back( e );
        //            m_propellers.push_back( p );
        //            m_propellerUnits.push_back( propellerUnit );
        //        }

        //        count++;
        //    }
        //}
        // else
        //{
        //    for(auto &data : aircraftData.engineData)
        //    {
        //        SmartPtr<CAircraftEngine> e = fb::make_ptr<CAircraftEngine>();

        //        SmartPtr<CAircraftPropeller> p = fb::make_ptr<CAircraftPropeller>();
        //        p->setParentAircraft( pThis );
        //        p->setParent( body );

        //        e->setName( data.name );
        //        e->setParentAircraft( pThis );
        //        e->setParent( body );
        //        e->load( &data );
        //        e->setPropeller( p );

        //        m_engines.push_back( e );
        //    }
        //}

        // for(auto &data : aircraftData.wheelData)
        //{
        //     auto wheel = fb::make_ptr<CWheelControllerDefault>();

        //    wheel->setOwner( pThis );
        //    wheel->setName( data.name );
        //    wheel->setMass( data.mass );
        //    wheel->setRadius( data.radius );
        //    wheel->setDamping( data.suspensionDamper );
        //    wheel->setSuspensionTravel( data.suspensionDistance * real_Num(0.5) );
        //    wheel->setSuspensionDistance( data.suspensionDistance );
        //    wheel->setSpringRate( data.springRate );
        //    wheel->setSteeringWheel( data.isSteeringWheel );

        //    data::vec4 p = data.localTransform.position;
        //    data::vec4 q = data.localTransform.orientation;
        //    data::vec4 s = data.localTransform.scale;

        //    auto vPos = Vector3<real_Num>( p.x, p.y, -p.z );
        //    auto vScale = Vector3<real_Num>( s.x, s.y, s.z );
        //    auto qRot = Quaternion<real_Num>( q.w, -q.x, -q.y, q.z );

        //    auto localTransform = wheel->getLocalTransform();
        //    localTransform.setPosition( vPos );
        //    localTransform.setScale( vScale );
        //    localTransform.setOrientation( qRot );

        //    m_wheels.push_back( wheel );
        //}

        // for(auto &data : aircraftData.controlSurfaceData)
        //{
        //     SmartPtr<CAircraftControlSurface> c = fb::make_ptr<CAircraftControlSurface>();
        //     c->setParentAircraft( pThis );
        //     c->setSurfaceId( data.surfaceId );
        //     c->setName( data.name );
        //     c->setReversed( data.reverse );
        //     c->setTipHingeDistanceFromTrailingEdge( data.tipHingeDistanceFromTrailingEdge );
        //     c->setRootHingeDistanceFromTrailingEdge( data.rootHingeDistanceFromTrailingEdge );
        //     c->setRotationAxis( Vector3<real_Num>( data.modelRotationAxis.x,
        //     data.modelRotationAxis.y,
        //                                            -data.modelRotationAxis.z ) );

        //    c->load( &data );

        //    Array<bool> affectedSections;
        //    affectedSections.resize( data.affectedSections.size() );

        //    for(size_t i = 0; i < affectedSections.size(); ++i)
        //    {
        //        affectedSections[i] = data.affectedSections[i] == 1;
        //    }

        //    c->setAffectedSections( affectedSections );

        //    SmartPtr<InputController> inputController = c->getInputController();
        //    if(inputController)
        //    {
        //        inputController->setParentAircraft( pThis );
        //        inputController->setAxisName( data.inputData.axisName );
        //    }

        //    m_controlSurfaces.push_back( c );
        //}

        // for(auto &propwashData : aircraftData.propWashData)
        //{
        //     SmartPtr<CAircraftPropWash> propwash = fb::make_ptr<CAircraftPropWash>();

        //    propwash->setName( propwashData.name );
        //    propwash->setParentAircraft( pThis );

        //    for(auto pu : m_propellerUnits)
        //    {
        //        if(pu)
        //        {
        //            auto e = pu->getPowerUnit();
        //            if(e->getName() == propwashData.propwashSource)
        //            {
        //                propwash->setPropellerUnit( pu );
        //            }
        //        }
        //    }

        //    propwash->setStrength( propwashData.strength );

        //    Array<bool> affectedSections;
        //    affectedSections.resize( propwashData.affectedSections.size() );

        //    for(size_t i = 0; i < affectedSections.size(); ++i)
        //    {
        //        affectedSections[i] = propwashData.affectedSections[i] == 1;
        //    }

        //    propwash->setAffectedSections( affectedSections );
        //    propwash->setSectionMultipliers( propwashData.sectionMultipliers );

        //    m_propwashes.push_back( propwash );
        //}

        ////m_numWings = aircraftData.wingData.size();
        // int currentWingIdx = 0;

        // for(size_t i = 0; i < aircraftData.wingData.size(); ++i)
        //{
        //     auto &wingData = aircraftData.wingData[i];
        //     bool useSubDivide = false;

        //    Vector3<real_Num> subDivision( wingData.subDivision.x, wingData.subDivision.y,
        //                                   wingData.subDivision.z );
        //    if(subDivision.X() > 0 || subDivision.Y() > 0 || subDivision.Z() > 0)
        //    {
        //        useSubDivide = true;
        //    }

        //    if(useSubDivide)
        //    {
        //        Vector3<real_Num> localPosition( wingData.localTransform.position.x,
        //                                         wingData.localTransform.position.y,
        //                                         wingData.localTransform.position.z );
        //        Vector3<real_Num> localScale( wingData.localTransform.scale.x,
        //                                      wingData.localTransform.scale.y,
        //                                      wingData.localTransform.scale.z );

        //        if(subDivision.Z() > 0)
        //        {
        //            localScale.Z() = wingData.localTransform.scale.z / subDivision.Z();
        //        }

        //        Vector3<real_Num> segmentSize = localScale;

        //        segmentSize.X() = 0;
        //        segmentSize.Y() = 0;

        //        if(subDivision.Z() > 0)
        //        {
        //            segmentSize.Z() = wingData.localTransform.scale.z / subDivision.Z();
        //        }

        //        Vector3<real_Num> startPos = (
        //            localPosition - ( localScale * static_cast<real_Num>(0.5) * subDivision ) );
        //        Vector3<real_Num> endPos = (
        //            localPosition + ( localScale * static_cast<real_Num>(0.5) * subDivision ) );

        //        for(s32 x = 0; x <= static_cast<s32>(subDivision.X()); ++x)
        //        {
        //            for(s32 y = 0; y <= static_cast<s32>(subDivision.Y()); ++y)
        //            {
        //                for(s32 z = 0; z <= static_cast<s32>(subDivision.Z()); ++z)
        //                {
        //                    auto curWingData = wingData;

        //                    curWingData.name =
        //                        curWingData.name + "_" + StringUtil::toString( x ) + "_" +
        //                        StringUtil:: toString( y ) + "_" + StringUtil::toString( z );

        //                    Vector3<real_Num> wingLocalPosition =
        //                        startPos + ( endPos - startPos ) * ( static_cast<real_Num>(z) /
        //                                                             subDivision.Z() );

        //                    curWingData.localTransform.position = data::vec4(
        //                        wingLocalPosition.X(), wingLocalPosition.Y(), wingLocalPosition.Z()
        //                        );
        //                    curWingData.localTransform.scale = data::vec4(
        //                        localScale.X(), localScale.Y(), localScale.Z() );

        //                    auto wing = &m_wings[currentWingIdx++];

        //                    wing->setName( curWingData.name );
        //                    wing->setParentAircraft( pThis );
        //                    wing->setParent( body );
        //                    wing->load( &curWingData );

        //                    for(auto &c : m_controlSurfaces)
        //                    {
        //                        String controlSurfaceName = c->getName();
        //                        String wingName = wing->getName();

        //                        if(controlSurfaceName == wingName)
        //                        {
        //                            wing->setAttachedControlSurface( c );
        //                            break;
        //                        }
        //                    }

        //                    for(auto &p : m_propwashes)
        //                    {
        //                        String controlSurfaceName = p->getName();
        //                        String wingName = wing->getName();

        //                        if(controlSurfaceName == wingName)
        //                        {
        //                            wing->setAttachedPropWash( p );
        //                            break;
        //                        }
        //                    }
        //                }
        //            }
        //        }
        //    }
        //    else
        //    {
        //        auto wing = &m_wings[currentWingIdx++];

        //        wing->setName( wingData.name );
        //        wing->setParentAircraft( pThis );
        //        wing->setParent( body );
        //        wing->load( &wingData );

        //        for(auto c : m_controlSurfaces)
        //        {
        //            String controlSurfaceName = c->getName();
        //            String wingName = wing->getName();

        //            if(controlSurfaceName == wingName)
        //            {
        //                wing->setAttachedControlSurface( c );
        //                break;
        //            }
        //        }

        //        for(auto &p : m_propwashes)
        //        {
        //            String controlSurfaceName = p->getName();
        //            String wingName = wing->getName();

        //            if(controlSurfaceName == wingName)
        //            {
        //                wing->setAttachedPropWash( p );
        //                break;
        //            }
        //        }
        //    }
        //}

        // m_numWings = currentWingIdx;

        // for (auto c : m_controlSurfaces)
        //{
        //	auto wing = boost::make_shared<CAircraftWingDefault>();
        //	c->setControlWing(wing);
        // }
    }

    void CAircraftFast::loadProperties()
    {
        // if(!m_properties)
        //{
        //     m_properties = fb::make_ptr<Properties>();
        // }

        // m_properties->clearAll();
        // m_powerChannels.clear();

        // auto applicationManager = core::ApplicationManager::instance();
        // auto fileSystem = applicationManager->getFileSystem();
        // auto modelDataFilePath = getModelDataFilePath();

        // if(fileSystem->isExistingFile( modelDataFilePath ))
        //{
        //     auto doc = XMLUtil::loadFile( modelDataFilePath );
        //     if(!doc->Error())
        //     {
        //         auto root = doc->RootElement();
        //         if(root)
        //         {
        //             auto results = root->FirstChild();
        //             while(results)
        //             {
        //                 auto result = results->FirstChild( "result" );
        //                 while(result)
        //                 {
        //                     String title = XMLUtil::getText( result->FirstChildElement( "title" )
        //                     ); String value = XMLUtil::getText( result->FirstChildElement( "value"
        //                     ) );

        //                    m_properties->addProperty( title, value );

        //                    result = result->NextSiblingElement();
        //                }

        //                auto group = results->FirstChild( "group" );
        //                if(group)
        //                {
        //                    auto powerChannel = group->FirstChild( "powerchannel" );
        //                    if(powerChannel)
        //                    {
        //                        SmartPtr<Properties> properties = fb::make_ptr<Properties>();

        //                        auto result = group->FirstChild( "result" );
        //                        while(result)
        //                        {
        //                            String title = XMLUtil::getText(
        //                                result->FirstChildElement( "title" ) );
        //                            String value = XMLUtil::getText(
        //                                result->FirstChildElement( "value" ) );

        //                            properties->addProperty( title, value );

        //                            result = result->NextSiblingElement();
        //                        }

        //                        m_powerChannels.push_back( properties );
        //                    }
        //                }

        //                results = results->NextSibling();
        //            }
        //        }
        //    }
        //}
    }

    void CAircraftFast::loadDefaults()
    {
        // try
        //{
        //     data::aircraft_data aircraftData;
        //     //JsonUtil::parse(data, &aircraftData);

        //    // dummy data
        //    //setup default data for testing
        //    data::aircraft_engine_data e1;
        //    aircraftData.engineData.push_back( e1 );

        //    data::aircraft_wing_data rWingOuterData;
        //    rWingOuterData.sectionCount = 10;
        //    rWingOuterData.wingTipWidthZeroToOne = 0.43;
        //    rWingOuterData.wingTipSweep = 0.36;
        //    rWingOuterData.wingTipAngle = 0.0;
        //    rWingOuterData.cdOverride = 0.045;
        //    rWingOuterData.wingArea = 0.66;
        //    rWingOuterData.localTransform.position = data::vec4( -1.122, -0.212, -0.018 );
        //    rWingOuterData.localTransform.orientation = data::vec4(
        //        0.01967231, 0.9995907, 0.02076876, -0.0004087369 );
        //    rWingOuterData.localTransform.scale = data::vec4( 0.9154646, 0.1693609, 0.9529831 );
        //    aircraftData.wingData.push_back( rWingOuterData );

        //    data::aircraft_wing_data rWingOuterDataMirror = getMirror( rWingOuterData );
        //    aircraftData.wingData.push_back( rWingOuterDataMirror );

        //    data::aircraft_wing_data rStabilatorData;
        //    rStabilatorData.sectionCount = 10;
        //    rStabilatorData.wingTipWidthZeroToOne = 0.43;
        //    rStabilatorData.wingTipSweep = 0.36;
        //    rStabilatorData.wingTipAngle = 0.0;
        //    rStabilatorData.cdOverride = 0.045;
        //    rStabilatorData.wingArea = 0.66;
        //    rStabilatorData.localTransform.position = data::vec4( -0.434, -0.242, 1.521 );
        //    rStabilatorData.localTransform.orientation = data::vec4(
        //        0.01967231, 0.9995907, 0.02076876, -0.0004087369 );
        //    rStabilatorData.localTransform.scale = data::vec4( 0.6079797, 0.1693609, 0.9529831 );
        //    aircraftData.wingData.push_back( rStabilatorData );

        //    data::aircraft_wing_data rStabilatorDataMirror = getMirror( rStabilatorData );
        //    aircraftData.wingData.push_back( rStabilatorDataMirror );

        //    SmartPtr<CAircraftFast> pThis = getSharedFromThis<CAircraftFast>();

        //    SmartPtr<CAircraftBody> body = fb::make_ptr<CAircraftBody>();
        //    body->setParentAircraft( pThis );
        //    setBody( body );

        //    // create components
        //    for(auto &data : aircraftData.engineData)
        //    {
        //        SmartPtr<CAircraftEngine> e = fb::make_ptr<CAircraftEngine>();

        //        e->setParentAircraft( pThis );
        //        m_engines.push_back( e );
        //    }

        //    //for (size_t i=0; i<aircraftData.wingData.size(); ++i)
        //    //{
        //    //	SmartPtr<Wing> wing = boost::make_shared<Wing>();
        //    //	m_wings.push_back(wing);
        //    //}

        //    //SmartPtr<CAircraftFastWing> rWingOuter = boost::make_shared<CAircraftFastWing>();
        //    //rWingOuter->setParentAircraft(pThis);
        //    //rWingOuter->setParent(body);
        //    //rWingOuter->load(&rWingOuterData);
        //    //m_wings.push_back(rWingOuter);

        //    //SmartPtr<CAircraftFastWing> rWingOuterMirror =
        //    boost::make_shared<CAircraftFastWing>();
        //    //rWingOuterMirror->setParentAircraft(pThis);
        //    //rWingOuterMirror->setParent(body);
        //    //rWingOuterMirror->load(&rWingOuterDataMirror);
        //    //m_wings.push_back(rWingOuterMirror);

        //    //SmartPtr<CAircraftFastWing> rStabilator = boost::make_shared<CAircraftFastWing>();
        //    //rStabilator->setParentAircraft(pThis);
        //    //rStabilator->setParent(body);
        //    //rStabilator->load(&rStabilatorData);
        //    //m_wings.push_back(rStabilator);

        //    //SmartPtr<CAircraftFastWing> rStabilatorMirror =
        //    boost::make_shared<CAircraftFastWing>();
        //    //rStabilatorMirror->setParentAircraft(pThis);
        //    //rStabilatorMirror->setParent(body);
        //    //rStabilatorMirror->load(&rStabilatorDataMirror);
        //    //m_wings.push_back(rStabilatorMirror);

        //    std::string jsonStr = DataUtil::toString( &aircraftData );

        //    std::fstream fs;
        //    fs.open( "fw_default.json", std::fstream::out | std::fstream::trunc );
        //    fs << jsonStr;
        //    fs.close();
        //}
        // catch(std::exception &e)
        //{
        //    WP_LOG_EXCEPTION( e );
        //}
    }

    void CAircraftFast::reload(void *pData)
    {
        // std::string filePath = "fw.json";
        // std::fstream stream(filePath, std::fstream::in);
        // Ogre::DataStreamPtr pStream(OGRE_NEW Ogre::FileStreamDataStream(&stream, false));
        // if (pStream)
        //{
        //	std::string data = pStream->getAsString();
        //	loadFromString(data);
        // }
    }

    void CAircraftFast::unload(SmartPtr<ISharedObject> data)
    {
        m_wind = nullptr;
        m_batteryPack = nullptr;

        for(auto esc : m_escs)
        {
            esc->unload(data);
        }

        for(auto e : m_engines)
        {
            e->unload(data);
        }

        for(auto &w : m_wings)
        {
            w.unload(data);
        }

        for(auto w : m_wheels)
        {
            w->unload(data);
        }

        for(auto c : m_controlSurfaces)
        {
            c->unload(data);
        }

        for(auto p : m_propellers)
        {
            p->unload(data);
        }

        for(auto p : m_propellerUnits)
        {
            p->unload(data);
        }

        for(auto p : m_propwashes)
        {
            p->unload(data);
        }

        m_propwashes.clear();
        m_escs.clear();
        m_engines.clear();
        m_controlSurfaces.clear();
        m_propellers.clear();
        m_propellerUnits.clear();
        m_wheels.clear();

        m_properties = nullptr;
        m_powerChannels.clear();

        m_rigidbody = nullptr;

        // m_bodyTransform = nullptr;
        // m_worldTransform = nullptr;
        // m_localTransform = nullptr;

        m_callback = nullptr;
    }

    void CAircraftFast::setBody(SmartPtr<IVehicleBody> body)
    {
        m_rigidbody = body;
    }

    void CAircraftFast::getAllTxData()
    {
        if(m_callback)
        {
            m_channels = m_callback->getInputData();
        }
    }

    f32 CAircraftFast::getChannel(s32 idx) const
    {
        return m_channels[idx];
    }

    void CAircraftFast::setChannel(s32 idx, f32 channel)
    {
        m_channels[idx] = channel;
    }

    void CAircraftFast::addForce(const Vector3<real_Num> &force)
    {
        SpinRWMutex::ScopedLock lock(m_forceMutex, true);
        m_force += force;
    }

    void CAircraftFast::addTorque(const Vector3<real_Num> &torque)
    {
        SpinRWMutex::ScopedLock lock(m_torqueMutex, true);
        m_torque += torque;
    }

    void CAircraftFast::clearForces()
    {
        SpinRWMutex::ScopedLock lock0(m_forceMutex, true);
        SpinRWMutex::ScopedLock lock1(m_torqueMutex, true);
        m_force = Vector3<real_Num>::zero();
        m_torque = Vector3<real_Num>::zero();
    }

    void CAircraftFast::addForce(int Bdy, const Vector3<real_Num> &Force,
                                 const Vector3<real_Num> &Loc)
    {
        WP_ASSERT(Force.length() < 1e5);
        WP_ASSERT(Loc.length() < 1e5);

        // if (m_callback)
        //{
        //	m_callback->addForce(Bdy, Force, Loc);
        // }

        const Vector3<real_Num> centerOfMass = m_worldTransform.transformPoint(m_cg);
        const Vector3<real_Num> torque = (Loc - centerOfMass).crossProduct(Force);

        addForce(Force);
        addTorque(torque);
    }

    void CAircraftFast::addTorque(int Bdy, const Vector3<real_Num> &Torque)
    {
        WP_ASSERT(Torque.length() < 1e5);

        // if (m_callback)
        //{
        //	m_callback->addTorque(Bdy, Torque);
        // }

        addTorque(Torque);
    }

    void CAircraftFast::addLocalForce(int Bdy, const Vector3<real_Num> &Force,
                                      const Vector3<real_Num> &Loc)
    {
        WP_ASSERT(Force.length() < 1e5);
        WP_ASSERT(Loc.length() < 1e5);

        // if (m_callback)
        //{
        //	m_callback->addLocalForce(Bdy, Force, Loc);
        // }

        Vector3<real_Num> worldPoint = m_worldTransform.transformPoint(Loc);
        Vector3<real_Num> force = m_worldTransform.transformVector(Force);

        const Vector3<real_Num> centerOfMass = m_worldTransform.transformPoint(m_cg);
        const Vector3<real_Num> torque = (worldPoint - centerOfMass).crossProduct(force);

        addForce(force);
        addTorque(torque);
    }

    void CAircraftFast::addLocalTorque(int Bdy, const Vector3<real_Num> &Torque)
    {
        WP_ASSERT(Torque.length() < 1e5);

        // if (m_callback)
        //{
        //	m_callback->addLocalTorque(Bdy, Torque);
        // }

        Vector3<real_Num> torque = m_worldTransform.getOrientation() * Torque;
        addTorque(torque);
    }

    real_Num CAircraftFast::getRollwiseDamping() const
    {
        return m_rollwiseDamping;
    }

    void CAircraftFast::setRollwiseDamping(real_Num rollwiseDamping)
    {
        m_rollwiseDamping = rollwiseDamping;
    }

    Vector3<real_Num> CAircraftFast::getAngularVelocity()
    {
        if(m_callback)
        {
            return m_callback->getAngularVelocity();
        }

        return Vector3<real_Num>::zero();
    }

    Vector3<real_Num> CAircraftFast::getLinearVelocity()
    {
        if(m_callback)
        {
            return m_callback->getLinearVelocity();
        }

        return Vector3<real_Num>::zero();
    }

    Vector3<real_Num> CAircraftFast::getLocalAngularVelocity()
    {
        if(m_callback)
        {
            return m_callback->getLocalAngularVelocity();
        }

        return Vector3<real_Num>::zero();
    }

    Vector3<real_Num> CAircraftFast::getPosition()
    {
        if(m_callback)
        {
            return m_callback->getPosition();
        }

        return Vector3<real_Num>::zero();
    }

    Vector3<real_Num> CAircraftFast::getPosition() const
    {
        return Vector3<real_Num>::zero();
    }

    void CAircraftFast::setPosition(const Vector3<real_Num> &position)
    {
    }

    bool CAircraftFast::isUserControlled() const
    {
        return true;
    }

    void CAircraftFast::setUserControlled(bool userControlled)
    {
    }

    real_Num CAircraftFast::getMass() const
    {
        return m_rigidbody->getMass();
    }

    void CAircraftFast::setMass(real_Num mass)
    {
        m_rigidbody->setMass(mass);
    }

    void CAircraftFast::setDisplayDebugData(bool displayDebugData)
    {
        m_displayDebugData = displayDebugData;
    }

    Quaternion<real_Num> CAircraftFast::getOrientation()
    {
        if(m_callback)
        {
            return m_callback->getOrientation();
        }

        return Quaternion<real_Num>::identity();
    }

    Transform3<real_Num> CAircraftFast::getWorldTransform() const
    {
        return m_worldTransform;
    }

    void CAircraftFast::updateTransform()
    {
        auto p = getPosition();
        auto q = getOrientation();

        WP_ASSERT(MathUtil<real_Num>::isFinite( p ));
        WP_ASSERT(MathUtil<real_Num>::isFinite( q ));

        // p = Vector3<real_Num>::zero();
        // q = Quaternion<real_Num>::identity();

        // if (m_bodyTransform)
        {
            m_bodyTransform.setPosition(p);
            m_bodyTransform.setOrientation(q);
            m_bodyTransform.setScale(Vector3<real_Num>::unit());

            m_worldTransform.transformFromParent(m_bodyTransform, m_localTransform);
        }

        if(m_rigidbody)
        {
            auto cg = m_bodyTransform.transformPoint(m_cg);
            m_rigidbody->setWorldCenterOfMass(cg);
        }

        for(auto &e : m_engines)
        {
            e->updateTransform();
        }

        for(auto &w : m_wings)
        {
            w.updateTransform();
        }

        for(auto &w : m_wheels)
        {
            w->updateTransform();
        }
    }

    // data::aircraft_wing_data CAircraftFast::getMirror( const data::aircraft_wing_data &wingData )
    //{
    //     data::aircraft_wing_data r = wingData;
    //     r.isMirror = true;

    //    if(r.isMirror)
    //    {
    //        r.localTransform.position.x = -wingData.localTransform.position.x;
    //    }

    //    return r;
    //}

    void CAircraftFast::setWorldTransform(const Transform3<real_Num> &worldTransform)
    {
        m_worldTransform = worldTransform;
    }

    Transform3<real_Num> CAircraftFast::getLocalTransform() const
    {
        return m_localTransform;
    }

    void CAircraftFast::setLocalTransform(const Transform3<real_Num> &localTransform)
    {
        m_localTransform = localTransform;
    }

    void CAircraftFast::drawPoint(int Bdy, int id, const Vector3<real_Num> &positon, u32 color)
    {
        auto size = 0.1f;
        auto offset0 = Vector3<real_Num>::forward() * size;
        auto offset1 = Vector3<real_Num>::up() * size;
        auto offset2 = Vector3<real_Num>::right() * size;

        auto p = m_worldTransform.getOrientation() * positon;
        p += m_worldTransform.getPosition();

        displayVector(Bdy, id, p + offset0, p - offset0, color);
        displayVector(Bdy, id + 1, p + offset1, p - offset1, color);
        displayVector(Bdy, id + 2, p + offset2, p - offset2, color);
    }

    void CAircraftFast::drawLocalPoint(int Bdy, int id, const Vector3<real_Num> &positon,
                                       u32 color)
    {
        auto size = 0.1f;
        auto offset0 = Vector3<real_Num>::forward() * size;
        auto offset1 = Vector3<real_Num>::up() * size;
        auto offset2 = Vector3<real_Num>::right() * size;

        auto p = m_worldTransform.getOrientation() * positon;
        p += m_worldTransform.getPosition();

        displayVector(Bdy, id, p + offset0, p - offset0, color);
        displayVector(Bdy, id + 1, p + offset1, p - offset1, color);
        displayVector(Bdy, id + 2, p + offset2, p - offset2, color);
    }

    void CAircraftFast::displayLocalVector(int Bdy, const Vector3<real_Num> &V,
                                           const Vector3<real_Num> &Org, u32 colour)
    {
        try
        {
            if(m_callback)
            {
                m_callback->displayLocalVector(Bdy, Vector3<real_Num>(V.X(), V.Y(), V.Z()),
                                               Vector3<real_Num>(Org.X(), Org.Y(), Org.Z()),
                                               colour);
            }
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    }

    void CAircraftFast::displayLocalVector(int Bdy, int id, const Vector3<real_Num> &V,
                                           const Vector3<real_Num> &Org, u32 colour)
    {
        try
        {
            if(m_callback)
            {
                Vector3<real_Num> start = getWorldTransform().transformPoint(V);
                Vector3<real_Num> end = getWorldTransform().transformPoint(Org);

                m_callback->displayVector(Bdy, id, start, end, colour);
            }
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    }

    SmartPtr<IAircraftCallback> CAircraftFast::getCallback() const
    {
        return m_callback;
    }

    void CAircraftFast::setCallback(SmartPtr<IAircraftCallback> callback)
    {
        m_callback = callback;
    }

    Transform3<real_Num> CAircraftFast::getBodyTransform() const
    {
        return m_bodyTransform;
    }

    void CAircraftFast::displayVector(int Bdy, int id, const Vector3<real_Num> &V,
                                      const Vector3<real_Num> &Org, u32 colour)
    {
        try
        {
            if(m_callback)
            {
                m_callback->displayVector(Bdy, id, Vector3<real_Num>(V.X(), V.Y(), V.Z()),
                                          Vector3<real_Num>(Org.X(), Org.Y(), Org.Z()), colour);
            }
        }
        catch(std::exception &Err)
        {
            WP_LOG_EXCEPTION(Err);
        }
    }

    Vector3<real_Num> CAircraftFast::getLocalLinearVelocity()
    {
        if(m_callback)
        {
            return m_callback->getLocalLinearVelocity();
        }

        return Vector3<real_Num>::zero();
    }

    void CAircraftFast::setBodyTransform(Transform3<real_Num> bodyTransform)
    {
        m_bodyTransform = bodyTransform;
    }

    void CAircraftFast::setControlAngle(int id, float angle)
    {
    }

    Vector3<real_Num> CAircraftFast::getPointVelocity(const Vector3<real_Num> &p)
    {
        if(m_callback)
        {
            return m_callback->getPointVelocity(p);
        }

        return Vector3<real_Num>::zero();
    }

    real_Num CAircraftFast::getAirDensity() const
    {
        return m_airDensity;
    }

    void CAircraftFast::setAirDensity(real_Num airDensity)
    {
        m_airDensity = airDensity;
    }

    const Array<SmartPtr<Properties>> &CAircraftFast::getPowerChannels() const
    {
        return m_powerChannels;
    }

    Array<SmartPtr<Properties>> &CAircraftFast::getPowerChannels()
    {
        return m_powerChannels;
    }

    void CAircraftFast::setPowerChannels(Array<SmartPtr<Properties>> powerChannels)
    {
        m_powerChannels = powerChannels;
    }

    bool CAircraftFast::getEmulateBattery() const
    {
        return m_emulateBattery;
    }

    void CAircraftFast::setEmulateBattery(bool emulateBattery)
    {
        m_emulateBattery = emulateBattery;
    }

    bool CAircraftFast::isElectric() const
    {
        return m_bIsElectric;
    }

    void CAircraftFast::setElectric(bool bIsElectric)
    {
        m_bIsElectric = bIsElectric;
    }

    SmartPtr<IBatteryPack> CAircraftFast::getBatteryPack() const
    {
        return m_batteryPack;
    }

    void CAircraftFast::setBatteryPack(SmartPtr<IBatteryPack> batteryPack)
    {
        m_batteryPack = batteryPack;
    }

    bool CAircraftFast::getEnablePowerUnit() const
    {
        return m_enablePowerUnit;
    }

    void CAircraftFast::setEnablePowerUnit(bool enablePowerUnit)
    {
        m_enablePowerUnit = enablePowerUnit;
    }

    SmartPtr<IAerodymanicsWind> CAircraftFast::getWind() const
    {
        return m_wind;
    }

    void CAircraftFast::setWind(SmartPtr<IAerodymanicsWind> wind)
    {
        m_wind = wind;
    }

    void CAircraftFast::addPropellerUnit(SmartPtr<IAircraftPropellerUnit> propellerUnit)
    {
        m_propellerUnits.push_back(propellerUnit);
    }

    void CAircraftFast::removePropellerUnit(SmartPtr<IAircraftPropellerUnit> propellerUnit)
    {
        auto it = std::find(m_propellerUnits.begin(), m_propellerUnits.end(), propellerUnit);
        if(it != m_propellerUnits.end())
        {
            m_propellerUnits.erase(it);
        }
    }

    Array<SmartPtr<IAircraftPropellerUnit>> CAircraftFast::getPropellerUnits() const
    {
        return m_propellerUnits;
    }

    void CAircraftFast::setPropellerUnits(
        const Array<SmartPtr<IAircraftPropellerUnit>> &propellerUnits)
    {
        m_propellerUnits = propellerUnits;
    }

    void CAircraftFast::addWheel(SmartPtr<IWheelComponent> wheel)
    {
        SmartPtr<IVehicle> pThis = getSharedFromThis<IVehicle>();

        wheel->setOwner(pThis);
        m_wheels.push_back(wheel);
    }

    void CAircraftFast::removeWheel(SmartPtr<IWheelComponent> wheel)
    {
        auto it = std::find(m_wheels.begin(), m_wheels.end(), wheel);
        if(it != m_wheels.end())
        {
            m_wheels.erase(it);
        }
    }

    Array<SmartPtr<IWheelComponent>> CAircraftFast::getWheels() const
    {
        return m_wheels;
    }

    void CAircraftFast::setWheels(const Array<SmartPtr<IWheelComponent>> &wheels)
    {
        m_wheels = wheels;
    }

    real_Num CAircraftFast::getSectionMultiplier() const
    {
        return m_sectionMultiplier;
    }

    void CAircraftFast::setSectionMultiplier(real_Num sectionMultiplier)
    {
        m_sectionMultiplier = sectionMultiplier;
    }

    String CAircraftFast::getModelDataFilePath() const
    {
        return m_modelDataFilePath;
    }

    void CAircraftFast::setModelDataFilePath(const String &modelDataFilePath)
    {
        m_modelDataFilePath = modelDataFilePath;
    }

    real_Num CAircraftFast::getThrust(int idx) const
    {
        if(idx < m_propellerUnits.size())
        {
            auto thrust = m_propellerUnits[idx]->getThrust();
            return thrust.length();
        }

        return 0.0;
    }
}
