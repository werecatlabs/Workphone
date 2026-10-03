#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CAircraftControlSurface.hpp>
#include <WPVehiclePhysics/InputController.hpp>
#include <WPVehiclePhysics/CAircraft.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace vehicle
    {
        CAircraftControlSurface::CAircraftControlSurface()
        {
            m_reverse = false;
            m_affectedSections.resize( 20 );
            m_controllable = true;
            m_inputController = workphone::make_ptr<InputController>();

            m_maxDeflectionDegrees = 30.0f;
            setRootHingeDistanceFromTrailingEdge( 0.25f );
            setTipHingeDistanceFromTrailingEdge( 0.25f );

            setRotationAxis( Vector3<real_Num>::left() );

            m_initialModelRotation = Quaternion<real_Num>::identity();
            setCurrentDeflection( 0.0f );

            m_surfaceId = 0;

            for( size_t i = 0; i < m_affectedSections.size(); ++i )
            {
                m_affectedSections[i] = true;
            }
        }

        CAircraftControlSurface::~CAircraftControlSurface()
        {
        }

        void CAircraftControlSurface::load( SmartPtr<ISharedObject> data )
        {
        }

        void CAircraftControlSurface::load( void *pData )
        {
            // WP_LOG( "Control Surface load" );

            // auto data = static_cast<data::aircraft_control_surface_data *>(pData);

            // m_minDeflectionDegrees = data->minDeflectionDegrees;
            // m_maxDeflectionDegrees = data->maxDeflectionDegrees;

            // m_aoaMultiplier = data->aoaMultiplier;

            // m_clMultiplier = data->clMultiplier;
            // m_cdMultiplier = data->cdMultiplier;
            // m_cmMultiplier = data->cmMultiplier;

            // m_stallControlCL = data->stallControlCL;
            // m_stallControlCD = data->stallControlCD;
            // m_stallControlCM = data->stallControlCM;
            // m_stallThreshold = data->stallThreshold;

            // m_clLookup = fb::make_ptr<LinearSpline1<real_Num>>();
            // m_cdLookup = fb::make_ptr<LinearSpline1<real_Num>>();
            // m_cmLookup = fb::make_ptr<LinearSpline1<real_Num>>();

            // Deque<std::pair<real_Num, real_Num>> clPoints;
            // Deque<std::pair<real_Num, real_Num>> cdPoints;
            // Deque<std::pair<real_Num, real_Num>> cmPoints;

            // real_Num angle = -90.0;
            // for(auto &clPoint : data->clLookup)
            //{
            //     clPoints.emplace_back( std::pair<real_Num, real_Num>( angle, value ) );
            //     angle = angle + 10.0;
            // }

            // angle = -90.0;
            // for(auto &cdPoint : data->cdLookup)
            //{
            //     cdPoints.emplace_back( std::pair<real_Num, real_Num>( angle, value ) );
            //     angle = angle + 10.0;
            // }

            // angle = -90.0;
            // for(auto &cmPoint : data->cmLookup)
            //{
            //     cmPoints.emplace_back( std::pair<real_Num, real_Num>( angle, value ) );
            //     angle = angle + 10.0;
            // }

            // m_clLookup->setPoints(
            //     Array<std::pair<real_Num, real_Num>>( clPoints.begin(), clPoints.end() ) );
            // m_cdLookup->setPoints(
            //     Array<std::pair<real_Num, real_Num>>( cdPoints.begin(), cdPoints.end() ) );
            // m_cmLookup->setPoints(
            //     Array<std::pair<real_Num, real_Num>>( cmPoints.begin(), cmPoints.end() ) );

            // WP_LOG( "Control Surface loaded" );
        }

        void CAircraftControlSurface::unload( SmartPtr<ISharedObject> data )
        {
            if( m_mainWing )
            {
                m_mainWing->unload( data );
            }

            if( m_controlWing )
            {
                m_controlWing->unload( data );
            }

            m_mainWing = nullptr;
            m_controlWing = nullptr;
        }

        void CAircraftControlSurface::update( const double &t, const double &dt )
        {
            // if (m_controlWing)
            //{
            //	m_controlWing->update(t, dt);
            // }
        }

        void CAircraftControlSurface::updateTransform()
        {
            if( m_controlWing )
            {
                CAircraftAttachment::updateTransform();
                m_controlWing->updateTransform();
                // const real_Num normalizedAngle = getCurrentDeflection();
                // auto fCurrentDeflection = normalizedAngle > real_Num(0.0) ?
                //	Math<real_Num>::Abs(normalizedAngle) * m_maxDeflectionDegrees :
                //	Math<real_Num>::Abs(normalizedAngle) * m_minDeflectionDegrees;
                // fCurrentDeflection = Math<real_Num>::DegToRad(fCurrentDeflection);

                // auto q = Quaternion<real_Num>::angleAxis(fCurrentDeflection,
                // Vector3<real_Num>::right()); m_localTransform->setOrientation(q);

                // SmartPtr<Transform3<real_Num>> parentTransform =
                // getParentAircraft()->getWorldTransform();

                // WP_ASSERT(parentTransform->getScale().length() > std::numeric_limits<f32>::epsilon());
                // WP_ASSERT(getLocalTransform()->getScale().length() >
                // std::numeric_limits<f32>::epsilon());

                // getWorldTransform()->transformFromParent(parentTransform, getLocalTransform());
                // WP_ASSERT(getWorldTransform()->getScale().length() >
                // std::numeric_limits<f32>::epsilon());
            }
        }

        const Array<bool> &CAircraftControlSurface::getAffectedSections() const
        {
            return m_affectedSections;
        }

        Array<bool> &CAircraftControlSurface::getAffectedSections()
        {
            return m_affectedSections;
        }

        void CAircraftControlSurface::setAffectedSections( const Array<bool> &affectedSections )
        {
            m_affectedSections = affectedSections;
        }

        void CAircraftControlSurface::modifyWingGeometry( s32 SectionIndex, Vector3<real_Num> &pointA,
                                                          Vector3<real_Num> &pointB,
                                                          Vector3<real_Num> &pointC,
                                                          Vector3<real_Num> &pointD )
        {
#if defined _DEBUG
            if( getName().find( "WING" ) != std::string::npos )
            {
                int stop = 0;
                stop = 0;
            }
#endif

            auto aircraft = getParentAircraft();
            auto index = SectionIndex / aircraft->getSectionMultiplier();

            if( index < static_cast<s32>( m_affectedSections.size() ) )
            {
                if( m_affectedSections[index] == true )
                {
                    // return;
                    // R A-----------------B (Leading edge)
                    // O |                 |
                    // O |                 |
                    // T D-----------------C (Trailing edge

                    // First step is to work out the aileron position and offset on the wing.
                    const real_Num rootHingeDistanceFromTrailingEdge =
                        getRootHingeDistanceFromTrailingEdge();
                    const real_Num tipHingeDistanceFromTrailingEdge =
                        getTipHingeDistanceFromTrailingEdge();

                    const Vector3<real_Num> wingRootAileronHingePos = pointA;
                    // pointD + ((pointA - pointD) * rootHingeDistanceFromTrailingEdge);
                    const Vector3<real_Num> wingTipAileronHingePos = pointB;
                    // pointC + ((pointB - pointC) * tipHingeDistanceFromTrailingEdge);

                    const Vector3<real_Num> aileronHinge =
                        ( wingTipAileronHingePos - wingRootAileronHingePos ).normaliseCopy();

                    Vector3<real_Num> rootAileronAngle = pointD - wingRootAileronHingePos;
                    Vector3<real_Num> tipAileronAngle = pointC - wingTipAileronHingePos;

                    auto rootLength = rootAileronAngle.length();
                    auto tipLength = tipAileronAngle.length();

                    const real_Num normalizedAngle = getCurrentDeflection();
                    auto           fCurrentDeflection =
                        normalizedAngle > static_cast<real_Num>( 0.0 )
                            ? Math<real_Num>::Abs( normalizedAngle ) * m_maxDeflectionDegrees
                            : Math<real_Num>::Abs( normalizedAngle ) * m_minDeflectionDegrees;
                    fCurrentDeflection = Math<real_Num>::DegToRad( fCurrentDeflection ) * 2.0;

                    // Deflect ailerons.
                    if( !isReversed() )
                    {
                        auto hingeRotation =
                            Quaternion<real_Num>::angleAxis( fCurrentDeflection, aileronHinge );
                        rootAileronAngle = hingeRotation * rootAileronAngle.normaliseCopy();
                        tipAileronAngle = hingeRotation * tipAileronAngle.normaliseCopy();
                    }
                    else
                    {
                        auto hingeRotation =
                            Quaternion<real_Num>::angleAxis( -fCurrentDeflection, aileronHinge );
                        rootAileronAngle = hingeRotation * rootAileronAngle.normaliseCopy();
                        tipAileronAngle = hingeRotation * tipAileronAngle.normaliseCopy();
                    }

                    // Once we know the deflection of the aileron and where are new trailing edge is, we
                    // can use this to tweak the wing chord line.
                    pointD = wingRootAileronHingePos + ( rootAileronAngle.normaliseCopy() * rootLength );
                    pointC = wingTipAileronHingePos + ( tipAileronAngle.normaliseCopy() * tipLength );
                }
            }
        }

        void CAircraftControlSurface::modifyWingGeometry( s32 SectionIndex, Vector3<real_Num> &pointA,
                                                          Vector3<real_Num> &pointB,
                                                          Vector3<real_Num> &pointC,
                                                          Vector3<real_Num> &pointD, real_Num t )
        {
#if defined _DEBUG
            if( getName().find( "WING" ) != std::string::npos )
            {
                int stop = 0;
                stop = 0;
            }
#endif

            auto aircraft = getParentAircraft();
            auto index = SectionIndex / aircraft->getSectionMultiplier();

            if( index < static_cast<s32>( m_affectedSections.size() ) )
            {
                if( m_affectedSections[index] == true )
                {
                    // return;
                    // R A-----------------B (Leading edge)
                    // O |                 |
                    // O |                 |
                    // T D-----------------C (Trailing edge

                    // First step is to work out the aileron position and offset on the wing.
                    const real_Num rootHingeDistanceFromTrailingEdge =
                        getRootHingeDistanceFromTrailingEdge();
                    const real_Num tipHingeDistanceFromTrailingEdge =
                        getTipHingeDistanceFromTrailingEdge();

                    const Vector3<real_Num> wingRootAileronHingePos = pointA;
                    // pointD + ((pointA - pointD) * rootHingeDistanceFromTrailingEdge);
                    const Vector3<real_Num> wingTipAileronHingePos = pointB;
                    // pointC + ((pointB - pointC) * tipHingeDistanceFromTrailingEdge);

                    const Vector3<real_Num> aileronHinge =
                        ( wingTipAileronHingePos - wingRootAileronHingePos ).normaliseCopy();

                    Vector3<real_Num> rootAileronAngle = pointD - wingRootAileronHingePos;
                    Vector3<real_Num> tipAileronAngle = pointC - wingTipAileronHingePos;

                    auto rootLength = rootAileronAngle.length();
                    auto tipLength = tipAileronAngle.length();

                    const real_Num normalizedAngle = ( ( getCurrentDeflection() + t ) + 1.0 ) / 2.0;
                    // auto fCurrentDeflection = normalizedAngle > real_Num(0.0) ?
                    //	(Math<real_Num>::Abs(normalizedAngle) + t) * (m_maxDeflectionDegrees) :
                    //	(Math<real_Num>::Abs(normalizedAngle) + t) * (m_minDeflectionDegrees);
                    auto fCurrentDeflection =
                        m_minDeflectionDegrees +
                        ( m_maxDeflectionDegrees - m_minDeflectionDegrees ) * normalizedAngle;
                    fCurrentDeflection = Math<real_Num>::DegToRad( fCurrentDeflection ) * 2.0;

                    // Deflect ailerons.
                    if( !isReversed() )
                    {
                        const Quaternion<real_Num> hingeRotation =
                            Quaternion<real_Num>::angleAxis( fCurrentDeflection, aileronHinge );
                        rootAileronAngle = hingeRotation * rootAileronAngle;
                        tipAileronAngle = hingeRotation * tipAileronAngle;
                    }
                    else
                    {
                        const Quaternion<real_Num> hingeRotation =
                            Quaternion<real_Num>::angleAxis( -fCurrentDeflection, aileronHinge );
                        rootAileronAngle = hingeRotation * rootAileronAngle;
                        tipAileronAngle = hingeRotation * tipAileronAngle;
                    }

                    // Once we know the deflection of the aileron and where are new trailing edge is, we
                    // can use this to tweak the wing chord line.
                    pointD = wingRootAileronHingePos + ( rootAileronAngle.normaliseCopy() * rootLength );
                    pointC = wingTipAileronHingePos + ( tipAileronAngle.normaliseCopy() * tipLength );
                }
            }
        }

        void CAircraftControlSurface::modifyMainWingGeometry( s32                SectionIndex,
                                                              Vector3<real_Num> &pointA,
                                                              Vector3<real_Num> &pointB,
                                                              Vector3<real_Num> &pointC,
                                                              Vector3<real_Num> &pointD, real_Num t )
        {
#if defined _DEBUG
            if( getName().find( "WING" ) != std::string::npos )
            {
                int stop = 0;
                stop = 0;
            }
#endif

            auto aircraft = getParentAircraft();
            auto index = SectionIndex / aircraft->getSectionMultiplier();

            if( index < static_cast<s32>( m_affectedSections.size() ) )
            {
                if( m_affectedSections[index] == true )
                {
                    // return;
                    // R A-----------------B (Leading edge)
                    // O |                 |
                    // O |                 |
                    // T D-----------------C (Trailing edge

                    // First step is to work out the aileron position and offset on the wing.
                    const real_Num rootHingeDistanceFromTrailingEdge =
                        getRootHingeDistanceFromTrailingEdge();
                    const real_Num tipHingeDistanceFromTrailingEdge =
                        getTipHingeDistanceFromTrailingEdge();

                    const Vector3<real_Num> wingRootAileronHingePos = pointA;
                    // pointD + ((pointA - pointD) * rootHingeDistanceFromTrailingEdge);
                    const Vector3<real_Num> wingTipAileronHingePos = pointB;
                    // pointC + ((pointB - pointC) * tipHingeDistanceFromTrailingEdge);

                    const Vector3<real_Num> aileronHinge =
                        ( wingTipAileronHingePos - wingRootAileronHingePos ).normaliseCopy();

                    Vector3<real_Num> rootAileronAngle = pointD - wingRootAileronHingePos;
                    Vector3<real_Num> tipAileronAngle = pointC - wingTipAileronHingePos;

                    auto rootLength = rootAileronAngle.length();
                    auto tipLength = tipAileronAngle.length();

                    const real_Num normalizedAngle = getCurrentDeflection();
                    auto fCurrentDeflection = t > static_cast<real_Num>( 0.0 )
                                                ? Math<real_Num>::Abs( t ) * m_maxDeflectionDegrees
                                                : Math<real_Num>::Abs( t ) * m_minDeflectionDegrees;
                    fCurrentDeflection = Math<real_Num>::DegToRad( fCurrentDeflection ) * 2.0;

                    // Deflect ailerons.
                    if( !isReversed() )
                    {
                        const Quaternion<real_Num> hingeRotation =
                            Quaternion<real_Num>::angleAxis( fCurrentDeflection, aileronHinge );
                        rootAileronAngle = hingeRotation * rootAileronAngle;
                        tipAileronAngle = hingeRotation * tipAileronAngle;
                    }
                    else
                    {
                        const Quaternion<real_Num> hingeRotation =
                            Quaternion<real_Num>::angleAxis( -fCurrentDeflection, aileronHinge );
                        rootAileronAngle = hingeRotation * rootAileronAngle;
                        tipAileronAngle = hingeRotation * tipAileronAngle;
                    }

                    // Once we know the deflection of the aileron and where are new trailing edge is, we
                    // can use this to tweak the wing chord line.
                    pointD = wingRootAileronHingePos + ( rootAileronAngle.normaliseCopy() * rootLength );
                    pointC = wingTipAileronHingePos + ( tipAileronAngle.normaliseCopy() * tipLength );
                }
            }
        }

        s32 CAircraftControlSurface::getSurfaceId() const
        {
            return m_surfaceId;
        }

        void CAircraftControlSurface::setSurfaceId( s32 surfaceId )
        {
            m_surfaceId = surfaceId;
        }

        SmartPtr<InputController> CAircraftControlSurface::getInputController() const
        {
            return m_inputController;
        }

        void CAircraftControlSurface::setInputController( SmartPtr<InputController> inputController )
        {
            m_inputController = inputController;
        }

        real_Num CAircraftControlSurface::getCurrentDeflection() const
        {
            return m_currentDeflection;
        }

        void CAircraftControlSurface::setCurrentDeflection( real_Num deflection )
        {
            m_currentDeflection = deflection;
        }

        bool CAircraftControlSurface::isReversed() const
        {
            return m_reverse;
        }

        void CAircraftControlSurface::setReversed( bool reversed )
        {
            m_reverse = reversed;
        }

        real_Num CAircraftControlSurface::getTipHingeDistanceFromTrailingEdge() const
        {
            return m_tipHingeDistanceFromTrailingEdge;
        }

        void CAircraftControlSurface::setTipHingeDistanceFromTrailingEdge( real_Num tipHingeDistance )
        {
            m_tipHingeDistanceFromTrailingEdge = tipHingeDistance;
        }

        real_Num CAircraftControlSurface::getRootHingeDistanceFromTrailingEdge() const
        {
            return m_rootHingeDistanceFromTrailingEdge;
        }

        void CAircraftControlSurface::setRootHingeDistanceFromTrailingEdge( real_Num rootHingeDistance )
        {
            m_rootHingeDistanceFromTrailingEdge = rootHingeDistance;
        }

        Vector3<real_Num> CAircraftControlSurface::getRotationAxis() const
        {
            return m_rotationAxis;
        }

        void CAircraftControlSurface::setRotationAxis( Vector3<real_Num> rotationAxis )
        {
            m_rotationAxis = rotationAxis;
        }

        bool CAircraftControlSurface::isAffectedSection( int index ) const
        {
            return m_affectedSections[index];
        }

        SmartPtr<LinearSpline1<real_Num>> CAircraftControlSurface::getClLookup() const
        {
            return m_clLookup;
        }

        void CAircraftControlSurface::setClLookup( SmartPtr<LinearSpline1<real_Num>> linearSpline1 )
        {
            m_clLookup = linearSpline1;
        }

        SmartPtr<LinearSpline1<real_Num>> CAircraftControlSurface::getCdLookup() const
        {
            return m_cdLookup;
        }

        void CAircraftControlSurface::setCdLookup( SmartPtr<LinearSpline1<real_Num>> linearSpline1 )
        {
            m_cdLookup = linearSpline1;
        }

        SmartPtr<LinearSpline1<real_Num>> CAircraftControlSurface::getCmLookup() const
        {
            return m_cmLookup;
        }

        void CAircraftControlSurface::setCmLookup( SmartPtr<LinearSpline1<real_Num>> linearSpline1 )
        {
            m_cmLookup = linearSpline1;
        }

        real_Num CAircraftControlSurface::getClMultiplier() const
        {
            return m_clMultiplier;
        }

        void CAircraftControlSurface::setClMultiplier( real_Num clMultiplier )
        {
            m_clMultiplier = clMultiplier;
        }

        real_Num CAircraftControlSurface::getCdMultiplier() const
        {
            return m_cdMultiplier;
        }

        void CAircraftControlSurface::setCdMultiplier( real_Num cdMultiplier )
        {
            m_cdMultiplier = cdMultiplier;
        }

        real_Num CAircraftControlSurface::getCmMultiplier() const
        {
            return m_cmMultiplier;
        }

        void CAircraftControlSurface::setCmMultiplier( real_Num cmMultiplier )
        {
            m_cmMultiplier = cmMultiplier;
        }

        real_Num CAircraftControlSurface::getMinDeflectionDegrees() const
        {
            return m_minDeflectionDegrees;
        }

        void CAircraftControlSurface::setMinDeflectionDegrees( real_Num minDeflectionDegrees )
        {
            m_minDeflectionDegrees = minDeflectionDegrees;
        }

        real_Num CAircraftControlSurface::getMaxDeflectionDegrees() const
        {
            return m_maxDeflectionDegrees;
        }

        void CAircraftControlSurface::setMaxDeflectionDegrees( real_Num maxDeflectionDegrees )
        {
            m_maxDeflectionDegrees = maxDeflectionDegrees;
        }

        real_Num CAircraftControlSurface::getAoAMultiplier() const
        {
            return m_aoaMultiplier;
        }

        void CAircraftControlSurface::setAoAMultiplier( real_Num aoaMultiplier )
        {
            m_aoaMultiplier = aoaMultiplier;
        }

        SmartPtr<IAircraftWing> CAircraftControlSurface::getMainWing() const
        {
            return m_mainWing;
        }

        void CAircraftControlSurface::setMainWing( SmartPtr<IAircraftWing> mainWing )
        {
            m_mainWing = mainWing;
        }

        SmartPtr<IAircraftWing> CAircraftControlSurface::getControlWing() const
        {
            return m_controlWing;
        }

        void CAircraftControlSurface::setControlWing( SmartPtr<IAircraftWing> controlWing )
        {
            m_controlWing = controlWing;

            if( m_controlWing )
            {
                if( m_mainWing )
                {
                    void *pData = m_mainWing->getData();
                    if( pData )
                    {
                        // m_controlWing->load(pData);
                    }
                }
            }
        }

        bool CAircraftControlSurface::useControlWing() const
        {
            return m_useControlWing;
        }

        void CAircraftControlSurface::setUseControlWing( bool useControlWing )
        {
            m_useControlWing = useControlWing;
        }

        void CAircraftControlSurface::updateGeometry()
        {
            if( m_controlWing )
            {
                m_controlWing->updateGeometry();
            }
        }

        real_Num CAircraftControlSurface::getStallControlCL() const
        {
            return m_stallControlCL;
        }

        void CAircraftControlSurface::setStallControlCL( real_Num stallControlCL )
        {
            m_stallControlCL = stallControlCL;
        }

        real_Num CAircraftControlSurface::getStallControlCD() const
        {
            return m_stallControlCD;
        }

        void CAircraftControlSurface::setStallControlCD( real_Num stallControlCD )
        {
            m_stallControlCD = stallControlCD;
        }

        real_Num CAircraftControlSurface::getStallControlCM() const
        {
            return m_stallControlCM;
        }

        void CAircraftControlSurface::setStallControlCM( real_Num stallControlCM )
        {
            m_stallControlCM = stallControlCM;
        }

        real_Num CAircraftControlSurface::getStallThreshold() const
        {
            return m_stallThreshold;
        }

        void CAircraftControlSurface::setStallThreshold( real_Num stallThreshold )
        {
            m_stallThreshold = stallThreshold;
        }

        void CAircraftControlSurface::reset()
        {
            if( m_controlWing )
            {
                m_controlWing->reset();
            }
        }
    } // namespace vehicle
} // namespace workphone
