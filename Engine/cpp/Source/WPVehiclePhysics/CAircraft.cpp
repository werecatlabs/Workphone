#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CAircraft.hpp>
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
#include <WPVehiclePhysics/CAircraftPropellerUnitSimple.hpp>
#include <WPVehiclePhysics/InputController.hpp>
#include <WPVehiclePhysics/CAerodymanicsWind.hpp>
#include <WPVehiclePhysics/CAircraftPropWash.hpp>
#include <WPVehiclePhysics/WheelControllerArcade.hpp>
#include <WPVehiclePhysics/WheelControllerPacejka.hpp>
#include <Workphone/Workphone.hpp>
#include <fstream>

namespace workphone::vehicle
{
    using CAircraftWingDefault = CAircraftWing;
    using CWheelControllerDefault = WheelControllerArcade;
    // typedef CWheelControllerPacejka CWheelControllerDefault;
    // typedef CAircraftPropellerUnitSimple CAircraftPropellerUnitDefault;
    using CAircraftPropellerUnitDefault = CAircraftPropellerUnit;

    CAircraft::CAircraft()
    {
        // Stable defaults for scene-created aircraft without a model data file.
        // Z is the low-drag forward axis used by the propeller implementation.
        m_drag = Vector3<real_Num>(0.55, 0.85,
                                   0.08);
    }

    CAircraft::~CAircraft() = default;

    void CAircraft::load(SmartPtr<ISharedObject> data)
    {
        if(isLoaded())
        {
            return;
        }

        setLoadingState(LoadingState::Loading);
        CAerodynamicsVehicle<IAircraft>::load(data);
        loadFromData({});
        setLoadingState(LoadingState::Loaded);
    }

    real_Num CAircraft::getEngineRPM(int idx) const
    {
        if(idx < static_cast<s32>(m_engines.size()))
        {
            const SmartPtr<IAircraftPowerUnit> engine = m_engines[idx];
            return engine->getRPM();
        }

        return 0.0;
    }

    s32 CAircraft::getKey()
    {
#if defined WP_PLATFORM_WIN32
        //	for (int i = 8; i <= 256; i++)
        //	{
        //		if (GetAsyncKeyState(i) & 0x7FFF)
        //		{

        //			// This if filters the keys, i want to allow direction arrows
        //			// and q for quit. If you want to add more just add the code for the key,
        //			// to know the key code just coment the if line and print the keycode.
        //			if ((i >= 37 && i <= 40) || i == 81)
        //				return i;
        //	}
        //}

        // if ((GetAsyncKeyState(m_keyLeft) & 0x8000) != 0)
        //{
        //	return m_keyLeft;
        // }

        if((GetAsyncKeyState(m_keyTop) & 0x8000) != 0)
        {
            return m_keyTop;
        }

        if((GetAsyncKeyState(m_keyRight) & 0x8000) != 0)
        {
            return m_keyRight;
        }

        if((GetAsyncKeyState(m_keyDown) & 0x8000) != 0)
        {
            return m_keyDown;
        }

        return -1;
#else
        return -1;
#endif
    }

#if defined WP_PLATFORM_WIN32
#elif defined WP_PLATFORM_LINUX
    ///**
    // *
    // * @param ks  like XK_Shift_L, see /usr/include/X11/keysymdef.h
    // * @return
    // */
    // bool key_is_pressed(KeySym ks) {
    //	Display* dpy = XOpenDisplay(":0");
    //	char keys_return[32];
    //	XQueryKeymap(dpy, keys_return);
    //	KeyCode kc2 = XKeysymToKeycode(dpy, ks);
    //	bool isPressed = !!(keys_return[kc2 >> 3] & (1 << (kc2 & 7)));
    //	XCloseDisplay(dpy);
    //	return isPressed;
    //}

    // bool ctrl_is_pressed() {
    //	return key_is_pressed(XK_Control_L) || key_is_pressed(XK_Control_R);
    // }

    // bool shift_is_pressed() {
    //	return key_is_pressed(XK_Shift_L) || key_is_pressed(XK_Shift_R);
    // }
#endif

    void CAircraft::update()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto timer = applicationManager ? applicationManager->getTimerPtr() : nullptr;
        if(!timer)
        {
            return;
        }

        const auto dt = static_cast<real_Num>(timer->getDeltaTime());
        if(dt <= std::numeric_limits<real_Num>::epsilon())
        {
            return;
        }

        update(timer->getTime(), dt);
        CAerodynamicsVehicle<IAircraft>::update();
    }

    void CAircraft::update(const double &t, const double &fDT)
    {
#if defined WP_PLATFORM_WIN32
        // using namespace std;

        // auto shift = (::GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        // auto ctrl = (::GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;

        // if (ctrl)
        //{
        //	int keyCode = getKey();
        //	if (keyCode == m_keyLeft)
        //	{
        //		for (auto e : m_engines)
        //		{
        //			auto motor = fb::dynamic_pointer_cast<CAircraftMotor>(e);
        //			if (motor)
        //			{
        //				auto msrgain = motor->getMsrGain();

        //				motor->setMsrGain(msrgain - (0.1 * fDT));
        //			}
        //		}
        //	}
        //	else if (keyCode == m_keyRight)
        //	{
        //		for (auto e : m_engines)
        //		{
        //			auto motor = fb::dynamic_pointer_cast<CAircraftMotor>(e);
        //			if (motor)
        //			{
        //				auto msrgain = motor->getMsrGain();

        //				motor->setMsrGain(msrgain + (0.1 * fDT));
        //			}
        //		}
        //	}
        //}
#elif defined WP_PLATFORM_LINUX

        // if (shift_is_pressed())
        //{
        //	if (key_is_pressed(XK_Left))
        //	{
        //		CDFiddle = CDFiddle - (dt * 1.0);
        //		WP_LOG_ERROR("cd: " + StringUtil::toString((float)CDFiddle));
        //	}

        //	if (key_is_pressed(XK_Right))
        //	{
        //		CDFiddle = CDFiddle + (dt * 1.0);
        //		WP_LOG_ERROR("cd: " + StringUtil::toString((float)CDFiddle));
        //	}

        //	if (key_is_pressed(XK_Up))
        //	{
        //		CLFiddle = CLFiddle + (dt * 1.0);
        //		WP_LOG_ERROR("cl: " + StringUtil::toString((float)CLFiddle));
        //	}

        //	if (key_is_pressed(XK_Down))
        //	{
        //		CLFiddle = CLFiddle - (dt * 1.0);
        //		WP_LOG_ERROR("cl: " + StringUtil::toString((float)CLFiddle));
        //	}
        //}
#endif

        // return;
        // WP_LOG(String("Delta: ") + StringUtil::toString(1.0 / fDT));

        if(!getBody() || !m_callback)
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

        WP_ASSERT(getBody());

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

        getAllTxData();
        updateTransform();

        // if (m_rigidbody)
        //{
        //	m_rigidbody->update(t, dt);
        // }

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

        // for (auto& w : m_wings)
        //{
        //	WP_ASSERT(w);
        //	w->update(t, dt);
        // }

        // for (auto& c : m_controlSurfaces)
        //{
        //	WP_ASSERT(c);
        //	c->update(t, dt);
        // }

        auto steeringAngle = controlAngles[5];

        for(auto &w : m_wheels)
        {
            if(w->isSteeringWheel())
            {
                w->setSteeringAngle(steeringAngle);
            }
        }

        // if (!getEnablePowerUnit())
        //{
        //	static auto steeringAngle2 = -30.0;
        //	if (Math<real_Num>::Abs(steeringAngle2) > 0.3)
        //	{
        //		addLocalTorque(0, Vector3<real_Num>::UNIT_Y * steeringAngle2 * getMass() * 0.001);
        //	}
        // }

        // for (auto& w : m_wheels)
        //{
        //	WP_ASSERT(w);
        //	w->update(t, dt);
        // }

        updateDefaultAerodynamics(static_cast<real_Num>(dt));
        updateDrag(t, dt);

        if(m_callback)
        {
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

        static auto nextUpdate = 0.0;
        if(nextUpdate < t)
        {
            // WP_LOG("Yaw Rate: " + StringUtil::toString(getLocalAngularVelocity().Y()));
            nextUpdate = t + 3.0;
        }
    }

    void CAircraft::updateDefaultAerodynamics(real_Num dt)
    {
        if(!m_wings.empty() || !getBody())
        {
            return;
        }

        const auto localVelocity = getLocalLinearVelocity();
        const auto forwardSpeed =
            Math<real_Num>::max(0.0, -localVelocity.Z());
        const auto dynamicPressure =
            static_cast<real_Num>(0.5) * m_airDensity * forwardSpeed * forwardSpeed;

        // A moderate fallback lift curve keeps scene-created aircraft useful
        // while detailed imported aircraft continue to use their wing sections.
        const auto lift = Math<real_Num>::min(dynamicPressure * m_sectionMultiplier *
                                              static_cast<real_Num>(0.35),
                                              80000.0);

        const auto throttle =
            Math<real_Num>::clamp(getChannel(m_thrChannel),
                                  0.0, 1.0);
        const auto thrust = throttle *
                            Math<real_Num>::max(getMass(), 1.0) *
                            static_cast<real_Num>(8.0);

        addLocalForce(0, Vector3<real_Num>(0, lift, -thrust), m_cg);

        const auto roll =
            Math<real_Num>::clamp(getChannel(m_ailChannel),
                                  -1.0, 1.0);
        const auto pitch =
            Math<real_Num>::clamp(getChannel(m_eleChannel),
                                  -1.0, 1.0);
        const auto yaw =
            Math<real_Num>::clamp(getChannel(m_yawChannel),
                                  -1.0, 1.0);

        const auto mass = Math<real_Num>::max(getMass(), 1.0);
        auto controlTorque = Vector3<real_Num>(pitch * mass * static_cast<real_Num>(2.5),
                                               yaw * mass * static_cast<real_Num>(1.5),
                                               -roll * mass * static_cast<real_Num>(3.0));

        const auto angularVelocity = getLocalAngularVelocity();
        controlTorque -= angularVelocity * m_rollwiseDamping * mass;
        addLocalTorque(0, controlTorque * Math<real_Num>::clamp(dt * static_cast<real_Num>(60.0),
                                                                0.0,
                                                                1.0));
    }

    //-----------------------------------------------
    Vector3<real_Num> CAircraft::getDrag() const
    {
        return m_drag;
    }

    //-----------------------------------------------
    void CAircraft::updateDrag(const double &t, const double &dt)
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

        // localVelocity.X() = modelLocalVelocity.X() * 2.0 * -Xcoef;
        // localVelocity.Y() = modelLocalVelocity.Y() * 2.0 * -Ycoef;
        // localVelocity.Z() = modelLocalVelocity.Z() * 2.0 * -Zcoef;

        WP_ASSERT(localVelocity.length() < 1e5);

        if(localVelocity.length() < 1e5)
        {
            addLocalForce(0, localVelocity, Vector3<real_Num>::zero());
        }
    }

    IVehicleBody *CAircraft::getBodyPtr() const
    {
        return m_rigidbody.get();
    }

    SmartPtr<IVehicleBody> CAircraft::getBody() const
    {
        return m_rigidbody;
    }

    bool CAircraft::isValid() const
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

    void CAircraft::loadFromData(const String &data)
    {
        try
        {
            // auto worldTransform = fb::make_ptr<Transform3<real_Num>>();
            // setWorldTransform(worldTransform);

            // auto localTransform = fb::make_ptr<Transform3<real_Num>>();
            // setLocalTransform(localTransform);

            // auto bodyTransform = fb::make_ptr<Transform3<real_Num>>();
            // m_bodyTransform = bodyTransform;

            m_properties = workphone::make_ptr<Properties>();

            SmartPtr<IAerodymanicsWind> wind = workphone::make_ptr<CAerodymanicsWind>();
            setWind(wind);

            SmartPtr<CAircraft> pThis = getSharedFromThis<CAircraft>();

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

            for(auto &w : m_wings)
            {
                w->updateBodyTransform();
            }

            for(auto &w : m_wings)
            {
                w->updateGeometry();
            }

            for(auto &w : m_controlSurfaces)
            {
                w->updateGeometry();
            }
        }
        catch(std::exception &e)
        {
            WP_LOG_EXCEPTION(e);
        }
    }

    void CAircraft::loadFromString(const String &data)
    {
        WP_LOG(data);

        m_batteryPack = nullptr;
        m_escs.clear();
        m_engines.clear();
        m_wings.clear();
        m_controlSurfaces.clear();
        m_propellers.clear();
        m_propellerUnits.clear();
        m_wheels.clear();
        m_propwashes.clear();

        /*
            m_aircraftData = data::aircraft_data();

            DataUtil::parse( data, &m_aircraftData );

            m_cg = Vector3<real_Num>( m_aircraftData.cgPosition.x, m_aircraftData.cgPosition.y,
                                      m_aircraftData.cgPosition.z );
            m_drag = Vector3<real_Num>( m_aircraftData.drag.x, m_aircraftData.drag.y,
                                        m_aircraftData.drag.z );
            m_rollwiseDamping = m_aircraftData.rollwiseDamping;
            m_sectionMultiplier = m_aircraftData.sectionMultiplier;

            data::vec4 p = m_aircraftData.localTransform.position;
            data::vec4 q = m_aircraftData.localTransform.orientation;
            data::vec4 s = m_aircraftData.localTransform.scale;

            auto vPos = Vector3<real_Num>( p.x, p.y, -p.z );
            auto vScale = Vector3<real_Num>( s.x, s.y, s.z );
            auto qRot = Quaternion<real_Num>( q.w, -q.x, -q.y, q.z );

            auto localTransform = getLocalTransform();
            localTransform.setPosition( vPos );
            localTransform.setScale( vScale );
            localTransform.setOrientation( qRot );

            updateTransform();

            SmartPtr<CAircraft> pThis = getSharedFromThis<CAircraft>();

            SmartPtr<IVehicleBody> body = getBody();
            //body->setParentAircraft(pThis);
            //setBody(body);

            if(isElectric())
            {
                SmartPtr<CBatteryPack> b = fb::make_ptr<CBatteryPack>();
                b->load( m_properties );
                b->setParentAircraft( pThis );
                b->setParent( body );
                setBatteryPack( b );

                s32 count = 0;
                for(auto &data : m_aircraftData.engineData)
                {
                    if(count < static_cast<s32>(m_powerChannels.size()))
                    {
                        SmartPtr<Properties> properties = m_powerChannels[count];

                        SmartPtr<CAircraftMotor> e = fb::make_ptr<CAircraftMotor>();
                        SmartPtr<CAircraftPropeller> p = fb::make_ptr<CAircraftPropeller>();
                        auto propellerUnit = fb::make_ptr<CAircraftPropellerUnitDefault>();

                        SmartPtr<CESController> esc = fb::make_ptr<CESController>();
                        esc->load( properties );
                        esc->setParentAircraft( pThis );
                        esc->setParent( body );
                        m_escs.push_back( esc );

                        esc->setBatteryPack( b );

                        p->setParentAircraft( pThis );
                        p->setParent( body );

                        e->setName( data.name );
                        e->setParentAircraft( pThis );
                        e->setParent( body );
                        e->setESC( esc );
                        e->setBatteryPack( b );
                        e->load( &data );
                        e->setPropeller( p );

                        p->load( properties );
                        e->load( properties );

                        propellerUnit->setPowerUnit( e );
                        propellerUnit->setPropeller( p );
                        propellerUnit->setBatteryPack( b );
                        propellerUnit->setESC( esc );
                        propellerUnit->setParentAircraft( pThis );
                        propellerUnit->setParent( body );

                        esc->setMotor( e );

                        m_engines.push_back( e );
                        m_propellers.push_back( p );
                        m_propellerUnits.push_back( propellerUnit );
                    }

                    count++;
                }
            }
            else
            {
                s32 count = 0;
                for(auto &data : m_aircraftData.engineData)
                {
                    if(count < static_cast<s32>(m_powerChannels.size()))
                    {
                        SmartPtr<Properties> properties = m_powerChannels[count];
                        SmartPtr<CAircraftEngine> e = fb::make_ptr<CAircraftEngine>();
                        SmartPtr<CAircraftPropeller> p = fb::make_ptr<CAircraftPropeller>();
                        SmartPtr<CAircraftPropellerUnitDefault> propellerUnit = fb::make_ptr<
                            CAircraftPropellerUnitDefault>();

                        p->setParentAircraft( pThis );
                        p->setParent( body );

                        e->setName( data.name );
                        e->setParentAircraft( pThis );
                        e->setParent( body );
                        e->load( &data );
                        e->setPropeller( p );

                        p->load( properties );
                        e->load( properties );

                        propellerUnit->setPowerUnit( e );
                        propellerUnit->setPropeller( p );
                        propellerUnit->setParentAircraft( pThis );
                        propellerUnit->setParent( body );

                        m_engines.push_back( e );
                        m_propellers.push_back( p );
                        m_propellerUnits.push_back( propellerUnit );
                    }
                }
            }

            for(auto &data : m_aircraftData.wheelData)
            {
                auto wheel = fb::make_ptr<CWheelControllerDefault>();

                wheel->setOwner( pThis );
                wheel->setName( data.name );
                wheel->setMass( data.mass );
                wheel->setRadius( data.radius );
                wheel->setDamping( data.suspensionDamper );
                wheel->setSuspensionTravel( data.suspensionDistance * 0.5 );
                wheel->setSuspensionDistance( data.suspensionDistance );
                wheel->setSpringRate( data.springRate );
                wheel->setSteeringWheel( data.isSteeringWheel );

                auto p = data.localTransform.position;
                auto q = data.localTransform.orientation;
                auto s = data.localTransform.scale;

                auto vPos = Vector3<real_Num>( p.x, p.y, -p.z );
                auto vScale = Vector3<real_Num>( s.x, s.y, s.z );
                auto qRot = Quaternion<real_Num>( q.w, -q.x, -q.y, q.z );

                auto localTransform = wheel->getLocalTransform();
                localTransform.setPosition( vPos );
                localTransform.setScale( vScale );
                localTransform.setOrientation( qRot );

                m_wheels.push_back( wheel );
            }

            for(auto &data : m_aircraftData.controlSurfaceData)
            {
                SmartPtr<CAircraftControlSurface> c = fb::make_ptr<CAircraftControlSurface>();
                c->setParentAircraft( pThis );
                c->setSurfaceId( data.surfaceId );
                c->setName( data.name );
                c->setReversed( data.reverse );
                c->setTipHingeDistanceFromTrailingEdge( data.tipHingeDistanceFromTrailingEdge );
                c->setRootHingeDistanceFromTrailingEdge( data.rootHingeDistanceFromTrailingEdge );
                c->setRotationAxis( Vector3<real_Num>( data.modelRotationAxis.x,
            data.modelRotationAxis.y, -data.modelRotationAxis.z ) );

                c->load( &data );

                Array<bool> affectedSections;
                affectedSections.resize( data.affectedSections.size() );

                for(size_t i = 0; i < affectedSections.size(); ++i)
                {
                    affectedSections[i] = data.affectedSections[i] == 1;
                }

                c->setAffectedSections( affectedSections );

                SmartPtr<InputController> inputController = c->getInputController();
                if(inputController)
                {
                    inputController->setParentAircraft( pThis );
                    inputController->setAxisName( data.inputData.axisName );
                }

                m_controlSurfaces.push_back( c );
            }

            for(auto &propwashData : m_aircraftData.propWashData)
            {
                auto propwash = fb::make_ptr<CAircraftPropWash>();

                propwash->setName( propwashData.name );
                propwash->setParentAircraft( pThis );

                propwash->load( &propwashData );

                for(auto pu : m_propellerUnits)
                {
                    if(pu)
                    {
                        auto e = pu->getPowerUnit();
                        if(e->getName() == propwashData.propwashSource)
                        {
                            propwash->setPropellerUnit( pu );
                        }
                    }
                }

                propwash->setStrength( propwashData.strength );

                Array<bool> affectedSections;
                affectedSections.resize( propwashData.affectedSections.size() );

                for(size_t i = 0; i < affectedSections.size(); ++i)
                {
                    affectedSections[i] = propwashData.affectedSections[i] == 1;
                }

                propwash->setAffectedSections( affectedSections );
                propwash->setSectionMultipliers( propwashData.sectionMultipliers );

                m_propwashes.push_back( propwash );
            }

            for(size_t i = 0; i < m_aircraftData.wingData.size(); ++i)
            {
                auto &wingData = m_aircraftData.wingData[i];
                bool useSubDivide = false;

                Vector3<real_Num> subDivision( wingData.subDivision.x, wingData.subDivision.y,
                                               wingData.subDivision.z );
                if(subDivision.X() > 0 || subDivision.Y() > 0 || subDivision.Z() > 0)
                {
                    useSubDivide = true;
                }

                if(useSubDivide)
                {
                    Vector3<real_Num> localPosition( wingData.localTransform.position.x,
                                                     wingData.localTransform.position.y,
                                                     wingData.localTransform.position.z );
                    Vector3<real_Num> localScale( wingData.localTransform.scale.x,
                                                  wingData.localTransform.scale.y,
                                                  wingData.localTransform.scale.z );

                    if(subDivision.Z() > 0)
                    {
                        localScale.Z() = wingData.localTransform.scale.z / subDivision.Z();
                    }

                    Vector3<real_Num> segmentSize = localScale;

                    segmentSize.X() = 0;
                    segmentSize.Y() = 0;

                    if(subDivision.Z() > 0)
                    {
                        segmentSize.Z() = wingData.localTransform.scale.z / subDivision.Z();
                    }

                    Vector3<real_Num> startPos = (
                        localPosition - ( localScale * static_cast<real_Num>(0.5) * subDivision ) );
                    Vector3<real_Num> endPos = (
                        localPosition + ( localScale * static_cast<real_Num>(0.5) * subDivision ) );

                    for(s32 x = 0; x <= static_cast<s32>(subDivision.X()); ++x)
                    {
                        for(s32 y = 0; y <= static_cast<s32>(subDivision.Y()); ++y)
                        {
                            for(s32 z = 0; z <= static_cast<s32>(subDivision.Z()); ++z)
                            {
                                auto curWingData = wingData;

                                curWingData.name =
                                    curWingData.name + "_" + StringUtil::toString( x ) + "_" +
            StringUtil:: toString( y ) + "_" + StringUtil::toString( z );

                                Vector3<real_Num> wingLocalPosition =
                                    startPos + ( endPos - startPos ) * ( static_cast<real_Num>(z) /
                                                                         subDivision.Z() );

                                curWingData.localTransform.position = data::vec4(
                                    wingLocalPosition.X(), wingLocalPosition.Y(), wingLocalPosition.Z()
            ); curWingData.localTransform.scale = data::vec4( localScale.X(), localScale.Y(),
            localScale.Z() );

                                SmartPtr<CAircraftWingDefault> wing =
            fb::make_ptr<CAircraftWingDefault>();

                                wing->setName( curWingData.name );
                                wing->setParentAircraft( pThis );
                                wing->setParent( body );
                                wing->load( &curWingData );

                                for(auto &c : m_controlSurfaces)
                                {
                                    String controlSurfaceName = c->getName();
                                    String wingName = wing->getName();

                                    if(controlSurfaceName == wingName)
                                    {
                                        wing->setAttachedControlSurface( c );
                                        break;
                                    }
                                }

                                for(auto &p : m_propwashes)
                                {
                                    String controlSurfaceName = p->getName();
                                    String wingName = wing->getName();

                                    if(controlSurfaceName == wingName)
                                    {
                                        wing->setAttachedPropWash( p );
                                        break;
                                    }
                                }

                                m_wings.push_back( wing );
                            }
                        }
                    }
                }
                else
                {
                    SmartPtr<CAircraftWingDefault> wing = fb::make_ptr<CAircraftWingDefault>();

                    wing->setName( wingData.name );
                    wing->setParentAircraft( pThis );
                    wing->setParent( body );
                    wing->load( &wingData );

                    for(auto c : m_controlSurfaces)
                    {
                        String controlSurfaceName = c->getName();
                        String wingName = wing->getName();

                        if(controlSurfaceName == wingName)
                        {
                            wing->setAttachedControlSurface( c );
                            break;
                        }
                    }

                    for(auto &p : m_propwashes)
                    {
                        String controlSurfaceName = p->getName();
                        String wingName = wing->getName();

                        if(controlSurfaceName == wingName)
                        {
                            wing->setAttachedPropWash( p );
                            break;
                        }
                    }

                    m_wings.push_back( wing );
                }
            }

            for(auto c : m_controlSurfaces)
            {
                auto mainWing = c->getMainWing();
                if(mainWing)
                {
                    if(mainWing->getAttachedControlSurface())
                    {
                        auto wing = fb::make_ptr<CAircraftWingDefault>();
                        wing->setName( c->getName() + "_ControlSurface" );
                        wing->setParentAircraft( pThis );
                        wing->setParent( body );
                        wing->setControlSurface( true );
                        wing->setAttachedControlSurface( c );

                        for(auto &p : m_propwashes)
                        {
                            String propWashName = p->getName();
                            String wingName = mainWing->getName();

                            if(propWashName == wingName)
                            {
                                WP_ASSERT( mainWing->getName().find(p->getName()) != String::npos );
                                wing->setAttachedPropWash( p );
                                break;
                            }
                        }

                        c->setControlWing( wing );
                    }
                }
            }
            */
    }

    void CAircraft::loadProperties()
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

    void CAircraft::loadDefaults()
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

        //    SmartPtr<CAircraft> pThis = getSharedFromThis<CAircraft>();

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

        //    //SmartPtr<CAircraftWing> rWingOuter = boost::make_shared<CAircraftWing>();
        //    //rWingOuter->setParentAircraft(pThis);
        //    //rWingOuter->setParent(body);
        //    //rWingOuter->load(&rWingOuterData);
        //    //m_wings.push_back(rWingOuter);

        //    //SmartPtr<CAircraftWing> rWingOuterMirror = boost::make_shared<CAircraftWing>();
        //    //rWingOuterMirror->setParentAircraft(pThis);
        //    //rWingOuterMirror->setParent(body);
        //    //rWingOuterMirror->load(&rWingOuterDataMirror);
        //    //m_wings.push_back(rWingOuterMirror);

        //    //SmartPtr<CAircraftWing> rStabilator = boost::make_shared<CAircraftWing>();
        //    //rStabilator->setParentAircraft(pThis);
        //    //rStabilator->setParent(body);
        //    //rStabilator->load(&rStabilatorData);
        //    //m_wings.push_back(rStabilator);

        //    //SmartPtr<CAircraftWing> rStabilatorMirror = boost::make_shared<CAircraftWing>();
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

    void CAircraft::reloadFromData(void *pData)
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

    void CAircraft::unload(SmartPtr<ISharedObject> data)
    {
        try
        {
            setLoadingState(LoadingState::Unloading);

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

            for(auto w : m_wings)
            {
                w->unload(data);
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
            m_wings.clear();
            m_controlSurfaces.clear();
            m_propellers.clear();
            m_propellerUnits.clear();
            m_wheels.clear();

            m_properties = nullptr;
            m_powerChannels.clear();

            m_rigidbody = nullptr;

            m_callback = nullptr;
            setLoadingState(LoadingState::Unloaded);
        }
        catch(std::exception &e)
        {
            WP_LOG_EXCEPTION(e);
        }
    }

    void CAircraft::setBody(SmartPtr<IVehicleBody> body)
    {
        m_rigidbody = body;
    }

    void CAircraft::getAllTxData()
    {
        if(m_callback)
        {
            m_channels = m_callback->getInputData();
        }
    }

    real_Num CAircraft::getRollwiseDamping() const
    {
        return m_rollwiseDamping;
    }

    void CAircraft::setRollwiseDamping(real_Num rollwiseDamping)
    {
        m_rollwiseDamping = rollwiseDamping;
    }

    bool CAircraft::isUserControlled() const
    {
        return true;
    }

    void CAircraft::setUserControlled(bool userControlled)
    {
    }

    real_Num CAircraft::getMass() const
    {
        auto body = getBodyPtr();
        return body->getMass();
    }

    void CAircraft::setMass(real_Num mass)
    {
        auto body = getBodyPtr();
        body->setMass(mass);
    }

    Transform3<real_Num> CAircraft::getWorldTransform() const
    {
        return m_worldTransform;
    }

    void CAircraft::updateTransform()
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

        if(auto body = getBody())
        {
            auto cg = m_bodyTransform.transformPoint(m_cg);
            body->setWorldCenterOfMass(cg);
        }

        for(auto &e : m_engines)
        {
            e->updateTransform();
        }

        for(auto &w : m_wings)
        {
            w->updateTransform();
        }

        for(auto &c : m_controlSurfaces)
        {
            c->updateTransform();
        }

        for(auto &w : m_wheels)
        {
            w->updateTransform();
        }
    }

    SmartPtr<Properties> CAircraft::getMirror(SmartPtr<Properties> wingData)
    {
        //    data::aircraft_wing_data r = wingData;
        //    r.isMirror = true;

        //    if(r.isMirror)
        //    {
        //        r.localTransform.position.x = -wingData.localTransform.position.x;
        //    }

        //    return r;
        return nullptr;
    }

    void CAircraft::setWorldTransform(const Transform3<real_Num> &worldTransform)
    {
        m_worldTransform = worldTransform;
    }

    Transform3<real_Num> CAircraft::getLocalTransform() const
    {
        return m_localTransform;
    }

    void CAircraft::setLocalTransform(const Transform3<real_Num> &localTransform)
    {
        m_localTransform = localTransform;
    }

    SmartPtr<IAircraftCallback> CAircraft::getCallback() const
    {
        return m_callback;
    }

    void CAircraft::setCallback(SmartPtr<IAircraftCallback> callback)
    {
        m_callback = callback;
    }

    Transform3<real_Num> CAircraft::getBodyTransform() const
    {
        return m_bodyTransform;
    }

    void CAircraft::setBodyTransform(Transform3<real_Num> bodyTransform)
    {
        m_bodyTransform = bodyTransform;
    }

    void CAircraft::setControlAngle(int id, float angle)
    {
    }

    real_Num CAircraft::getAirDensity() const
    {
        return m_airDensity;
    }

    void CAircraft::setAirDensity(real_Num airDensity)
    {
        m_airDensity = airDensity;
    }

    const Array<SmartPtr<Properties>> &CAircraft::getPowerChannels() const
    {
        return m_powerChannels;
    }

    Array<SmartPtr<Properties>> &CAircraft::getPowerChannels()
    {
        return m_powerChannels;
    }

    void CAircraft::setPowerChannels(Array<SmartPtr<Properties>> powerChannels)
    {
        m_powerChannels = powerChannels;
    }

    bool CAircraft::getEmulateBattery() const
    {
        return m_emulateBattery;
    }

    void CAircraft::setEmulateBattery(bool emulateBattery)
    {
        m_emulateBattery = emulateBattery;
    }

    bool CAircraft::isElectric() const
    {
        return m_bIsElectric;
    }

    void CAircraft::setElectric(bool bIsElectric)
    {
        m_bIsElectric = bIsElectric;
    }

    SmartPtr<IBatteryPack> CAircraft::getBatteryPack() const
    {
        return m_batteryPack;
    }

    void CAircraft::setBatteryPack(SmartPtr<IBatteryPack> batteryPack)
    {
        m_batteryPack = batteryPack;
    }

    bool CAircraft::getEnablePowerUnit() const
    {
        return m_enablePowerUnit;
    }

    void CAircraft::setEnablePowerUnit(bool enablePowerUnit)
    {
        m_enablePowerUnit = enablePowerUnit;
    }

    SmartPtr<IAerodymanicsWind> CAircraft::getWind() const
    {
        return m_wind;
    }

    void CAircraft::setWind(SmartPtr<IAerodymanicsWind> wind)
    {
        m_wind = wind;
    }

    void CAircraft::addPropellerUnit(SmartPtr<IAircraftPropellerUnit> propellerUnit)
    {
        m_propellerUnits.push_back(propellerUnit);
    }

    void CAircraft::removePropellerUnit(SmartPtr<IAircraftPropellerUnit> propellerUnit)
    {
        auto it = std::find(m_propellerUnits.begin(), m_propellerUnits.end(), propellerUnit);
        if(it != m_propellerUnits.end())
        {
            m_propellerUnits.erase(it);
        }
    }

    Array<SmartPtr<IAircraftPropellerUnit>> CAircraft::getPropellerUnits() const
    {
        return m_propellerUnits;
    }

    void CAircraft::setPropellerUnits(
        const Array<SmartPtr<IAircraftPropellerUnit>> &propellerUnits)
    {
        m_propellerUnits = propellerUnits;
    }

    void CAircraft::addWheel(SmartPtr<IWheelComponent> wheel)
    {
        SmartPtr<IVehicle> pThis = getSharedFromThis<IVehicle>();

        wheel->setOwner(pThis);
        m_wheels.push_back(wheel);
    }

    void CAircraft::removeWheel(SmartPtr<IWheelComponent> wheel)
    {
        auto it = std::find(m_wheels.begin(), m_wheels.end(), wheel);
        if(it != m_wheels.end())
        {
            m_wheels.erase(it);
        }
    }

    Array<SmartPtr<IWheelComponent>> CAircraft::getWheels() const
    {
        return m_wheels;
    }

    void CAircraft::setWheels(const Array<SmartPtr<IWheelComponent>> &wheels)
    {
        m_wheels = wheels;
    }

    real_Num CAircraft::getSectionMultiplier() const
    {
        return m_sectionMultiplier;
    }

    void CAircraft::setSectionMultiplier(real_Num sectionMultiplier)
    {
        m_sectionMultiplier = sectionMultiplier;
    }

    String CAircraft::getModelDataFilePath() const
    {
        return m_modelDataFilePath;
    }

    void CAircraft::setModelDataFilePath(const String &modelDataFilePath)
    {
        m_modelDataFilePath = modelDataFilePath;
    }

    void CAircraft::reset()
    {
        for(auto w : m_wings)
        {
            w->reset();
        }

        for(auto c : m_controlSurfaces)
        {
            c->reset();
        }

        for(auto p : m_propellers)
        {
            p->reset();
        }
    }

    real_Num CAircraft::getThrust(int idx) const
    {
        if(idx < m_propellerUnits.size())
        {
            auto thrust = m_propellerUnits[idx]->getThrust();
            return thrust.length();
        }

        return 0.0;
    }
}
