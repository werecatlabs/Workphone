#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CAircraftWingFast.hpp>
#include <WPVehiclePhysics/CAircraftBody.hpp>
#include <WPVehiclePhysics/CAircraft.hpp>
#include <WPVehiclePhysics/CAerofoil.hpp>
#include <WPVehiclePhysics/GroundEffect.hpp>
#include <WPVehiclePhysics/CAircraftPropWash.hpp>
#include <WPVehiclePhysics/EngineSimple.hpp>
#include <WPVehiclePhysics/CAircraftControlSurface.hpp>
#include <Workphone/Workphone.hpp>
#include <iostream>

namespace workphone
{
    namespace vehicle
    {
        u32 CAircraftWingFast::m_idExt = 0;

        CAircraftWingFast::CAircraftWingFast()
        {
            m_sectionCount = 10;
            m_wingTipWidthZeroToOne = static_cast<real_Num>( 1.0 );
            m_wingTipSweep = static_cast<real_Num>( 0.0 );
            m_wingTipAngle = static_cast<real_Num>( 0.0 );
            m_cdOverride = static_cast<real_Num>( 0.045 );

            m_wingArea = static_cast<real_Num>( 0.0 );

            m_rootLeadingEdge = Vector3<real_Num>::zero();
            m_rootTrailingEdge = Vector3<real_Num>::zero();
            m_tipLeadingEdge = Vector3<real_Num>::zero();
            m_tipTrailingEdge = Vector3<real_Num>::zero();
            m_rootLiftPosition = Vector3<real_Num>::zero();
            m_tipLiftPosition = Vector3<real_Num>::zero();
            m_liftLineChordPosition = static_cast<real_Num>( 0.5 );

            m_aoa = std::array<real_Num, MaxSections>( { 0 } );
            m_aoaPlus10 = std::array<real_Num, MaxSections>( { 0 } );
            m_aoaMinus10 = std::array<real_Num, MaxSections>( { 0 } );

            m_chordLengthsMinus10 = std::array<real_Num, MaxSections>( { 0 } );
            m_chordLengthsPlus10 = std::array<real_Num, MaxSections>( { 0 } );
            m_chordLengths = std::array<real_Num, MaxSections>( { 0 } );

            m_re = std::array<real_Num, MaxSections>( { 0 } );
            m_area = std::array<real_Num, MaxSections>( { 0 } );

            m_totalLift = std::array<real_Num, MaxSections>( { 0 } );
            m_totalDrag = std::array<real_Num, MaxSections>( { 0 } );
            m_totalPitch = std::array<real_Num, MaxSections>( { 0 } );

            m_fStallControlCL = std::array<real_Num, MaxSections>( { 0 } );
            m_fStallControlCD = std::array<real_Num, MaxSections>( { 0 } );
            m_fStallControlCM = std::array<real_Num, MaxSections>( { 0 } );
        }

        CAircraftWingFast::~CAircraftWingFast()
        {
        }

        void CAircraftWingFast::updateGeometry()
        {
#if 1
            auto transform = getLocalTransform();

            Vector3<real_Num>    p = transform.getPosition();
            Quaternion<real_Num> r = transform.getOrientation();

            Vector3<real_Num> scale = transform.getScale();
            Vector3<real_Num> right = transform.right();
            Vector3<real_Num> forward = transform.forward();

            forward = -forward; // todo

            // if (scale.X() < real_Num(0.0))
            //{
            //	right = -right;
            // }

            WP_ASSERT( scale.length() > std::numeric_limits<f32>::epsilon() );

            // Calculate root and tip center points.
            Vector3<real_Num> wingRootCenter =
                p - ( right * ( scale.X() * static_cast<real_Num>( 0.5 ) ) );
            Vector3<real_Num> wingTipCenter =
                p + ( right * ( scale.X() * static_cast<real_Num>( 0.5 ) ) );
            wingTipCenter += forward * m_wingTipSweep;

            // Calculate corners.
            m_rootLeadingEdge =
                wingRootCenter + ( forward * ( scale.Z() * static_cast<real_Num>( 0.5 ) ) );
            m_rootTrailingEdge =
                wingRootCenter - ( forward * ( scale.Z() * static_cast<real_Num>( 0.5 ) ) );
            m_tipLeadingEdge =
                wingTipCenter +
                ( forward * ( ( scale.Z() * static_cast<real_Num>( 0.5 ) ) * m_wingTipWidthZeroToOne ) );
            m_tipTrailingEdge =
                wingTipCenter -
                ( forward * ( ( scale.Z() * static_cast<real_Num>( 0.5 ) ) * m_wingTipWidthZeroToOne ) );

            // Tweak tip corners based on the angle between them.
            Vector3<real_Num>    tipTrailingEdgeToTipLeadingEdge = m_tipLeadingEdge - m_tipTrailingEdge;
            Quaternion<real_Num> rotation =
                Quaternion<real_Num>::angleAxis( m_wingTipAngle, r * Vector3<real_Num>::UNIT_X );
            tipTrailingEdgeToTipLeadingEdge = rotation * tipTrailingEdgeToTipLeadingEdge;
            m_tipTrailingEdge =
                wingTipCenter - ( tipTrailingEdgeToTipLeadingEdge * static_cast<real_Num>( 0.5 ) );
            m_tipLeadingEdge =
                wingTipCenter + ( tipTrailingEdgeToTipLeadingEdge * static_cast<real_Num>( 0.5 ) );

            auto rootDistance = static_cast<real_Num>( 0.0 );
            auto tipDistance = static_cast<real_Num>( 0.0 );

            auto attachedControlSurface = getAttachedControlSurface();
            if( attachedControlSurface )
            {
                if( !isControlSurface() )
                {
                    rootDistance = attachedControlSurface->getRootHingeDistanceFromTrailingEdge();
                    tipDistance = attachedControlSurface->getTipHingeDistanceFromTrailingEdge();

                    // m_rootTrailingEdge = m_rootTrailingEdge + (m_rootLeadingEdge - m_rootTrailingEdge)
                    // * rootDistance; m_tipTrailingEdge = m_tipTrailingEdge + (m_tipLeadingEdge -
                    // m_tipTrailingEdge) * tipDistance;
                }
                else
                {
                    rootDistance = attachedControlSurface->getRootHingeDistanceFromTrailingEdge();
                    tipDistance = attachedControlSurface->getTipHingeDistanceFromTrailingEdge();

                    auto wing = attachedControlSurface->getMainWing();
                    auto mainWing = workphone::static_pointer_cast<CAircraftWingFast>( wing );
                    if( mainWing )
                    {
                        m_rootLeadingEdge =
                            mainWing->m_rootTrailingEdge +
                            ( mainWing->m_rootLeadingEdge - mainWing->m_rootTrailingEdge ) *
                                rootDistance;
                        m_tipLeadingEdge =
                            mainWing->m_tipTrailingEdge +
                            ( mainWing->m_tipLeadingEdge - mainWing->m_tipTrailingEdge ) * tipDistance;
                    }
                }
            }

            m_rootLiftPosition = m_rootTrailingEdge + ( ( m_rootLeadingEdge - m_rootTrailingEdge ) *
                                                        m_liftLineChordPosition );
            m_tipLiftPosition = m_tipTrailingEdge +
                                ( ( m_tipLeadingEdge - m_tipTrailingEdge ) * m_liftLineChordPosition );

            // m_localWingRight = (wingRootCenter - wingTipCenter).normaliseCopy();
            m_localWingRight = ( wingTipCenter - wingRootCenter ).normaliseCopy();
            // m_localWingRight = transform->transformVector(m_localWingRight);
            m_localWingRight.normalise();

            if( scale.X() < static_cast<real_Num>( 0.0 ) )
            {
                m_localWingRight = -m_localWingRight;
            }

            // Calculate wing area.
            m_wingArea = calculateArea( m_rootLeadingEdge, m_tipLeadingEdge, m_tipTrailingEdge,
                                        m_rootTrailingEdge );
            WP_ASSERT( m_wingArea < static_cast<real_Num>( 10.0 ) );
#else
            SmartPtr<Transform3<real_Num>> bodyTransform = getParentAircraft()->getBodyTransform();
            SmartPtr<Transform3<real_Num>> worldTransform = getWorldTransform();

            Vector3<real_Num>    p = worldTransform->getPosition();
            Quaternion<real_Num> r = worldTransform->getOrientation();
            Vector3<real_Num>    scale = worldTransform->getScale();
            Vector3<real_Num>    right = worldTransform->right();
            Vector3<real_Num>    forward = worldTransform->forward();

            p = bodyTransform->inverseTransformPoint( p );
            r = bodyTransform->getOrientation().inverse() * r;

            right = bodyTransform->inverseTransformVector( right );
            forward = bodyTransform->inverseTransformVector( forward );

            forward = -forward; // todo

            // if (scale.X() < real_Num(0.0))
            //{
            //	right = -right;
            // }

            WP_ASSERT( scale.length() > std::numeric_limits<f32>::epsilon() );

            // Calculate root and tip center points.
            Vector3<real_Num> wingRootCenter = p - ( right * ( scale.X() * real_Num( 0.5 ) ) );
            Vector3<real_Num> wingTipCenter = p + ( right * ( scale.X() * real_Num( 0.5 ) ) );
            wingTipCenter += forward * m_wingTipSweep;

            // Calculate corners.
            m_rootLeadingEdge = wingRootCenter + ( forward * ( scale.Z() * real_Num( 0.5 ) ) );
            m_rootTrailingEdge = wingRootCenter - ( forward * ( scale.Z() * real_Num( 0.5 ) ) );
            m_tipLeadingEdge =
                wingTipCenter +
                ( forward * ( ( scale.Z() * real_Num( 0.5 ) ) * m_wingTipWidthZeroToOne ) );
            m_tipTrailingEdge =
                wingTipCenter -
                ( forward * ( ( scale.Z() * real_Num( 0.5 ) ) * m_wingTipWidthZeroToOne ) );

            // Tweak tip corners based on the angle between them.
            Vector3<real_Num>    tipTrailingEdgeToTipLeadingEdge = m_tipLeadingEdge - m_tipTrailingEdge;
            Quaternion<real_Num> rotation =
                Quaternion<real_Num>::angleAxis( m_wingTipAngle, r * Vector3<real_Num>::UNIT_X );
            tipTrailingEdgeToTipLeadingEdge = rotation * tipTrailingEdgeToTipLeadingEdge;
            m_tipTrailingEdge = wingTipCenter - ( tipTrailingEdgeToTipLeadingEdge * real_Num( 0.5 ) );
            m_tipLeadingEdge = wingTipCenter + ( tipTrailingEdgeToTipLeadingEdge * real_Num( 0.5 ) );

            m_rootLiftPosition = m_rootTrailingEdge + ( ( m_rootLeadingEdge - m_rootTrailingEdge ) *
                                                        m_liftLineChordPosition );
            m_tipLiftPosition = m_tipTrailingEdge +
                                ( ( m_tipLeadingEdge - m_tipTrailingEdge ) * m_liftLineChordPosition );

            // m_localWingRight = (wingRootCenter - wingTipCenter).normaliseCopy();
            m_localWingRight = ( wingTipCenter - wingRootCenter ).normaliseCopy();
            // transform->transformVector(m_localWingRight);
            m_localWingRight.normalise();

            if( scale.X() < real_Num( 0.0 ) )
            {
                m_localWingRight = -m_localWingRight;
            }

            m_localWingRight = worldTransform->right();
            m_localWingRight = bodyTransform->inverseTransformVector( m_localWingRight );

            // Calculate wing area.
            m_wingArea = calculateArea( m_rootLeadingEdge, m_tipLeadingEdge, m_tipTrailingEdge,
                                        m_rootTrailingEdge );
            WP_ASSERT( m_wingArea < real_Num( 10.0 ) );
#endif
        }

        real_Num CAircraftWingFast::calculateArea( const Vector3<real_Num> &pointA,
                                                   const Vector3<real_Num> &pointB,
                                                   const Vector3<real_Num> &pointC,
                                                   const Vector3<real_Num> &pointD )
        {
            auto ab = ( pointB - pointA ).length();
            auto bc = ( pointC - pointB ).length();
            auto cd = ( pointD - pointC ).length();
            auto da = ( pointA - pointD ).length();

            WP_ASSERT( ab < static_cast<real_Num>( 10.0 ) );
            WP_ASSERT( bc < static_cast<real_Num>( 10.0 ) );
            WP_ASSERT( cd < static_cast<real_Num>( 10.0 ) );
            WP_ASSERT( da < static_cast<real_Num>( 10.0 ) );

            auto s = ( ab + bc + cd + da ) * static_cast<real_Num>( 0.5 );
            WP_ASSERT( Math<real_Num>::isFinite( s ) );

            auto squareArea = ( s - ab ) * ( s - bc ) * ( s - cd ) * ( s - da );
            WP_ASSERT( Math<real_Num>::isFinite( squareArea ) );

            WP_ASSERT( Math<real_Num>::isFinite( Math<real_Num>::Sqrt( squareArea ) ) );
            return Math<real_Num>::Sqrt( squareArea );
        }

        s32 CAircraftWingFast::getDebugId( s32 section, s32 index ) const
        {
            if( section < m_debugIds.size() )
            {
                if( index < m_debugIds[section].size() )
                {
                    int id = m_debugIds[section][index];
                    WP_ASSERT( id != 0 );
                    return id;
                }
            }

            return 0;
        }

        s32 CAircraftWingFast::getDebugId( s32 i ) const
        {
            if( i < m_ids.size() )
            {
                return m_ids[i];
            }

            return 0;
        }

        void CAircraftWingFast::drawGizmos()
        {
            // Draw icon.
            // Gizmos.DrawIcon(transform.position, "wing.png", true);

            // WingBoxCollider = (BoxCollider)gameObject.GetComponent<Collider>();
            // if (null != WingBoxCollider)
            {
                // Clamp box collider scales.
                // WingBoxCollider.size = new Vector3(1.0f, 0.1f, 1.0f);

                // Wing geometry.
                // DEBUG_DRAW_LINE_BY_ID(getDebugId(1), WingRootLeadingEdge, WingTipLeadingEdge,
                // 0x00FF00); DEBUG_DRAW_LINE_BY_ID(getDebugId(2), WingTipTrailingEdge,
                // WingRootTrailingEdge, 0xFF00000); DEBUG_DRAW_LINE_BY_ID(getDebugId(3),
                // WingRootTrailingEdge, WingRootLeadingEdge, 0x0000FF);
                // DEBUG_DRAW_LINE_BY_ID(getDebugId(4), WingTipLeadingEdge, WingTipTrailingEdge,
                // 0xFFF000);

                // Sections.
                // Gizmos.color = Color.blue;
                // for (int i = 0; i < SectionCount; i++)
                //{
                //	Vector3 sectionStart = WingRootTrailingEdge + ((WingTipTrailingEdge -
                // WingRootTrailingEdge) * (float)i / (float)SectionCount); 	Vector3 sectionEnd =
                // WingRootLeadingEdge + ((WingTipLeadingEdge - WingRootLeadingEdge) * (float)i /
                //(float)SectionCount);
                //	//Gizmos.DrawLine( sectionStart, sectionEnd );
                // }

                // Lift line.
                // Gizmos.color = Color.green;
                // Gizmos.DrawLine( RootLiftPosition, TipLiftPosition );

                // Aileron hinge
                // AttachedControlSurface = gameObject.GetComponent<ControlSurface>();
                // if (null != AttachedControlSurface)
                //{
                //	float rootHingeOffset = AttachedControlSurface.RootHingeDistanceFromTrailingEdge;
                //	float tipHingeOffset = AttachedControlSurface.m_tipHingeDistanceFromTrailingEdge;

                //	Vector3 wingRootAileronHingePos = WingRootTrailingEdge + ((WingRootLeadingEdge -
                // WingRootTrailingEdge) * rootHingeOffset); 	Vector3 wingTipAileronHingePos =
                // WingTipTrailingEdge + ((WingTipLeadingEdge - WingTipTrailingEdge) * tipHingeOffset);

                //	Gizmos.color = Color.magenta;
                //	//Gizmos.DrawLine( wingRootAileronHingePos, wingTipAileronHingePos );

                //	//Control surface - Draw crosses over each control surface section which is affected.
                //	if (null != AttachedControlSurface.AffectedSections)
                //	{
                //		for (int i = 0; i < AttachedControlSurface.AffectedSections.Length; i++)
                //		{
                //			if (AttachedControlSurface.AffectedSections[i] == true)
                //			{
                //				Vector3 hingeLeft = wingRootAileronHingePos + ((wingTipAileronHingePos -
                // wingRootAileronHingePos) * ((float)i /
                //(float)AttachedControlSurface.AffectedSections.Length)); 				Vector3
                //hingeRight = wingRootAileronHingePos + ((wingTipAileronHingePos -
                // wingRootAileronHingePos) *
                //((float)(i + 1) / (float)AttachedControlSurface.AffectedSections.Length));

                //				Vector3 backLeft = WingRootTrailingEdge + ((WingTipTrailingEdge -
                // WingRootTrailingEdge) * ((float)i /
                //(float)AttachedControlSurface.AffectedSections.Length)); 				Vector3 backRight
                //= WingRootTrailingEdge + ((WingTipTrailingEdge - WingRootTrailingEdge) * ((float)(i +
                // 1)
                /// (float)AttachedControlSurface.AffectedSections.Length));

                //				//Gizmos.DrawLine( hingeLeft, backRight );
                //				//Gizmos.DrawLine( hingeRight, backLeft );
                //			}
                //		}
                //	}
                //}

                ////Prop wash - Draw crosses over each control surface section which is affected.
                // m_attachedPropWash = gameObject.GetComponent<PropWash>();
                // if (null != m_attachedPropWash)
                //{
                //	Gizmos.color = Color.cyan;
                //	if (null != m_attachedPropWash.AffectedSections)
                //	{
                //		for (int i = 0; i < m_attachedPropWash.AffectedSections.Length; i++)
                //		{
                //			if (m_attachedPropWash.AffectedSections[i] == true)
                //			{
                //				Vector3 frontLeft = WingRootLeadingEdge + ((WingTipLeadingEdge -
                // WingRootLeadingEdge) * ((float)i /
                // (float)m_attachedPropWash.AffectedSections.Length)); 				Vector3 frontRight =
                //WingRootLeadingEdge + ((WingTipLeadingEdge - WingRootLeadingEdge) * ((float)(i + 1) /
                //(float)m_attachedPropWash.AffectedSections.Length));

                //				Vector3 backLeft = WingRootTrailingEdge + ((WingTipTrailingEdge -
                // WingRootTrailingEdge) * ((float)i /
                //(float)m_attachedPropWash.AffectedSections.Length)); 				Vector3 backRight =
                // WingRootTrailingEdge + ((WingTipTrailingEdge - WingRootTrailingEdge) * ((float)(i + 1)
                /// (float)m_attachedPropWash.AffectedSections.Length));

                //				//Vector3 topCenter = hingeLeft + ( (hingeRight-hingeLeft) * 0.5f );
                //				//Vector3 bottomCenter = backLeft + ( (backRight-backLeft) * 0.5f );
                //				///Vector3 leftCenter = backLeft + ( (hingeLeft-backLeft) * 0.5f );
                //				//Vector3 rightCenter = backRight + ( (hingeRight-backRight) * 0.5f );

                //				//Gizmos.DrawLine( frontLeft, backRight );
                //				//Gizmos.DrawLine( frontRight, backLeft );
                //			}
                //		}
                //	}
                //}
            }
        }

        Vector3<real_Num> CAircraftWingFast::calculateRelativeWind(
            s32 sectionIndex, const Vector3<real_Num> &aerodynamicCenter, const Vector3<real_Num> &wind )
        {
            SmartPtr<IVehicleBody> aircraftBody = m_parent;
            auto                   aircraft = m_parentAircraft;

            auto aircraftTransform = aircraft->getBodyTransform();
            auto worldTransform = getWorldTransform();
            auto localTransform = getLocalTransform();
            auto propwash = m_propWash;

            Vector3<real_Num> worldAerodynamicCenter =
                aircraftTransform.transformPoint( aerodynamicCenter );

            Vector3<real_Num> relativeWind = wind - aircraft->getPointVelocity( worldAerodynamicCenter );

            real_Num maxVelocity = static_cast<real_Num>( 10000.0 );
            relativeWind.X() = Math<real_Num>::clamp( relativeWind.X(), -maxVelocity, maxVelocity );
            relativeWind.Y() = Math<real_Num>::clamp( relativeWind.Y(), -maxVelocity, maxVelocity );
            relativeWind.Z() = Math<real_Num>::clamp( relativeWind.Z(), -maxVelocity, maxVelocity );

            ////

            ////Calculate angular to linear velocity of any rotation and add to relative wind.
            // Vector3<real_Num> fromCOMToAerodynamicCenter = aerodynamicCenter -
            // aircraftBody->getWorldCenterOfMass(); Vector3<real_Num> angularVelocity =
            // aircraftBody->getAngularVelocity();

            // Vector3<real_Num> localRelativeWind =
            // angularVelocity.crossProduct(fromCOMToAerodynamicCenter); localRelativeWind *=
            // -((angularVelocity.length()) * fromCOMToAerodynamicCenter.length());

            //////Tweak rollwise damping based on parent aircraft if it exists.
            // if (aircraft != nullptr)
            //{
            //	localRelativeWind *= aircraft->getRollwiseDamping();
            // }

            // real_Num maxLocalRelativeWind = real_Num(3000.0);
            // localRelativeWind.X() = Math<real_Num>::clamp(localRelativeWind.X(),
            // -maxLocalRelativeWind, maxLocalRelativeWind); localRelativeWind.Y() =
            // Math<real_Num>::clamp(localRelativeWind.Y(), -maxLocalRelativeWind, maxLocalRelativeWind);
            // localRelativeWind.Z() = Math<real_Num>::clamp(localRelativeWind.Z(),
            // -maxLocalRelativeWind, maxLocalRelativeWind);

            // WP_ASSERT(localRelativeWind.length() < real_Num(10.0));

            // Apply
            // relativeWind += localRelativeWind;

            relativeWind = aircraftTransform.inverseTransformVector( relativeWind );

            if( propwash != nullptr )
            {
                relativeWind += propwash->getPropWash( sectionIndex );

                // if (aircraft->getDisplayDebugData())
                //{
                //	auto propWash = propwash->getPropWash(sectionIndex);
                //	DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIndex, 3), aerodynamicCenter,
                // aerodynamicCenter + propWash, 0x00FF00);
                // }
            }

            // Vector3<real_Num> localRight =
            // aircraftTransform->inverseTransformVector(worldTransform->right()); real_Num
            // perpChordDotRelativeWind = localRight.dotProduct(localRelativeWind); Vector3<real_Num>
            // localCorrection = localRight * perpChordDotRelativeWind; localRelativeWind -=
            // localCorrection;

            // relativeWind = aircraftTransform->transformDirection(localRelativeWind);

            // Tweak relative wind so we only consider that which is flowing over the wing.
            // DEBUG_DRAW_LINE_BY_ID(aerodynamicCenter - (relativeWind.normalized), aerodynamicCenter,
            // Color.grey); Vector3<real_Num> liftVector = relativeWind; real_Num relativeWindLength =
            // relativeWind.length(); Vector3<real_Num> correction = worldTransform->right(); real_Num
            // perpChordDotRelativeWind = correction.dotProduct(relativeWind); correction *=
            // perpChordDotRelativeWind; relativeWind -= correction;

            // relativeWind = relativeWind.normaliseCopy()* (relativeWindLength +
            // perpChordDotRelativeWind); Debug.DrawLine(aerodynamicCenter - relativeWind.normalized,
            // aerodynamicCenter, Color.white);

            // if (aircraft->getDisplayDebugData())
            //{
            //	DEBUG_DRAW_LINE_BY_ID(getDebugId(sectionIndex, 9), aerodynamicCenter, aerodynamicCenter +
            //(relativeWind * 1.0f), 0xFF0000);
            // }

            return relativeWind;
        }

        void CAircraftWingFast::calculateRelativeWindMinus10()
        {
            SmartPtr<IVehicleBody> aircraftBody = m_parent;
            auto                   aircraft = m_parentAircraft;

            auto aircraftTransform = aircraft->getBodyTransform();
            // SmartPtr<Transform3<real_Num>>& worldTransform = getWorldTransform();
            auto localTransform = getLocalTransform();
            // SmartPtr<Transform3<real_Num>>& localBodyTransform = getLocalBodyTransform();
            SmartPtr<IAerodymanicsWind> pWind = aircraft->getWind();

            auto wind = pWind;
            if( !wind )
            {
                return;
            }

            auto aircraftWorldPosition = aircraftTransform.getPosition();
            auto windVector = wind->getWind( aircraftWorldPosition.Y(), 0.0 );

            auto attachedControlSurface = m_controlSurface;
            auto propwash = m_propWash;
            auto aerofoil = m_aerofoil;

            const float debugLineScale = 5.0f; // 1.0f / 30.0f;

            s32 lineId = 0;

            auto airDensity = aircraft->getAirDensity();

            auto localWingRootLeadingEdge = m_rootLeadingEdge;
            auto localWingRootTrailingEdge = m_rootTrailingEdge;
            auto localWingTipLeadingEdge = m_tipLeadingEdge;
            auto localWingTipTrailingEdge = m_tipTrailingEdge;
            auto localRootLiftPosition = m_rootLiftPosition;
            auto localTipLiftPosition = m_tipLiftPosition;
            auto liftLineChordPosition = m_liftLineChordPosition;
            auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                if( m_controlSurface )
                {
                    if( isControlSurface() )
                    {
                        if( !m_controlSurface->isAffectedSection( sectionIdx ) )
                        {
                            continue;
                        }
                    }
                }

                auto aerodynamicCenter = m_aerodynamicCentersMinus10[sectionIdx];

                auto relativeWind = calculateRelativeWind3( sectionIdx, aerodynamicCenter, windVector );

                if( !aircraft->getEnablePowerUnit() )
                {
                    static auto w = Vector3<real_Num>( 0, 0, -5 );

                    Quaternion<real_Num> q;
                    q.fromDegrees( m_windRotation );

                    relativeWind = q * w;
                    // relativeWind.normalise();

                    // relativeWind = aircraft->getBodyTransform()->inverseTransformVector(relativeWind);
                }

                // Find the angle of attack.
                m_relativeWindMinus10[sectionIdx] = relativeWind;
            }
        }

        void CAircraftWingFast::calculateRelativeWindPlus10()
        {
            SmartPtr<IVehicleBody> aircraftBody = m_parent;
            auto                   aircraft = m_parentAircraft;

            auto                        aircraftTransform = aircraft->getBodyTransform();
            auto                        worldTransform = getWorldTransform();
            auto                        localTransform = getLocalTransform();
            auto                        localBodyTransform = getLocalBodyTransform();
            SmartPtr<IAerodymanicsWind> pWind = aircraft->getWind();

            auto wind = pWind;
            if( !wind )
            {
                return;
            }

            auto aircraftWorldPosition = aircraftTransform.getPosition();
            auto windVector = wind->getWind( aircraftWorldPosition.Y(), 0.0 );

            auto attachedControlSurface = m_controlSurface;
            auto propwash = m_propWash;
            auto aerofoil = m_aerofoil;

            const float debugLineScale = 5.0f; // 1.0f / 30.0f;

            s32 lineId = 0;

            auto airDensity = aircraft->getAirDensity();

            auto localWingRootLeadingEdge = m_rootLeadingEdge;
            auto localWingRootTrailingEdge = m_rootTrailingEdge;
            auto localWingTipLeadingEdge = m_tipLeadingEdge;
            auto localWingTipTrailingEdge = m_tipTrailingEdge;
            auto localRootLiftPosition = m_rootLiftPosition;
            auto localTipLiftPosition = m_tipLiftPosition;
            auto liftLineChordPosition = m_liftLineChordPosition;
            auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                if( m_controlSurface )
                {
                    if( isControlSurface() )
                    {
                        if( !m_controlSurface->isAffectedSection( sectionIdx ) )
                        {
                            continue;
                        }
                    }
                }

                auto aerodynamicCenter = m_aerodynamicCentersPlus10[sectionIdx];

                auto relativeWind = calculateRelativeWind3( sectionIdx, aerodynamicCenter, windVector );

                if( !aircraft->getEnablePowerUnit() )
                {
                    static auto w = Vector3<real_Num>( 0, 0, -5 );

                    Quaternion<real_Num> q;
                    q.fromDegrees( m_windRotation );

                    relativeWind = q * w;
                    // relativeWind.normalise();

                    // relativeWind = aircraft->getBodyTransform()->inverseTransformVector(relativeWind);
                }

                // Find the angle of attack.
                m_relativeWindPlus10[sectionIdx] = relativeWind;
            }
        }

        void CAircraftWingFast::calculateRelativeWind()
        {
            SmartPtr<IVehicleBody> aircraftBody = m_parent;
            auto                   aircraft = m_parentAircraft;

            auto                        aircraftTransform = aircraft->getBodyTransform();
            auto                        worldTransform = getWorldTransform();
            auto                        localTransform = getLocalTransform();
            auto                        localBodyTransform = getLocalBodyTransform();
            SmartPtr<IAerodymanicsWind> pWind = aircraft->getWind();

            auto wind = pWind;
            if( !wind )
            {
                return;
            }

            auto aircraftWorldPosition = aircraftTransform.getPosition();
            auto windVector = wind->getWind( aircraftWorldPosition.Y(), 0.0 );

            auto attachedControlSurface = m_controlSurface;
            auto propwash = m_propWash;
            auto aerofoil = m_aerofoil;

            const float debugLineScale = 5.0f; // 1.0f / 30.0f;

            s32 lineId = 0;

            auto airDensity = aircraft->getAirDensity();

            auto localWingRootLeadingEdge = m_rootLeadingEdge;
            auto localWingRootTrailingEdge = m_rootTrailingEdge;
            auto localWingTipLeadingEdge = m_tipLeadingEdge;
            auto localWingTipTrailingEdge = m_tipTrailingEdge;
            auto localRootLiftPosition = m_rootLiftPosition;
            auto localTipLiftPosition = m_tipLiftPosition;
            auto liftLineChordPosition = m_liftLineChordPosition;
            auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                if( m_controlSurface )
                {
                    if( isControlSurface() )
                    {
                        if( !m_controlSurface->isAffectedSection( sectionIdx ) )
                        {
                            continue;
                        }
                    }
                }

                auto aerodynamicCenter = m_aerodynamicCenters[sectionIdx];

                auto relativeWind = calculateRelativeWind3( sectionIdx, aerodynamicCenter, windVector );

                if( !aircraft->getEnablePowerUnit() )
                {
                    static auto w = Vector3<real_Num>( 0, 0, -5 );

                    Quaternion<real_Num> q;
                    q.fromDegrees( m_windRotation );

                    relativeWind = q * w;
                    // relativeWind.normalise();

                    // relativeWind = aircraft->getBodyTransform()->inverseTransformVector(relativeWind);
                }

                // Find the angle of attack.
                m_relativeWind[sectionIdx] = relativeWind;
            }
        }

        Vector3<real_Num> CAircraftWingFast::calculateRelativeWind2(
            s32 sectionIndex, const Vector3<real_Num> &aerodynamicCenter, const Vector3<real_Num> &wind )
        {
            // return Vector3<real_Num>::zero();

            auto aircraftBody = m_parent;
            auto aircraft = m_parentAircraft;

            auto aircraftTransform = aircraft->getBodyTransform();
            auto worldTransform = getWorldTransform();
            auto localTransform = getLocalTransform();

            auto relativeWind = wind - aircraft->getLinearVelocity();

            auto worldAerodynamicCenter = aircraftTransform.transformPoint( aerodynamicCenter );
            auto fromCOMToAerodynamicCenter =
                worldAerodynamicCenter - aircraftBody->getWorldCenterOfMass();
            auto angularVelocity = aircraftBody->getAngularVelocity();

            auto localRelativeWind = fromCOMToAerodynamicCenter.crossProduct( angularVelocity );

            localRelativeWind *= aircraft->getRollwiseDamping();

            relativeWind += localRelativeWind;
            relativeWind = aircraftTransform.inverseTransformVector( relativeWind );

            if( m_propWash )
            {
                relativeWind += m_propWash->getPropWash( sectionIndex );
            }

            // auto correction = m_localWingRight;
            // const auto perpChordDotRelativeWind = correction.dotProduct(relativeWind);
            // correction *= perpChordDotRelativeWind;
            // relativeWind -= correction;

            // Tweak relative wind so we only consider that which is flowing over the wing.
            // DEBUG_DRAW_LINE_BY_ID(aerodynamicCenter - (relativeWind.normalized), aerodynamicCenter,
            // Color.grey); Vector3<real_Num> liftVector = relativeWind; real_Num relativeWindLength =
            // relativeWind.length(); Vector3<real_Num> correction = worldTransform->right(); real_Num
            // perpChordDotRelativeWind = correction.dotProduct(relativeWind); correction *=
            // perpChordDotRelativeWind; relativeWind -= correction;

            const real_Num maxVelocity = static_cast<real_Num>( 10000.0 );
            relativeWind.X() = Math<real_Num>::clamp( relativeWind.X(), -maxVelocity, maxVelocity );
            relativeWind.Y() = Math<real_Num>::clamp( relativeWind.Y(), -maxVelocity, maxVelocity );
            relativeWind.Z() = Math<real_Num>::clamp( relativeWind.Z(), -maxVelocity, maxVelocity );

            return relativeWind;
        }

        Vector3<real_Num> CAircraftWingFast::calculateRelativeWind3(
            s32 sectionIndex, const Vector3<real_Num> &aerodynamicCenter, const Vector3<real_Num> &wind )
        {
            // return Vector3<real_Num>::zero();

            auto aircraftBody = m_parent;
            auto aircraft = m_parentAircraft;

            auto aircraftTransform = aircraft->getBodyTransform();
            auto worldTransform = getWorldTransform();
            auto localTransform = getLocalTransform();

            auto relativeWind = wind - aircraft->getLocalLinearVelocity();

            auto fromCOMToAerodynamicCenter = aerodynamicCenter - aircraft->getCG();
            auto angularVelocity = aircraftBody->getLocalAngularVelocity();

            auto localRelativeWind = fromCOMToAerodynamicCenter.crossProduct( angularVelocity );

            localRelativeWind *= aircraft->getRollwiseDamping();

            relativeWind += localRelativeWind;

            if( m_propWash )
            {
                relativeWind += m_propWash->getPropWash( sectionIndex );
            }

            // auto correction = m_localWingRight;
            // const auto perpChordDotRelativeWind = correction.dotProduct(relativeWind);
            // correction *= perpChordDotRelativeWind;
            // relativeWind -= correction;

            // Tweak relative wind so we only consider that which is flowing over the wing.
            // DEBUG_DRAW_LINE_BY_ID(aerodynamicCenter - (relativeWind.normalized), aerodynamicCenter,
            // Color.grey); Vector3<real_Num> liftVector = relativeWind; real_Num relativeWindLength =
            // relativeWind.length(); Vector3<real_Num> correction = worldTransform->right(); real_Num
            // perpChordDotRelativeWind = correction.dotProduct(relativeWind); correction *=
            // perpChordDotRelativeWind; relativeWind -= correction;

            const real_Num maxVelocity = static_cast<real_Num>( 10000.0 );
            relativeWind.X() = Math<real_Num>::clamp( relativeWind.X(), -maxVelocity, maxVelocity );
            relativeWind.Y() = Math<real_Num>::clamp( relativeWind.Y(), -maxVelocity, maxVelocity );
            relativeWind.Z() = Math<real_Num>::clamp( relativeWind.Z(), -maxVelocity, maxVelocity );

            return relativeWind;
        }

        void CAircraftWingFast::update( const double &t, const double &fDT )
        {
            // SmartPtr<IVehicleBody> aircraftBody = m_parent;
            // auto aircraft = m_parentAircraft;

            ////const SmartPtr<Transform3<real_Num>>& aircraftTransform = aircraft->getBodyTransform();
            ////SmartPtr<Transform3<real_Num>>& worldTransform = getWorldTransform();
            // const SmartPtr<Transform3<real_Num>>& localTransform = getLocalTransform();
            ////SmartPtr<Transform3<real_Num>>& localBodyTransform = getLocalBodyTransform();
            // SmartPtr<IAerodymanicsWind> pWind = aircraft->getWind();

            // auto wind = pWind;
            // auto attachedControlSurface = m_controlSurface;
            // auto propwash = m_propWash;
            // auto aerofoil = m_aerofoil;

            // const float debugLineScale = 5.0f; // 1.0f / 30.0f;

            // s32 lineId = 0;

            // auto airDensity = aircraft->getAirDensity();

            // auto localWingRootLeadingEdge = m_rootLeadingEdge;
            // auto localWingRootTrailingEdge = m_rootTrailingEdge;
            // auto localWingTipLeadingEdge = m_tipLeadingEdge;
            // auto localWingTipTrailingEdge = m_tipTrailingEdge;
            // auto localRootLiftPosition = m_rootLiftPosition;
            // auto localTipLiftPosition = m_tipLiftPosition;
            // auto liftLineChordPosition = m_liftLineChordPosition;
            // auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

            ////m_windRotation.X() += 100.0 * fDT;
            ////m_windRotation.X() = Math<real_Num>::wrap(m_windRotation.X(), -25.0, 25.0);

            // static real_Num angle = 45.0;
            // m_windRotation.X() = angle;

            // DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(11), m_tipLiftPosition, m_tipLiftPosition +
            // (m_localWingRight * 0.1), 0x000000);

            updateGeometry();
            updatePoints();
            updateVisualPoints();
            calculateAera();

            calculateModifiedSections();
            // calculateModifiedSections10();
            // calculateModifiedSectionsMinus10();

            calculateAeroProperties();
            // calculateAeroProperties10();
            // calculateAeroPropertiesMinus10();

            calculateRelativeWind();

            calculateAlpha();
            // calculateAlpha10();
            // calculateAlphaMinus10();

            calculateStallControl();
            calculateForces( fDT );

            updateForcesFast();
        }

        void CAircraftWingFast::updateWorker()
        {
            auto rate = 1.0 / 120.0;
            auto timer = boost::make_shared<TimerBoost>();

            while( getLoadingState() == LoadingState::Loaded )
            {
                auto startTime = timer->now();
                updateGeometry();
                updatePoints();
                calculateAera();

                calculateModifiedSections();
                calculateAera();
                calculateAeroProperties();
                calculateRelativeWind();
                calculateAlpha();
                calculateForces( rate );

                auto endTime = timer->now();
                auto timeTaken = endTime - startTime;
                auto sleepTime = rate - timeTaken;
                if( sleepTime > 0.0 )
                {
                    Thread::sleep( sleepTime );
                }
            }
        }

        void CAircraftWingFast::calculateStallControl()
        {
            auto aircraft = m_parentAircraft;
            auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();
            auto attachedControlSurface = m_controlSurface;

            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                auto area = m_area[sectionIdx];
                auto chordLine = m_chordLines[sectionIdx];
                // auto chordLength = m_chordLengths[sectionIdx];
                // auto upFull = m_up[sectionIdx];
                // auto aerodynamicCenter = m_aerodynamicCenters[sectionIdx];
                auto relativeWind = m_relativeWind[sectionIdx];
                auto relativeWindLength = relativeWind.length();
                auto relativeWindNormalised = relativeWind.normaliseCopy();
                auto angleOfAttackFull = m_aoa[sectionIdx];

                auto fStallControlCL = static_cast<real_Num>( 0.0 );
                auto fStallControlCD = static_cast<real_Num>( 0.0 );
                auto fStallControlCM = static_cast<real_Num>( 0.0 );

#if defined _DEBUG
                if( !Math<real_Num>::equals( m_stallControlCL, 0.0 ) )
                {
                    int stop = 0;
                    stop = 0;
                }
#endif

                auto chordLineDotRelWindFull = chordLine.dotProduct( -relativeWindNormalised );
                auto aoaNormalised = Math<real_Num>::clamp(
                    angleOfAttackFull / static_cast<real_Num>( 180.0 ), -1.0, 1.0 );

                if( chordLineDotRelWindFull > std::numeric_limits<real_Num>::epsilon() )
                {
                    if( attachedControlSurface != nullptr )
                    {
                        if( isControlSurface() )
                        {
                            auto stallThreshold = m_controlSurface->getStallThreshold();
                            auto stallControlCL = m_controlSurface->getStallControlCL();
                            auto stallControlCD = m_controlSurface->getStallControlCD();
                            auto stallControlCM = m_controlSurface->getStallControlCM();

                            if( aoaNormalised > stallThreshold )
                            {
                                auto range = static_cast<real_Num>( 1.0 ) - stallThreshold;
                                auto fUpDotRelWind = aoaNormalised - stallThreshold;

                                if( range > std::numeric_limits<real_Num>::epsilon() )
                                {
                                    fStallControlCL = ( fUpDotRelWind / range ) * stallControlCL;
                                    fStallControlCD = ( fUpDotRelWind / range ) * stallControlCD;
                                    fStallControlCM = ( fUpDotRelWind / range ) * stallControlCM;
                                }
                            }
                            else if( aoaNormalised < -stallThreshold )
                            {
                                auto range = static_cast<real_Num>( 1.0 ) - stallThreshold;
                                auto fUpDotRelWind = aoaNormalised + stallThreshold;

                                if( range > std::numeric_limits<real_Num>::epsilon() )
                                {
                                    fStallControlCL = ( fUpDotRelWind / range ) * stallControlCL;
                                    fStallControlCD = ( fUpDotRelWind / range ) * stallControlCD;
                                    fStallControlCM = ( fUpDotRelWind / range ) * stallControlCM;
                                }
                            }
                        }
                        else
                        {
                            auto stallThreshold = m_stallThreshold;
                            auto stallControlCL = m_stallControlCL;
                            auto stallControlCD = m_stallControlCD;
                            auto stallControlCM = m_stallControlCM;

                            if( aoaNormalised > stallThreshold )
                            {
                                auto range = static_cast<real_Num>( 1.0 ) - stallThreshold;
                                auto fUpDotRelWind = aoaNormalised - stallThreshold;

                                if( range > std::numeric_limits<real_Num>::epsilon() )
                                {
                                    fStallControlCL = ( fUpDotRelWind / range ) * stallControlCL;
                                    fStallControlCD = ( fUpDotRelWind / range ) * stallControlCD;
                                    fStallControlCM = ( fUpDotRelWind / range ) * stallControlCM;
                                }
                            }
                            else if( aoaNormalised < -stallThreshold )
                            {
                                auto range = static_cast<real_Num>( 1.0 ) - stallThreshold;
                                auto fUpDotRelWind = aoaNormalised + stallThreshold;

                                if( range > std::numeric_limits<real_Num>::epsilon() )
                                {
                                    fStallControlCL = ( fUpDotRelWind / range ) * stallControlCL;
                                    fStallControlCD = ( fUpDotRelWind / range ) * stallControlCD;
                                    fStallControlCM = ( fUpDotRelWind / range ) * stallControlCM;
                                }
                            }
                        }
                    }
                }

                m_fStallControlCL[sectionIdx] = fStallControlCL;
                m_fStallControlCD[sectionIdx] = fStallControlCD;
                m_fStallControlCM[sectionIdx] = fStallControlCM;
            }
        }

        void CAircraftWingFast::calculateForces( double fDT )
        {
            SmartPtr<IVehicleBody> aircraftBody = m_parent;
            auto                   aircraft = m_parentAircraft;

            // const SmartPtr<Transform3<real_Num>>& aircraftTransform = aircraft->getBodyTransform();
            // SmartPtr<Transform3<real_Num>>& worldTransform = getWorldTransform();
            auto localTransform = getLocalTransform();
            // SmartPtr<Transform3<real_Num>>& localBodyTransform = getLocalBodyTransform();
            SmartPtr<IAerodymanicsWind> pWind = aircraft->getWind();

            auto wind = pWind;
            auto attachedControlSurface = m_controlSurface;
            auto propwash = m_propWash;
            auto aerofoil = m_aerofoil;

            const float debugLineScale = 5.0f; // 1.0f / 30.0f;

            s32 lineId = 0;

            auto airDensity = aircraft->getAirDensity();

            auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

            // Per section update.
            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                if( attachedControlSurface )
                {
                    if( isControlSurface() )
                    {
                        if( !attachedControlSurface->isAffectedSection( sectionIdx ) )
                        {
                            continue;
                        }
                    }
                }

                auto area = m_area[sectionIdx];
                auto chordLine = m_chordLines[sectionIdx];
                // auto chordLength = m_chordLengths[sectionIdx];
                // auto upFull = m_up[sectionIdx];
                // auto aerodynamicCenter = m_aerodynamicCenters[sectionIdx];
                auto relativeWind = m_relativeWind[sectionIdx];
                auto relativeWindLength = relativeWind.length();
                auto relativeWindNormalised = relativeWind.normaliseCopy();
                auto angleOfAttackZero = m_aoa[sectionIdx];
                auto angleOfAttack10 = m_aoaPlus10[sectionIdx];
                auto angleOfAttackMinus10 = m_aoaMinus10[sectionIdx];

                auto chordLineRelativeWindDot = chordLine.dotProduct( relativeWindNormalised );

                ////Draw modified wing.
                if( aircraft->getDisplayDebugData() )
                {
                    // if (isControlSurface())
                    //{
                    auto a = m_modifiedSectionA[sectionIdx];
                    auto b = m_modifiedSectionB[sectionIdx];
                    auto c = m_modifiedSectionC[sectionIdx];
                    auto d = m_modifiedSectionD[sectionIdx];

                    DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 4 ), a, b, 0x00FF00 );
                    DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 5 ), b, c, 0xFF0000 );
                    DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 6 ), c, d, 0x0000FF );
                    DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 7 ), d, a, 0x00FFFF );
                    //}
                    // else
                    //{
                    //	auto a = m_visualSectionA[sectionIdx];
                    //	auto b = m_visualSectionB[sectionIdx];
                    //	auto c = m_visualSectionC[sectionIdx];
                    //	auto d = m_visualSectionD[sectionIdx];

                    //	DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 4), a, b, 0x00FF00);
                    //	DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 5), b, c, 0xFF0000);
                    //	DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 6), c, d, 0x0000FF);
                    //	DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 7), d, a, 0x00FFFF);
                    //}

                    // auto a10 = m_modifiedSectionA10[sectionIdx];
                    // auto b10 = m_modifiedSectionB10[sectionIdx];
                    // auto c10 = m_modifiedSectionC10[sectionIdx];
                    // auto d10 = m_modifiedSectionD10[sectionIdx];

                    // DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 8), a10, b10, 0x00FF00);
                    // DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 9), b10, c10, 0xFF0000);
                    // DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 10), c10, d10, 0x0000FF);
                    // DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 10), d10, a10, 0x00FFFF);

                    // auto aM10 = m_modifiedSectionAMinus10[sectionIdx];
                    // auto bM10 = m_modifiedSectionBMinus10[sectionIdx];
                    // auto cM10 = m_modifiedSectionCMinus10[sectionIdx];
                    // auto dM10 = m_modifiedSectionDMinus10[sectionIdx];

                    // DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 11), aM10, bM10, 0x00FF00);
                    // DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 12), bM10, cM10, 0xFF0000);
                    // DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 13), cM10, dM10, 0x0000FF);
                    // DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 14), dM10, aM10, 0x00FFFF);
                }

                // aerodynamicCenter += chordLine * chordLength * real_Num(0.5);

                // if (aircraft->getDisplayDebugData())
                //{
                //	if (getName().find("WING") != std::string::npos)
                //	{
                //		DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(i, 8), aerodynamicCenter,
                // aerodynamicCenter + chordLine, 			0x0000FF);
                //	}
                // }

                // if (aircraft->getDisplayDebugData())
                //{
                //	if (getName().find("WING") != std::string::npos)
                //	{
                //		DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 8), aerodynamicCenter,
                // aerodynamicCenter + chordLineFull, 			0x0000FF);
                //	}
                // }

#if defined _DEBUG
                if( getName().find( "STAB" ) != std::string::npos )
                {
                    // DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 9), aerodynamicCenter, aerodynamicCenter +
                    // (relativeWind * 1.0f), 0xFF0000);
                    int stop = 0;
                    stop = 0;
                }

                if( getName().find( "WING" ) != std::string::npos )
                {
                    // DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 9), aerodynamicCenter, aerodynamicCenter +
                    // (relativeWind * 1.0f), 0xFF0000);
                    int stop = 0;
                    stop = 0;
                }
#endif

                auto totalLift = static_cast<real_Num>( 0.0 );
                auto totalDrag = static_cast<real_Num>( 0.0 );
                auto cM = static_cast<real_Num>( 0.0 );

                auto clGroundEffectMult = static_cast<real_Num>( 1.0 );
                auto cdGroundEffectMult = static_cast<real_Num>( 1.0 );

                auto fStallControlCL = m_fStallControlCL[sectionIdx];
                auto fStallControlCD = m_fStallControlCD[sectionIdx];
                auto fStallControlCM = m_fStallControlCM[sectionIdx];

                if( m_groundEffect )
                {
                    auto a = m_modifiedSectionA[sectionIdx];
                    auto b = m_modifiedSectionB[sectionIdx];
                    auto c = m_modifiedSectionC[sectionIdx];
                    auto d = m_modifiedSectionD[sectionIdx];

                    m_groundEffect->getGroundEffectCoefficients( a, b, c, d, clGroundEffectMult,
                                                                 cdGroundEffectMult );
                }

                real_Num cL = static_cast<real_Num>( 0.0 );

                if( aerofoil )
                {
                    auto CL = aerofoil->getCL();
                    auto CD = aerofoil->getCD();
                    auto CM = aerofoil->getCM();

                    auto clZero = CL->interpolate( angleOfAttackZero );
                    auto cdZero = CD->interpolate( angleOfAttackZero );
                    auto cmZero = CM->interpolate( angleOfAttackZero );

                    auto camber = static_cast<real_Num>( 0.0 );
                    auto camberNormalised = static_cast<real_Num>( 0.0 );
                    auto fCurrentDeflection = static_cast<real_Num>( 0.0 );

                    if( m_controlSurface )
                    {
                        fCurrentDeflection =
                            m_controlSurface->getCurrentDeflection() * static_cast<real_Num>( 2.0 );

                        if( m_controlSurface->isReversed() )
                        {
                            camberNormalised = -fCurrentDeflection;
                            camber = -fCurrentDeflection;
                        }
                        else
                        {
                            camberNormalised = fCurrentDeflection;
                            camber = fCurrentDeflection;
                        }

                        camber = fCurrentDeflection > static_cast<real_Num>( 0.0 )
                                   ? Math<real_Num>::Abs( fCurrentDeflection ) *
                                         m_controlSurface->getMaxDeflectionDegrees()
                                   : Math<real_Num>::Abs( fCurrentDeflection ) *
                                         m_controlSurface->getMinDeflectionDegrees();
                    }

                    // Use aerofoil..
                    // L = cl * a * 0.5f * r * v^2
                    WP_ASSERT( CL );
                    if( m_controlSurface && isControlSurface() )
                    {
                        auto clLookup = m_controlSurface->getClLookup();
                        auto clLookupMultiplier = clLookup->interpolate( camber );
                        auto clMultiplier = m_controlSurface->getClMultiplier();

                        cL = clZero * clMultiplier * clLookupMultiplier * static_cast<real_Num>( 0.3 );

                        auto clampRange = static_cast<real_Num>( 180.0 );

                        if( fStallControlCL > std::numeric_limits<real_Num>::epsilon() ||
                            fStallControlCL < -std::numeric_limits<real_Num>::epsilon() )
                        {
                            cL += ( ( clZero * static_cast<real_Num>( 0.1 ) ) *
                                    Math<real_Num>::Abs( fStallControlCL ) *
                                    Math<real_Num>::Abs( fCurrentDeflection ) );
                        }
                    }
                    else if( m_controlSurface && !isControlSurface() )
                    {
                        if( fStallControlCL > std::numeric_limits<real_Num>::epsilon() ||
                            fStallControlCL < -std::numeric_limits<real_Num>::epsilon() )
                        {
                            cL = clZero + ( ( clZero * static_cast<real_Num>( 0.1 ) ) *
                                            Math<real_Num>::Abs( fStallControlCL ) *
                                            Math<real_Num>::Abs( fCurrentDeflection ) );
                        }
                        else
                        {
                            cL = clZero;
                        }

                        cL *= clGroundEffectMult;
                        cL *= m_clMultiplier;
                    }
                    else
                    {
                        cL = clZero;
                        cL *= clGroundEffectMult;
                        cL *= m_clMultiplier;
                    }

                    totalLift = cL * area * static_cast<real_Num>( 0.5 ) * airDensity *
                                ( relativeWindLength * relativeWindLength );

                    // D = 0.5 * cd * r * (v * v) * a;
                    WP_ASSERT( CD );
                    real_Num cD = 0.0;
                    if( m_controlSurface && isControlSurface() )
                    {
                        auto cdLookup = m_controlSurface->getCdLookup();
                        auto cdLookupValue = cdLookup->interpolate( camber );
                        auto cdMultiplier = m_controlSurface->getCdMultiplier();
                        cD = cdZero * cdMultiplier * cdLookupValue * static_cast<real_Num>( 0.3 );

                        if( fStallControlCD > std::numeric_limits<real_Num>::epsilon() ||
                            fStallControlCD < -std::numeric_limits<real_Num>::epsilon() )
                        {
                            cD += ( ( cdZero * static_cast<real_Num>( 0.1 ) ) *
                                    Math<real_Num>::Abs( fStallControlCD ) *
                                    Math<real_Num>::Abs( fCurrentDeflection ) );
                        }
                    }
                    else if( m_controlSurface && !isControlSurface() )
                    {
                        if( fStallControlCD > std::numeric_limits<real_Num>::epsilon() ||
                            fStallControlCD < -std::numeric_limits<real_Num>::epsilon() )
                        {
                            cD = cdZero + ( ( cdZero * static_cast<real_Num>( 0.1 ) ) *
                                            Math<real_Num>::Abs( fStallControlCD ) *
                                            Math<real_Num>::Abs( fCurrentDeflection ) );
                        }
                        else
                        {
                            cD = cdZero;
                        }

                        cD *= cdGroundEffectMult;
                        cD *= m_cdMultiplier;
                    }
                    else
                    {
                        cD = cdZero;
                        cD *= cdGroundEffectMult;
                        cD *= m_cdMultiplier;
                    }

                    totalDrag = static_cast<real_Num>( 0.5 ) * cD * airDensity *
                                ( relativeWindLength * relativeWindLength ) * area;

                    WP_ASSERT( CM );
                    if( m_controlSurface && isControlSurface() )
                    {
                        auto cmLookup = m_controlSurface->getCmLookup();
                        auto cmLookupValue = cmLookup->interpolate( camber );
                        auto cmMultiplier = m_controlSurface->getCmMultiplier();
                        cM = cmZero * cmMultiplier * cmLookupValue * static_cast<real_Num>( 0.3 );

                        if( fStallControlCM > std::numeric_limits<real_Num>::epsilon() ||
                            fStallControlCM < -std::numeric_limits<real_Num>::epsilon() )
                        {
                            cM += ( ( cmZero * static_cast<real_Num>( 0.1 ) ) *
                                    Math<real_Num>::Abs( fStallControlCM ) *
                                    Math<real_Num>::Abs( fCurrentDeflection ) );
                        }
                    }
                    else if( m_controlSurface && !isControlSurface() )
                    {
                        auto cmLookup = m_controlSurface->getCmLookup();
                        auto cmLookupValue = cmLookup->interpolate( camber );
                        auto cmMultiplier = m_controlSurface->getCmMultiplier();
                        auto delta = camberNormalised * cmMultiplier * cmLookupValue;

                        if( fStallControlCM > std::numeric_limits<real_Num>::epsilon() ||
                            fStallControlCM < -std::numeric_limits<real_Num>::epsilon() )
                        {
                            cM = cmZero + ( ( cmZero * static_cast<real_Num>( 0.1 ) ) *
                                            Math<real_Num>::Abs( fStallControlCM ) *
                                            Math<real_Num>::Abs( fCurrentDeflection ) );
                        }
                        else
                        {
                            cM = cmZero;
                        }

                        cM *= m_cmMultiplier;
                    }
                    else
                    {
                        cM = cmZero;
                        cM *= m_cmMultiplier;
                    }

                    WP_ASSERT( Math<real_Num>::Abs( totalLift ) < static_cast<real_Num>( 10000.0 ) );
                    WP_ASSERT( Math<real_Num>::Abs( totalDrag ) < static_cast<real_Num>( 10000.0 ) );

                    WP_ASSERT( Math<real_Num>::Abs( cL ) < static_cast<real_Num>( 1000.0 ) );
                    WP_ASSERT( Math<real_Num>::Abs( cD ) < static_cast<real_Num>( 1000.0 ) );
                    WP_ASSERT( Math<real_Num>::Abs( cM ) < static_cast<real_Num>( 1000.0 ) );
                }
                else
                {
                    WP_LOG_ERROR( "No airfoil assigned." );

                    // Fall back to basic l/d equations..
                    // L = cl * a * 0.5f * r * v^2
                    // Approximate Cl using the following formula - Cl = 2 * pi * angle (in radians)
                    real_Num cL = static_cast<real_Num>( 2.0 ) * Math<real_Num>::pi() *
                                  Math<real_Num>::DegToRad( angleOfAttackZero );
                    cL *= clGroundEffectMult;

                    real_Num r = static_cast<real_Num>( 1.29 );
                    real_Num v = relativeWind.length();
                    totalLift = cL * area * static_cast<real_Num>( 0.5 ) * r * ( v * v );

                    static real_Num maxLift = static_cast<real_Num>( 1.0 );
                    // totalLift = MathF::clamp(totalLift, -maxLift, maxLift);

                    // D = 0.5f * cd * r * v2 * a;
                    // Typical aerofoil drag co efficient is .045;
                    real_Num cD = m_cdOverride; // Typical aerofoil drag co efficient
                    cD *= cdGroundEffectMult;

                    totalDrag = static_cast<real_Num>( 0.5 ) * cD * r * ( v * v ) * area;

                    static real_Num maxDrag = static_cast<real_Num>( 1.0 );
                    totalDrag = Math<real_Num>::clamp( totalDrag, -maxDrag, maxDrag );

                    cM = static_cast<real_Num>( 0.0 );

                    WP_ASSERT( cL < static_cast<real_Num>( 1000.0 ) );
                    WP_ASSERT( cD < static_cast<real_Num>( 1000.0 ) );
                    WP_ASSERT( cM < static_cast<real_Num>( 1000.0 ) );
                }

                m_totalLift[sectionIdx] = totalLift;
                m_totalDrag[sectionIdx] = totalDrag;
                m_totalPitch[sectionIdx] = cM;
            }
        }

        void CAircraftWingFast::updateForces()
        {
            auto fDT = 1.0 / 50.0;

            SmartPtr<IVehicleBody> aircraftBody = m_parent;
            auto                   aircraft = m_parentAircraft;

            // const SmartPtr<Transform3<real_Num>>& aircraftTransform = aircraft->getBodyTransform();
            // SmartPtr<Transform3<real_Num>>& worldTransform = getWorldTransform();
            auto localTransform = getLocalTransform();
            // SmartPtr<Transform3<real_Num>>& localBodyTransform = getLocalBodyTransform();
            SmartPtr<IAerodymanicsWind> pWind = aircraft->getWind();

            auto wind = pWind;
            auto attachedControlSurface = m_controlSurface;
            auto propwash = m_propWash;
            auto aerofoil = m_aerofoil;

            const float debugLineScale = 5.0f; // 1.0f / 30.0f;

            s32 lineId = 0;

            auto airDensity = aircraft->getAirDensity();

            auto localWingRootLeadingEdge = m_rootLeadingEdge;
            auto localWingRootTrailingEdge = m_rootTrailingEdge;
            auto localWingTipLeadingEdge = m_tipLeadingEdge;
            auto localWingTipTrailingEdge = m_tipTrailingEdge;
            auto localRootLiftPosition = m_rootLiftPosition;
            auto localTipLiftPosition = m_tipLiftPosition;
            auto liftLineChordPosition = m_liftLineChordPosition;
            auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

            // Per section update.
            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                auto area = m_area[sectionIdx];
                auto chordLine = m_chordLines[sectionIdx];
                auto chordLength = m_chordLengths[sectionIdx];
                auto upFull = m_up[sectionIdx];
                auto aerodynamicCenter = m_aerodynamicCenters[sectionIdx];
                auto relativeWind = m_relativeWind[sectionIdx];
                auto relativeWindLength = relativeWind.length();
                auto relativeWindNormalised = relativeWind.normaliseCopy();
                auto angleOfAttackFull = m_aoa[sectionIdx];
                auto totalLift = m_totalLift[sectionIdx];
                auto totalDrag = m_totalDrag[sectionIdx];
                auto cM = m_totalPitch[sectionIdx];

                auto liftForce = Vector3<real_Num>::zero();
                auto dragForce = Vector3<real_Num>::zero();
                auto pitchAxis = Vector3<real_Num>::zero();

                if( !isControlSurface() )
                {
                    // Build Lift vector.
                    Vector3<real_Num> liftVector =
                        m_localWingRight.crossProduct( -relativeWindNormalised );
                    liftVector.normalise();
                    // WP_ASSERT(liftForce.dotProduct(worldTransform->up()) < real_Num(-0.5));

                    liftForce = liftVector * totalLift;

                    if( aircraft->getDisplayDebugData() )
                    {
                        // if (getName().find("STAB") != std::string::npos)
                        //{
                        //	if (aircraft->getDisplayDebugData())
                        //	{
                        //		DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 9), aerodynamicCenter,
                        //			aerodynamicCenter + (liftForce * debugLineScale), 0xF00000);
                        //	}
                        // }

                        if( getName().find( "WING" ) != std::string::npos )
                        {
                            if( aircraft->getDisplayDebugData() )
                            {
                                DEBUG_DRAW_LOCAL_LINE_BY_ID(
                                    getDebugId( sectionIdx, 9 ), aerodynamicCenter,
                                    aerodynamicCenter + ( liftForce * debugLineScale ), 0x00FF00 );
                            }
                        }

                        // if (getName().find("RUDDER") != std::string::npos)
                        //{
                        //	if (aircraft->getDisplayDebugData())
                        //	{
                        //		DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 9), aerodynamicCenter,
                        //			aerodynamicCenter + (liftForce * debugLineScale), 0x00FF00);
                        //	}
                        // }
                    }

                    // Drag vector.
                    dragForce = relativeWindNormalised * totalDrag;

                    // Debug.DrawLine(aerodynamicCenter, aerodynamicCenter +
                    // (dragForce*Time.deltaTime*debugLineScale), Color.red);

                    // Find wing pitching moment...
                    //  0.5 RoAir*Sqr(Flowpeed)

                    real_Num q = ( static_cast<real_Num>( 0.5 ) * aircraft->getAirDensity() *
                                   ( relativeWind.length() * relativeWind.length() ) );
                    real_Num wingPitchingMoment = cM * chordLength * q * area;
                    // Vector3<real_Num> pitchAxis = chordLine.crossProduct(liftForce.normaliseCopy());

                    auto liftForceNormalised = liftForce.normaliseCopy();
                    pitchAxis = liftForceNormalised.crossProduct( chordLine );
                    pitchAxis.normalise();
                    pitchAxis *= wingPitchingMoment;

                    WP_ASSERT( Math<real_Num>::Abs( liftForce.Z() ) < static_cast<real_Num>( 2000.0 ) );
                    WP_ASSERT( Math<real_Num>::Abs( dragForce.Y() ) < static_cast<real_Num>( 2000.0 ) );

                    // liftForce.X() = real_Num(0.0); // todo check
                    // liftForce.Z() = real_Num(0.0); // todo check
                    // dragForce.Y() = real_Num(0.0); // todo check

                    // WP_ASSERT(localLiftForce.length() < real_Num(2000.0));
                    // WP_ASSERT(localDragForce.length() < real_Num(2000.0));
                    // WP_ASSERT(localAerodynamicCenter.length() < real_Num(2.0));

                    if( aircraft->getEnablePowerUnit() )
                    {
                        aircraftBody->addLocalForceAtLocalPosition( liftForce, aerodynamicCenter );
                        aircraftBody->addLocalForceAtLocalPosition( dragForce, aerodynamicCenter );
                        aircraftBody->addLocalTorque( pitchAxis );
                    }
                }
                else
                {
                    if( attachedControlSurface )
                    {
#if 0
					auto mainWing = boost::static_pointer_cast<CAircraftWingFast>(attachedControlSurface->getMainWing());
					auto liftVector = mainWing->m_localWingRight.crossProduct(-relativeWind.normaliseCopy());
					//Vector3<real_Num> liftForce = relativeWind.normaliseCopy().crossProduct(wingRight);
					liftVector.normalise();
					//WP_ASSERT(liftForce.dotProduct(worldTransform->up()) < real_Num(-0.5));

					liftForce = liftVector * totalLift; // *fVorticity;

					if (aircraft->getDisplayDebugData())
					{
						//if (getName().find("STAB") != std::string::npos)
						//{
						//	if (aircraft->getDisplayDebugData())
						//	{
						//		DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 9), aerodynamicCenter,
						//			aerodynamicCenter + (liftForce * debugLineScale), 0xF00000);
						//	}
						//}

						if (getName().find("WING") != std::string::npos)
						{
							if (aircraft->getDisplayDebugData())
							{
								DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 9), aerodynamicCenter,
									aerodynamicCenter + (liftForce * debugLineScale), 0x00FF00);
							}
						}

						//if (getName().find("RUDDER") != std::string::npos)
						//{
						//	if (aircraft->getDisplayDebugData())
						//	{
						//		DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 9), aerodynamicCenter,
						//			aerodynamicCenter + (liftForce * debugLineScale), 0x00FF00);
						//	}
						//}
					}

					//Vector3<real_Num> localRelativeWind = aircraftTransform->inverseTransformVector(relativeWind);
					//Vector3<real_Num> localRight = localTransform->right();
					//Vector3<real_Num> localLiftForce = localRight.crossProduct(localRelativeWind.normaliseCopy());
					//localLiftForce.normalise();
					//localLiftForce *= totalLift;

					// Drag vector.
					dragForce = relativeWindNormalised * totalDrag;

					//Debug.DrawLine(aerodynamicCenter, aerodynamicCenter + (dragForce*Time.deltaTime*debugLineScale), Color.red);

					//Find wing pitching moment...
					// 0.5 RoAir*Sqr(Flowpeed)
					auto mainWingChordLine = mainWing->m_chordLines[sectionIdx];
					auto mainWingChordLength = mainWing->m_chordLengths[sectionIdx];

					real_Num q = (real_Num(0.5) * airDensity * (relativeWindLength * relativeWindLength));
					real_Num wingPitchingMoment = cM * mainWingChordLength * q * areaFull;

					auto liftForceNormalised = liftForce.normaliseCopy();
					pitchAxis = liftForceNormalised.crossProduct(mainWingChordLine);
					pitchAxis.normalise();
					pitchAxis *= wingPitchingMoment;

					WP_ASSERT(Math<real_Num>::Abs(liftForce.Z()) < real_Num(2000.0));
					WP_ASSERT(Math<real_Num>::Abs(dragForce.Y()) < real_Num(2000.0));
#else
                        auto wing = attachedControlSurface->getMainWing();
                        auto mainWing = workphone::static_pointer_cast<CAircraftWingFast>( wing );
                        if( mainWing )
                        {
                            auto mainWingAerodynamicCenter = mainWing->m_aerodynamicCenters[sectionIdx];

                            // Build Lift vector.
                            Vector3<real_Num> liftVector =
                                m_localWingRight.crossProduct( -relativeWindNormalised );
                            liftVector.normalise();
                            // WP_ASSERT(liftForce.dotProduct(worldTransform->up()) < real_Num(-0.5));

                            liftForce = liftVector * totalLift;

                            if( aircraft->getDisplayDebugData() )
                            {
                                // if (getName().find("STAB") != std::string::npos)
                                //{
                                //	if (aircraft->getDisplayDebugData())
                                //	{
                                //		DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 9),
                                // aerodynamicCenter, 			aerodynamicCenter + (liftForce *
                                // debugLineScale), 0xF00000);
                                //	}
                                // }

                                if( getName().find( "WING" ) != std::string::npos )
                                {
                                    if( aircraft->getDisplayDebugData() )
                                    {
                                        DEBUG_DRAW_LOCAL_LINE_BY_ID(
                                            getDebugId( sectionIdx, 9 ), mainWingAerodynamicCenter,
                                            mainWingAerodynamicCenter + ( liftForce * debugLineScale ),
                                            0x00FF00 );
                                    }
                                }

                                // if (getName().find("RUDDER") != std::string::npos)
                                //{
                                //	if (aircraft->getDisplayDebugData())
                                //	{
                                //		DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(sectionIdx, 9),
                                // aerodynamicCenter, 			aerodynamicCenter + (liftForce *
                                // debugLineScale), 0x00FF00);
                                //	}
                                // }
                            }

                            // Drag vector.
                            dragForce = relativeWindNormalised * totalDrag;

                            // Debug.DrawLine(aerodynamicCenter, aerodynamicCenter +
                            // (dragForce*Time.deltaTime*debugLineScale), Color.red);

                            // Find wing pitching moment...
                            //  0.5 RoAir*Sqr(Flowpeed)

                            real_Num q = ( static_cast<real_Num>( 0.5 ) * aircraft->getAirDensity() *
                                           ( relativeWind.length() * relativeWind.length() ) );
                            real_Num wingPitchingMoment = cM * chordLength * q * area;
                            // Vector3<real_Num> pitchAxis =
                            // chordLine.crossProduct(liftForce.normaliseCopy());

                            auto liftForceNormalised = liftForce.normaliseCopy();
                            pitchAxis = liftForceNormalised.crossProduct( chordLine );
                            pitchAxis.normalise();
                            pitchAxis *= wingPitchingMoment;

                            WP_ASSERT( Math<real_Num>::Abs( liftForce.Z() ) <
                                       static_cast<real_Num>( 2000.0 ) );
                            WP_ASSERT( Math<real_Num>::Abs( dragForce.Y() ) <
                                       static_cast<real_Num>( 2000.0 ) );

                            // liftForce.X() = real_Num(0.0); // todo check
                            // liftForce.Z() = real_Num(0.0); // todo check
                            // dragForce.Y() = real_Num(0.0); // todo check

                            // WP_ASSERT(localLiftForce.length() < real_Num(2000.0));
                            // WP_ASSERT(localDragForce.length() < real_Num(2000.0));
                            // WP_ASSERT(localAerodynamicCenter.length() < real_Num(2.0));
#endif

                            if( aircraft->getEnablePowerUnit() )
                            {
                                aircraftBody->addLocalForceAtLocalPosition( liftForce,
                                                                            mainWingAerodynamicCenter );
                                aircraftBody->addLocalForceAtLocalPosition( dragForce,
                                                                            mainWingAerodynamicCenter );
                                aircraftBody->addLocalTorque( pitchAxis );
                            }
                        }
                    }
                }
            }
        }

        void CAircraftWingFast::updateForcesFast()
        {
            auto airDensity = m_parentAircraft->getAirDensity();

            auto totalLiftForce = Vector3<real_Num>::zero();
            auto totalDragForce = Vector3<real_Num>::zero();
            auto totalPitchAxis = Vector3<real_Num>::zero();
            auto totalAerodynamicCenter = Vector3<real_Num>::zero();

            for( s32 sectionIdx = 0; sectionIdx < m_sectionCount; ++sectionIdx )
            {
                auto area = m_area[sectionIdx];
                auto chordLine = m_chordLines[sectionIdx];
                auto chordLength = m_chordLengths[sectionIdx];

                auto aerodynamicCenter = m_aerodynamicCenters[sectionIdx];
                auto relativeWind = m_relativeWind[sectionIdx];
                auto relativeWindLength = relativeWind.length();
                auto relativeWindNormalised = relativeWind.normaliseCopy();

                auto totalLift = m_totalLift[sectionIdx];
                auto totalDrag = m_totalDrag[sectionIdx];
                auto totalPitch = m_totalPitch[sectionIdx];

                // Build Lift vector.
                auto liftVector = m_localWingRight.crossProduct( -relativeWindNormalised );
                liftVector.normalise();
                // WP_ASSERT(liftForce.dotProduct(worldTransform->up()) < real_Num(-0.5));
                auto liftForce = liftVector * totalLift;

                // Drag vector.
                auto dragForce = relativeWindNormalised * totalDrag;

                // Debug.DrawLine(aerodynamicCenter, aerodynamicCenter +
                // (dragForce*Time.deltaTime*debugLineScale), Color.red);

                // Find wing pitching moment...
                //  0.5 RoAir*Sqr(Flowpeed)
                auto q = ( static_cast<real_Num>( 0.5 ) * airDensity *
                           ( relativeWindLength * relativeWindLength ) );
                auto wingPitchingMoment = totalPitch * chordLength * q * area;
                // Vector3<real_Num> pitchAxis = chordLine.crossProduct(liftForce.normaliseCopy());

                auto liftForceNormalised = liftForce.normaliseCopy();
                auto pitchAxis = liftForceNormalised.crossProduct( chordLine );
                pitchAxis.normalise();
                pitchAxis *= wingPitchingMoment;

                WP_ASSERT( Math<real_Num>::Abs( liftForce.Z() ) < static_cast<real_Num>( 2000.0 ) );
                WP_ASSERT( Math<real_Num>::Abs( dragForce.Y() ) < static_cast<real_Num>( 2000.0 ) );

                totalLiftForce += liftForce;
                totalDragForce += dragForce;
                totalPitchAxis += pitchAxis;

                // if (!isControlSurface())
                //{
                totalAerodynamicCenter += aerodynamicCenter;
                //}
                // else
                //{
                //	if (m_controlSurface)
                //	{
                //		auto mainWing =
                // boost::static_pointer_cast<CAircraftWingFast>(m_controlSurface->getMainWing()); if
                //(mainWing)
                //		{
                //			auto mainWingAerodynamicCenter = mainWing->m_aerodynamicCenters[sectionIdx];
                //			totalAerodynamicCenter += mainWingAerodynamicCenter;
                //		}
                //	}
                //}

                // if (!isControlSurface())
                //{
                //	if (m_parentAircraft->getEnablePowerUnit())
                //	{
                //		m_parent->addLocalForceAtLocalPosition(liftForce, aerodynamicCenter);
                //		m_parent->addLocalForceAtLocalPosition(dragForce, aerodynamicCenter);
                //		m_parent->addLocalTorque(pitchAxis);
                //	}
                // }
                // else
                //{
                //	if (m_controlSurface)
                //	{
                //		auto mainWing =
                // boost::static_pointer_cast<CAircraftWingFast>(m_controlSurface->getMainWing()); if
                //(mainWing)
                //		{
                //			auto mainWingAerodynamicCenter = mainWing->m_aerodynamicCenters[sectionIdx];

                //			if (m_parentAircraft->getEnablePowerUnit())
                //			{
                //				m_parent->addLocalForceAtLocalPosition(liftForce,
                // mainWingAerodynamicCenter); m_parent->addLocalForceAtLocalPosition(dragForce,
                // mainWingAerodynamicCenter); 				m_parent->addLocalTorque(pitchAxis);
                //			}
                //		}
                //	}
                //}
            }

            // totalLiftForce = totalLiftForce / (real_Num)m_sectionCount;
            // totalDragForce = totalDragForce / (real_Num)m_sectionCount;
            // totalPitchAxis = totalPitchAxis / (real_Num)m_sectionCount;
            totalAerodynamicCenter = totalAerodynamicCenter / static_cast<real_Num>( m_sectionCount );

            if( m_parentAircraft->getEnablePowerUnit() )
            {
                m_parent->addLocalForceAtLocalPosition( totalLiftForce, totalAerodynamicCenter );
                m_parent->addLocalForceAtLocalPosition( totalDragForce, totalAerodynamicCenter );
                m_parent->addLocalTorque( totalPitchAxis );
            }
        }

        void CAircraftWingFast::calculateAlphaMinus10()
        {
            for( s32 sectionIdx = 0; sectionIdx < m_sectionCount; ++sectionIdx )
            {
                auto chordLine = m_chordLinesMinus10[sectionIdx];
                auto chordLength = m_chordLengthsMinus10[sectionIdx];

                auto relativeWind = m_relativeWind[sectionIdx];
                auto relativeWindLength = relativeWind.length();
                auto relativeWindNormalised = relativeWind.normaliseCopy();

                auto upFull = m_upMinus10[sectionIdx];

                auto chordLineDotRelWindFull = chordLine.dotProduct( -relativeWindNormalised );
                auto upDotRelWindFull = upFull.dotProduct( -relativeWindNormalised );
                auto angleOfAttackRadsFull =
                    Math<real_Num>::ATan2( upDotRelWindFull, chordLineDotRelWindFull );

                if( isControlSurface() )
                {
                    angleOfAttackRadsFull *= m_aoaMultiplier;
                }
                else
                {
                    if( m_controlSurface )
                    {
                        if( m_controlSurface->isAffectedSection( sectionIdx ) )
                        {
                            angleOfAttackRadsFull *= m_controlSurface->getAoAMultiplier();
                        }
                    }
                }

                auto angleOfAttackFull = Math<real_Num>::RadToDeg( angleOfAttackRadsFull );
                angleOfAttackFull = Math<real_Num>::clamp(
                    angleOfAttackFull, -static_cast<real_Num>( 180.0 ), static_cast<real_Num>( 180.0 ) );
                auto aoaNormalised = Math<real_Num>::clamp(
                    angleOfAttackFull / static_cast<real_Num>( 180.0 ), -1.0, 1.0 );

                m_aoaMinus10[sectionIdx] = angleOfAttackFull;
            }
        }

        void CAircraftWingFast::calculateAlpha10()
        {
            for( s32 sectionIdx = 0; sectionIdx < m_sectionCount; ++sectionIdx )
            {
                auto chordLine = m_chordLinesPlus10[sectionIdx];
                auto chordLength = m_chordLengthsPlus10[sectionIdx];

                auto relativeWind = m_relativeWind[sectionIdx];
                auto relativeWindLength = relativeWind.length();
                auto relativeWindNormalised = relativeWind.normaliseCopy();

                auto upFull = m_upPlus10[sectionIdx];

                auto chordLineDotRelWindFull = chordLine.dotProduct( -relativeWindNormalised );
                auto upDotRelWindFull = upFull.dotProduct( -relativeWindNormalised );
                auto angleOfAttackRadsFull =
                    Math<real_Num>::ATan2( upDotRelWindFull, chordLineDotRelWindFull );

                if( isControlSurface() )
                {
                    angleOfAttackRadsFull *= m_aoaMultiplier;
                }
                else
                {
                    if( m_controlSurface )
                    {
                        if( m_controlSurface->isAffectedSection( sectionIdx ) )
                        {
                            angleOfAttackRadsFull *= m_controlSurface->getAoAMultiplier();
                        }
                    }
                }

                auto angleOfAttackFull = Math<real_Num>::RadToDeg( angleOfAttackRadsFull );
                angleOfAttackFull = Math<real_Num>::clamp(
                    angleOfAttackFull, -static_cast<real_Num>( 180.0 ), static_cast<real_Num>( 180.0 ) );
                auto aoaNormalised = Math<real_Num>::clamp(
                    angleOfAttackFull / static_cast<real_Num>( 180.0 ), -1.0, 1.0 );

                m_aoaPlus10[sectionIdx] = angleOfAttackFull;
            }
        }

        void CAircraftWingFast::calculateAlpha()
        {
            for( s32 sectionIdx = 0; sectionIdx < m_sectionCount; ++sectionIdx )
            {
                auto chordLine = m_chordLines[sectionIdx];
                auto chordLength = m_chordLengths[sectionIdx];

                auto relativeWind = m_relativeWind[sectionIdx];
                auto relativeWindLength = relativeWind.length();
                auto relativeWindNormalised = relativeWind.normaliseCopy();

                auto upFull = m_up[sectionIdx];

                auto chordLineDotRelWindFull = chordLine.dotProduct( -relativeWindNormalised );
                auto upDotRelWindFull = upFull.dotProduct( -relativeWindNormalised );
                auto angleOfAttackRadsFull =
                    Math<real_Num>::ATan2( upDotRelWindFull, chordLineDotRelWindFull );

                if( isControlSurface() )
                {
                    angleOfAttackRadsFull *= m_aoaMultiplier;
                }
                else
                {
                    if( m_controlSurface )
                    {
                        if( m_controlSurface->isAffectedSection( sectionIdx ) )
                        {
                            angleOfAttackRadsFull *= m_controlSurface->getAoAMultiplier();
                        }
                    }
                }

                auto angleOfAttackFull = Math<real_Num>::RadToDeg( angleOfAttackRadsFull );
                angleOfAttackFull = Math<real_Num>::clamp(
                    angleOfAttackFull, -static_cast<real_Num>( 180.0 ), static_cast<real_Num>( 180.0 ) );
                auto aoaNormalised = Math<real_Num>::clamp(
                    angleOfAttackFull / static_cast<real_Num>( 180.0 ), -1.0, 1.0 );

                m_aoa[sectionIdx] = angleOfAttackFull;
            }
        }

        void CAircraftWingFast::calculateAera()
        {
            auto sectionCount = m_sectionCount;
            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                const auto a = m_modifiedSectionA[sectionIdx];
                const auto b = m_modifiedSectionB[sectionIdx];
                const auto c = m_modifiedSectionC[sectionIdx];
                const auto d = m_modifiedSectionD[sectionIdx];

                m_area[sectionIdx] = calculateArea( a, b, c, d );
            }
        }

        void CAircraftWingFast::calculateAeroProperties()
        {
            SmartPtr<IVehicleBody> aircraftBody = m_parent;
            auto                   aircraft = m_parentAircraft;

            // const SmartPtr<Transform3<real_Num>>& aircraftTransform = aircraft->getBodyTransform();
            // SmartPtr<Transform3<real_Num>>& worldTransform = getWorldTransform();
            auto localTransform = getLocalTransform();
            // SmartPtr<Transform3<real_Num>>& localBodyTransform = getLocalBodyTransform();
            SmartPtr<IAerodymanicsWind> pWind = aircraft->getWind();

            auto wind = pWind;
            auto attachedControlSurface = m_controlSurface;
            auto propwash = m_propWash;
            auto aerofoil = m_aerofoil;

            const float debugLineScale = 5.0f; // 1.0f / 30.0f;

            s32 lineId = 0;

            auto airDensity = aircraft->getAirDensity();

            auto localWingRootLeadingEdge = m_rootLeadingEdge;
            auto localWingRootTrailingEdge = m_rootTrailingEdge;
            auto localWingTipLeadingEdge = m_tipLeadingEdge;
            auto localWingTipTrailingEdge = m_tipTrailingEdge;
            auto localRootLiftPosition = m_rootLiftPosition;
            auto localTipLiftPosition = m_tipLiftPosition;
            auto liftLineChordPosition = m_liftLineChordPosition;
            auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                auto a = m_modifiedSectionA[sectionIdx];
                auto b = m_modifiedSectionB[sectionIdx];
                auto c = m_modifiedSectionC[sectionIdx];
                auto d = m_modifiedSectionD[sectionIdx];

                Vector3<real_Num> sectionRootLiftPositionFull =
                    d + ( ( a - d ) * liftLineChordPosition );
                Vector3<real_Num> sectionTipLiftPositionFull = c + ( ( b - c ) * liftLineChordPosition );

                auto chordLine = ( a + ( ( b - a ) * static_cast<real_Num>( 0.5 ) ) ) -
                                 ( d + ( ( c - d ) * static_cast<real_Num>( 0.5 ) ) );
                auto chordLength = chordLine.length();
                chordLine.normalise();

                Vector3<real_Num> upFull = chordLine.crossProduct(
                    ( sectionTipLiftPositionFull - sectionRootLiftPositionFull ).normaliseCopy() );
                // Vector3<real_Num> up = (sectionTipLiftPosition -
                // sectionRootLiftPosition).normaliseCopy().crossProduct(chordLine);
                upFull.normalise();

                Vector3<real_Num> localScaleFull = localTransform.getScale();
                if( localScaleFull.X() < static_cast<real_Num>( 0.0 ) )
                {
                    upFull = -upFull;
                }

                // Find the aerodynamic center.
                Vector3<real_Num> aerodynamicCenter =
                    sectionRootLiftPositionFull +
                    ( ( sectionTipLiftPositionFull - sectionRootLiftPositionFull ) *
                      static_cast<real_Num>( 0.5 ) );

                m_chordLines[sectionIdx] = chordLine;
                m_chordLengths[sectionIdx] = chordLength;
                m_up[sectionIdx] = upFull;
                m_aerodynamicCenters[sectionIdx] = aerodynamicCenter;
            }
        }

        void CAircraftWingFast::calculateAeroProperties10()
        {
            SmartPtr<IVehicleBody> aircraftBody = m_parent;
            auto                   aircraft = m_parentAircraft;

            // const SmartPtr<Transform3<real_Num>>& aircraftTransform = aircraft->getBodyTransform();
            // SmartPtr<Transform3<real_Num>>& worldTransform = getWorldTransform();
            auto localTransform = getLocalTransform();
            // SmartPtr<Transform3<real_Num>>& localBodyTransform = getLocalBodyTransform();
            SmartPtr<IAerodymanicsWind> pWind = aircraft->getWind();

            auto wind = pWind;
            auto attachedControlSurface = m_controlSurface;
            auto propwash = m_propWash;
            auto aerofoil = m_aerofoil;

            const float debugLineScale = 5.0f; // 1.0f / 30.0f;

            s32 lineId = 0;

            auto airDensity = aircraft->getAirDensity();

            auto localWingRootLeadingEdge = m_rootLeadingEdge;
            auto localWingRootTrailingEdge = m_rootTrailingEdge;
            auto localWingTipLeadingEdge = m_tipLeadingEdge;
            auto localWingTipTrailingEdge = m_tipTrailingEdge;
            auto localRootLiftPosition = m_rootLiftPosition;
            auto localTipLiftPosition = m_tipLiftPosition;
            auto liftLineChordPosition = m_liftLineChordPosition;
            auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                auto a = m_modifiedSectionA10[sectionIdx];
                auto b = m_modifiedSectionB10[sectionIdx];
                auto c = m_modifiedSectionC10[sectionIdx];
                auto d = m_modifiedSectionD10[sectionIdx];

                Vector3<real_Num> sectionRootLiftPositionFull =
                    d + ( ( a - d ) * liftLineChordPosition );
                Vector3<real_Num> sectionTipLiftPositionFull = c + ( ( b - c ) * liftLineChordPosition );

                auto chordLine = ( a + ( ( b - a ) * static_cast<real_Num>( 0.5 ) ) ) -
                                 ( d + ( ( c - d ) * static_cast<real_Num>( 0.5 ) ) );
                auto chordLength = chordLine.length();
                chordLine.normalise();

                Vector3<real_Num> upFull = chordLine.crossProduct(
                    ( sectionTipLiftPositionFull - sectionRootLiftPositionFull ).normaliseCopy() );
                // Vector3<real_Num> up = (sectionTipLiftPosition -
                // sectionRootLiftPosition).normaliseCopy().crossProduct(chordLine);
                upFull.normalise();

                Vector3<real_Num> localScaleFull = localTransform.getScale();
                if( localScaleFull.X() < static_cast<real_Num>( 0.0 ) )
                {
                    upFull = -upFull;
                }

                ////Find the aerodynamic center.
                Vector3<real_Num> aerodynamicCenter =
                    sectionRootLiftPositionFull +
                    ( ( sectionTipLiftPositionFull - sectionRootLiftPositionFull ) *
                      static_cast<real_Num>( 0.5 ) );

                m_chordLinesPlus10[sectionIdx] = chordLine;
                m_chordLengthsPlus10[sectionIdx] = chordLength;
                m_upPlus10[sectionIdx] = upFull;
                m_aerodynamicCentersPlus10[sectionIdx] = aerodynamicCenter;
            }
        }

        void CAircraftWingFast::calculateAeroPropertiesMinus10()
        {
            SmartPtr<IVehicleBody> aircraftBody = m_parent;
            auto                   aircraft = m_parentAircraft;

            // const SmartPtr<Transform3<real_Num>>& aircraftTransform = aircraft->getBodyTransform();
            // SmartPtr<Transform3<real_Num>>& worldTransform = getWorldTransform();
            auto localTransform = getLocalTransform();
            // SmartPtr<Transform3<real_Num>>& localBodyTransform = getLocalBodyTransform();
            SmartPtr<IAerodymanicsWind> pWind = aircraft->getWind();

            auto wind = pWind;
            auto attachedControlSurface = m_controlSurface;
            auto propwash = m_propWash;
            auto aerofoil = m_aerofoil;

            const float debugLineScale = 5.0f; // 1.0f / 30.0f;

            s32 lineId = 0;

            auto airDensity = aircraft->getAirDensity();

            auto localWingRootLeadingEdge = m_rootLeadingEdge;
            auto localWingRootTrailingEdge = m_rootTrailingEdge;
            auto localWingTipLeadingEdge = m_tipLeadingEdge;
            auto localWingTipTrailingEdge = m_tipTrailingEdge;
            auto localRootLiftPosition = m_rootLiftPosition;
            auto localTipLiftPosition = m_tipLiftPosition;
            auto liftLineChordPosition = m_liftLineChordPosition;
            auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                auto a = m_modifiedSectionAMinus10[sectionIdx];
                auto b = m_modifiedSectionBMinus10[sectionIdx];
                auto c = m_modifiedSectionCMinus10[sectionIdx];
                auto d = m_modifiedSectionDMinus10[sectionIdx];

                Vector3<real_Num> sectionRootLiftPositionFull =
                    d + ( ( a - d ) * liftLineChordPosition );
                Vector3<real_Num> sectionTipLiftPositionFull = c + ( ( b - c ) * liftLineChordPosition );

                auto chordLine = ( a + ( ( b - a ) * static_cast<real_Num>( 0.5 ) ) ) -
                                 ( d + ( ( c - d ) * static_cast<real_Num>( 0.5 ) ) );
                auto chordLength = chordLine.length();
                chordLine.normalise();

                Vector3<real_Num> upFull = chordLine.crossProduct(
                    ( sectionTipLiftPositionFull - sectionRootLiftPositionFull ).normaliseCopy() );
                // Vector3<real_Num> up = (sectionTipLiftPosition -
                // sectionRootLiftPosition).normaliseCopy().crossProduct(chordLine);
                upFull.normalise();

                Vector3<real_Num> localScaleFull = localTransform.getScale();
                if( localScaleFull.X() < static_cast<real_Num>( 0.0 ) )
                {
                    upFull = -upFull;
                }

                ////Find the aerodynamic center.
                Vector3<real_Num> aerodynamicCenter =
                    sectionRootLiftPositionFull +
                    ( ( sectionTipLiftPositionFull - sectionRootLiftPositionFull ) *
                      static_cast<real_Num>( 0.5 ) );

                m_chordLinesMinus10[sectionIdx] = chordLine;
                m_chordLengthsMinus10[sectionIdx] = chordLength;
                m_upMinus10[sectionIdx] = upFull;
                m_aerodynamicCentersMinus10[sectionIdx] = aerodynamicCenter;
            }
        }

        void CAircraftWingFast::calculateModifiedSections()
        {
            auto aircraft = m_parentAircraft;
            auto attachedControlSurface = m_controlSurface;
            auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                auto a = m_sectionA[sectionIdx];
                auto b = m_sectionB[sectionIdx];
                auto c = m_sectionC[sectionIdx];
                auto d = m_sectionD[sectionIdx];

                if( attachedControlSurface )
                {
                    if( isControlSurface() )
                    {
                        attachedControlSurface->modifyWingGeometry( sectionIdx, a, b, c, d );
                    }
                }

                m_modifiedSectionA[sectionIdx] = a;
                m_modifiedSectionB[sectionIdx] = b;
                m_modifiedSectionC[sectionIdx] = c;
                m_modifiedSectionD[sectionIdx] = d;
            }
        }

        void CAircraftWingFast::calculateModifiedSections10()
        {
            auto aircraft = m_parentAircraft;
            auto attachedControlSurface = m_controlSurface;
            auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                auto a = m_sectionA[sectionIdx];
                auto b = m_sectionB[sectionIdx];
                auto c = m_sectionC[sectionIdx];
                auto d = m_sectionD[sectionIdx];

                if( attachedControlSurface )
                {
                    if( isControlSurface() )
                    {
                        // auto angle = attachedControlSurface->isReversed() ? real_Num(-0.1) :
                        // real_Num(0.1);
                        auto angle = static_cast<real_Num>( 0.1 );
                        attachedControlSurface->modifyWingGeometry( sectionIdx, a, b, c, d, angle );
                    }
                    else
                    {
                        attachedControlSurface->modifyMainWingGeometry( sectionIdx, a, b, c, d,
                                                                        static_cast<real_Num>( 0.1 ) );
                    }
                }

                m_modifiedSectionA10[sectionIdx] = a;
                m_modifiedSectionB10[sectionIdx] = b;
                m_modifiedSectionC10[sectionIdx] = c;
                m_modifiedSectionD10[sectionIdx] = d;
            }
        }

        void CAircraftWingFast::calculateModifiedSectionsMinus10()
        {
            auto aircraft = m_parentAircraft;
            auto attachedControlSurface = m_controlSurface;
            auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                auto a = m_sectionA[sectionIdx];
                auto b = m_sectionB[sectionIdx];
                auto c = m_sectionC[sectionIdx];
                auto d = m_sectionD[sectionIdx];

                if( attachedControlSurface )
                {
                    if( isControlSurface() )
                    {
                        // auto angle = attachedControlSurface->isReversed() ? real_Num(0.1) :
                        // real_Num(-0.1);
                        auto angle = static_cast<real_Num>( -0.1 );
                        attachedControlSurface->modifyWingGeometry( sectionIdx, a, b, c, d, angle );
                    }
                    else
                    {
                        attachedControlSurface->modifyMainWingGeometry( sectionIdx, a, b, c, d,
                                                                        static_cast<real_Num>( -0.1 ) );
                    }
                }

                m_modifiedSectionAMinus10[sectionIdx] = a;
                m_modifiedSectionBMinus10[sectionIdx] = b;
                m_modifiedSectionCMinus10[sectionIdx] = c;
                m_modifiedSectionDMinus10[sectionIdx] = d;
            }
        }

        void CAircraftWingFast::updatePoints()
        {
            auto aircraft = m_parentAircraft;

            auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                // R A-----------------B (Leading edge)
                // O |                 |
                // O |                 |
                // T D-----------------C (Trailing edge
                auto localWingRootLeadingEdge = m_rootLeadingEdge;
                auto localWingRootTrailingEdge = m_rootTrailingEdge;
                auto localWingTipLeadingEdge = m_tipLeadingEdge;
                auto localWingTipTrailingEdge = m_tipTrailingEdge;
                auto localRootLiftPosition = m_rootLiftPosition;
                auto localTipLiftPosition = m_tipLiftPosition;
                auto liftLineChordPosition = m_liftLineChordPosition;

                auto rootDistance = static_cast<real_Num>( 0.0 );
                auto tipDistance = static_cast<real_Num>( 0.0 );

                auto attachedControlSurface = getAttachedControlSurface();
                if( attachedControlSurface )
                {
                    if( attachedControlSurface->isAffectedSection( sectionIdx ) )
                    {
                        if( !isControlSurface() )
                        {
                            rootDistance =
                                attachedControlSurface->getRootHingeDistanceFromTrailingEdge();
                            tipDistance = attachedControlSurface->getTipHingeDistanceFromTrailingEdge();

                            localWingRootTrailingEdge =
                                m_rootTrailingEdge +
                                ( m_rootLeadingEdge - m_rootTrailingEdge ) * rootDistance;
                            localWingTipTrailingEdge =
                                m_tipTrailingEdge +
                                ( m_tipLeadingEdge - m_tipTrailingEdge ) * tipDistance;
                        }
                        else
                        {
                            auto wing = attachedControlSurface->getMainWing();
                            auto mainWing = workphone::static_pointer_cast<CAircraftWingFast>( wing );
                            if( mainWing )
                            {
                                localWingRootTrailingEdge = mainWing->m_rootTrailingEdge;
                                localWingTipTrailingEdge = mainWing->m_tipTrailingEdge;
                            }
                        }
                    }
                }

                // Find points a,b,c & d for this chunk of wing.
                const Vector3<real_Num> sectionA =
                    localWingRootLeadingEdge + ( ( localWingTipLeadingEdge - localWingRootLeadingEdge ) *
                                                 static_cast<real_Num>( sectionIdx ) / sectionCount );
                const Vector3<real_Num> sectionB =
                    localWingRootLeadingEdge +
                    ( ( localWingTipLeadingEdge - localWingRootLeadingEdge ) *
                      static_cast<real_Num>( sectionIdx + 1 ) / sectionCount );
                const Vector3<real_Num> sectionC =
                    localWingRootTrailingEdge +
                    ( ( localWingTipTrailingEdge - localWingRootTrailingEdge ) *
                      static_cast<real_Num>( sectionIdx + 1 ) / sectionCount );
                const Vector3<real_Num> sectionD =
                    localWingRootTrailingEdge +
                    ( ( localWingTipTrailingEdge - localWingRootTrailingEdge ) *
                      static_cast<real_Num>( sectionIdx ) / sectionCount );

                m_sectionA[sectionIdx] = sectionA;
                m_sectionB[sectionIdx] = sectionB;
                m_sectionC[sectionIdx] = sectionC;
                m_sectionD[sectionIdx] = sectionD;
            }
        }

        void CAircraftWingFast::updateVisualPoints()
        {
            auto aircraft = m_parentAircraft;

            auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                // R A-----------------B (Leading edge)
                // O |                 |
                // O |                 |
                // T D-----------------C (Trailing edge
                auto localWingRootLeadingEdge = m_rootLeadingEdge;
                auto localWingRootTrailingEdge = m_rootTrailingEdge;
                auto localWingTipLeadingEdge = m_tipLeadingEdge;
                auto localWingTipTrailingEdge = m_tipTrailingEdge;
                auto localRootLiftPosition = m_rootLiftPosition;
                auto localTipLiftPosition = m_tipLiftPosition;
                auto liftLineChordPosition = m_liftLineChordPosition;

                auto rootDistance = static_cast<real_Num>( 0.0 );
                auto tipDistance = static_cast<real_Num>( 0.0 );

                auto attachedControlSurface = getAttachedControlSurface();
                if( attachedControlSurface )
                {
                    if( attachedControlSurface->isAffectedSection( sectionIdx ) )
                    {
                        if( !isControlSurface() )
                        {
                            rootDistance =
                                attachedControlSurface->getRootHingeDistanceFromTrailingEdge();
                            tipDistance = attachedControlSurface->getTipHingeDistanceFromTrailingEdge();

                            localWingRootTrailingEdge =
                                m_rootTrailingEdge +
                                ( m_rootLeadingEdge - m_rootTrailingEdge ) * rootDistance;
                            localWingTipTrailingEdge =
                                m_tipTrailingEdge +
                                ( m_tipLeadingEdge - m_tipTrailingEdge ) * tipDistance;
                        }
                        else
                        {
                            auto wing = attachedControlSurface->getMainWing();
                            auto mainWing = workphone::static_pointer_cast<CAircraftWingFast>( wing );
                            if( mainWing )
                            {
                                localWingRootTrailingEdge = mainWing->m_rootTrailingEdge;
                                localWingTipTrailingEdge = mainWing->m_tipTrailingEdge;
                            }
                        }
                    }
                }

                // Find points a,b,c & d for this chunk of wing.
                const Vector3<real_Num> sectionA =
                    localWingRootLeadingEdge + ( ( localWingTipLeadingEdge - localWingRootLeadingEdge ) *
                                                 static_cast<real_Num>( sectionIdx ) / sectionCount );
                const Vector3<real_Num> sectionB =
                    localWingRootLeadingEdge +
                    ( ( localWingTipLeadingEdge - localWingRootLeadingEdge ) *
                      static_cast<real_Num>( sectionIdx + 1 ) / sectionCount );
                const Vector3<real_Num> sectionC =
                    localWingRootTrailingEdge +
                    ( ( localWingTipTrailingEdge - localWingRootTrailingEdge ) *
                      static_cast<real_Num>( sectionIdx + 1 ) / sectionCount );
                const Vector3<real_Num> sectionD =
                    localWingRootTrailingEdge +
                    ( ( localWingTipTrailingEdge - localWingRootTrailingEdge ) *
                      static_cast<real_Num>( sectionIdx ) / sectionCount );

                m_visualSectionA[sectionIdx] = sectionA;
                m_visualSectionB[sectionIdx] = sectionB;
                m_visualSectionC[sectionIdx] = sectionC;
                m_visualSectionD[sectionIdx] = sectionD;
            }
        }

        void CAircraftWingFast::updateCombined( const double &t, const double &fDT )
        {
            SmartPtr<IVehicleBody> aircraftBody = m_parent;
            auto                   aircraft = m_parentAircraft;

            // const SmartPtr<Transform3<real_Num>>& aircraftTransform = aircraft->getBodyTransform();
            // SmartPtr<Transform3<real_Num>>& worldTransform = getWorldTransform();
            auto localTransform = getLocalTransform();
            // SmartPtr<Transform3<real_Num>>& localBodyTransform = getLocalBodyTransform();
            SmartPtr<IAerodymanicsWind> pWind = aircraft->getWind();

            auto wind = pWind;
            auto attachedControlSurface = m_controlSurface;
            auto propwash = m_propWash;
            auto aerofoil = m_aerofoil;

            float debugLineScale = 5.0f; // 1.0f / 30.0f;

            s32 lineId = 0;

            auto localWingRootLeadingEdge = m_rootLeadingEdge;
            auto localWingRootTrailingEdge = m_rootTrailingEdge;
            auto localWingTipLeadingEdge = m_tipLeadingEdge;
            auto localWingTipTrailingEdge = m_tipTrailingEdge;
            auto localRootLiftPosition = m_rootLiftPosition;
            auto localTipLiftPosition = m_tipLiftPosition;
            auto liftLineChordPosition = m_liftLineChordPosition;
            auto sectionCount = m_sectionCount; // *aircraft->getSectionMultiplier();

            // m_windRotation.X() += 100.0 * fDT;
            // m_windRotation.X() = Math<real_Num>::wrap(m_windRotation.X(), -25.0, 25.0);

            static real_Num angle = 45.0;
            m_windRotation.X() = angle;

            DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( 11 ), m_tipLiftPosition,
                                         m_tipLiftPosition + ( m_localWingRight * 0.1 ), 0x000000 );

            // Per section update.
            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                // R A-----------------B (Leading edge)
                // O |                 |
                // O |                 |
                // T D-----------------C (Trailing edge

                // Find points a,b,c & d for this chunk of wing.
                const Vector3<real_Num> sectionA =
                    localWingRootLeadingEdge +
                    ( ( localWingTipLeadingEdge - localWingRootLeadingEdge ) *
                      static_cast<real_Num>( sectionIdx ) / static_cast<real_Num>( sectionCount ) );
                const Vector3<real_Num> sectionB =
                    localWingRootLeadingEdge +
                    ( ( localWingTipLeadingEdge - localWingRootLeadingEdge ) *
                      static_cast<real_Num>( sectionIdx + 1 ) / static_cast<real_Num>( sectionCount ) );
                const Vector3<real_Num> sectionC =
                    localWingRootTrailingEdge +
                    ( ( localWingTipTrailingEdge - localWingRootTrailingEdge ) *
                      static_cast<real_Num>( sectionIdx + 1 ) / static_cast<real_Num>( sectionCount ) );
                const Vector3<real_Num> sectionD =
                    localWingRootTrailingEdge +
                    ( ( localWingTipTrailingEdge - localWingRootTrailingEdge ) *
                      static_cast<real_Num>( sectionIdx ) / static_cast<real_Num>( sectionCount ) );

                auto a = sectionA;
                auto b = sectionB;
                auto c = sectionC;
                auto d = sectionD;

                //////Draw premodified wing.
                // DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 0), a, b);
                // DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 1), b, c);
                // DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 2), c, d);
                // DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 3), d, a);

                // if (getName() == "WING LOWER LEFT")
                //{
                //	int stop = 0;
                //	stop = 0;
                // }
                real_Num          areaZero = calculateArea( a, b, c, d );
                Vector3<real_Num> sectionRootLiftPositionZero =
                    d + ( ( a - d ) * liftLineChordPosition );
                Vector3<real_Num> sectionTipLiftPositionZero = c + ( ( b - c ) * liftLineChordPosition );

                Vector3<real_Num> chordLineZero = ( a + ( ( b - a ) * static_cast<real_Num>( 0.5 ) ) ) -
                                                  ( d + ( ( c - d ) * static_cast<real_Num>( 0.5 ) ) );
                real_Num          chordLengthZero = chordLineZero.length();
                chordLineZero.normalise();

                Vector3<real_Num> upZero = chordLineZero.crossProduct(
                    ( sectionTipLiftPositionZero - sectionRootLiftPositionZero ).normaliseCopy() );
                // Vector3<real_Num> up = (sectionTipLiftPosition -
                // sectionRootLiftPosition).normaliseCopy().crossProduct(chordLine);
                upZero.normalise();

                Vector3<real_Num> localScaleZero = localTransform.getScale();
                if( localScaleZero.X() < static_cast<real_Num>( 0.0 ) )
                {
                    upZero = -upZero;
                }

                // Find the aerodynamic center.
                Vector3<real_Num> aerodynamicCenter =
                    sectionRootLiftPositionZero +
                    ( ( sectionTipLiftPositionZero - sectionRootLiftPositionZero ) *
                      static_cast<real_Num>( 0.5 ) );

                if( attachedControlSurface )
                {
                    attachedControlSurface->modifyWingGeometry( sectionIdx, a, b, c, d );
                }

                real_Num          areaFull = calculateArea( a, b, c, d );
                Vector3<real_Num> sectionRootLiftPositionFull =
                    d + ( ( a - d ) * liftLineChordPosition );
                Vector3<real_Num> sectionTipLiftPositionFull = c + ( ( b - c ) * liftLineChordPosition );

                Vector3<real_Num> chordLineFull = ( a + ( ( b - a ) * static_cast<real_Num>( 0.5 ) ) ) -
                                                  ( d + ( ( c - d ) * static_cast<real_Num>( 0.5 ) ) );
                chordLineFull.normalise();

                Vector3<real_Num> upFull = chordLineFull.crossProduct(
                    ( sectionTipLiftPositionFull - sectionRootLiftPositionFull ).normaliseCopy() );
                // Vector3<real_Num> up = (sectionTipLiftPosition -
                // sectionRootLiftPosition).normaliseCopy().crossProduct(chordLine);
                upFull.normalise();

                Vector3<real_Num> localScaleFull = localTransform.getScale();
                if( localScaleFull.X() < static_cast<real_Num>( 0.0 ) )
                {
                    upFull = -upFull;
                }

                //////Draw modified wing.
                if( aircraft->getDisplayDebugData() )
                {
                    DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 4 ), a, b, 0x00FF00 );
                    DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 5 ), b, c, 0xFF0000 );
                    DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 6 ), c, d, 0x0000FF );
                    DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 7 ), d, a, 0x00FFFF );
                }

                a = sectionA;
                b = sectionB;
                c = sectionC;
                d = sectionD;

                // If we have a control surface attached update the geometry based on control surface
                // inputs.
                if( attachedControlSurface )
                {
                    attachedControlSurface->modifyWingGeometry( sectionIdx, a, b, c, d,
                                                                static_cast<real_Num>( 0.1 ) );
                }

                real_Num area10 = calculateArea( a, b, c, d );

                // WP_ASSERT(Math<real_Num>::equals(area, area2));
                // if (aircraft->getDisplayDebugData())
                //{
                //	DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(i, 14), a, b, 0x00FF00);
                //	DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(i, 15), b, c, 0xFF0000);
                //	DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(i, 16), c, d, 0x0000FF);
                //	DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(i, 17), d, a, 0x00FFFF);
                // }

                // Recalculate lift positions..
                Vector3<real_Num> sectionRootLiftPosition10 = d + ( ( a - d ) * liftLineChordPosition );
                Vector3<real_Num> sectionTipLiftPosition10 = c + ( ( b - c ) * liftLineChordPosition );

                // Find the chord line.
                Vector3<real_Num> chordLine10 = ( a + ( ( b - a ) * static_cast<real_Num>( 0.5 ) ) ) -
                                                ( d + ( ( c - d ) * static_cast<real_Num>( 0.5 ) ) );
                real_Num          chordLength10 = chordLine10.length();
                chordLine10.normalise();

                // aerodynamicCenter += chordLine * chordLength * real_Num(0.5);

                // if (aircraft->getDisplayDebugData())
                //{
                //	if (getName().find("WING") != std::string::npos)
                //	{
                //		DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(i, 8), aerodynamicCenter,
                // aerodynamicCenter + chordLine, 			0x0000FF);
                //	}
                // }

                if( aircraft->getDisplayDebugData() )
                {
                    if( getName().find( "WING" ) != std::string::npos )
                    {
                        DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 8 ), aerodynamicCenter,
                                                     aerodynamicCenter + chordLineFull, 0x0000FF );
                    }
                }

                Vector3<real_Num> windVector = wind->getWind( aerodynamicCenter.Y(), t );
                Vector3<real_Num> relativeWind =
                    calculateRelativeWind( sectionIdx, aerodynamicCenter, windVector );

                if( !aircraft->getEnablePowerUnit() )
                {
                    static auto w = Vector3<real_Num>( 0, 0, -5 );

                    Quaternion<real_Num> q;
                    q.fromDegrees( m_windRotation );

                    relativeWind = q * w;
                    // relativeWind.normalise();

                    // relativeWind = aircraft->getBodyTransform()->inverseTransformVector(relativeWind);
                }

                // const double AirViscosity = 18.325E-6;
                // m_re[i] = aircraft->getAirDensity() * relativeWind.length() * chordLength /
                // AirViscosity;

                if( aircraft->getDisplayDebugData() )
                {
                    if( getName().find( "STAB" ) != std::string::npos )
                    {
                        DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 14 ), aerodynamicCenter,
                                                     aerodynamicCenter + ( relativeWind * 1.0f ),
                                                     0xFF0000 );
                    }

                    if( getName().find( "WING" ) != std::string::npos )
                    {
                        DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 30 ), aerodynamicCenter,
                                                     aerodynamicCenter + ( relativeWind * 1.0f ),
                                                     0xFF0000 );
                    }
                }

                // Vector3<real_Num> localChordLine =
                // aircraftTransform->inverseTransformVector(chordLine); Vector3<real_Num>
                // localRelativeWind = aircraftTransform->inverseTransformVector(relativeWind);

                // localChordLine = localChordLine.normaliseCopy();
                // localRelativeWind = localRelativeWind.normaliseCopy();

                Vector3<real_Num> up10 = chordLine10.crossProduct(
                    ( sectionTipLiftPosition10 - sectionRootLiftPosition10 ).normaliseCopy() );
                // Vector3<real_Num> up = (sectionTipLiftPosition -
                // sectionRootLiftPosition).normaliseCopy().crossProduct(chordLine);
                up10.normalise();

                // WP_ASSERT(up.Y() > real_Num(0.0));

                Vector3<real_Num> localScale = localTransform.getScale();
                if( localScale.X() < static_cast<real_Num>( 0.0 ) )
                {
                    up10 = -up10;
                }

                if( aircraft->getDisplayDebugData() )
                {
                    if( getName().find( "WING" ) != std::string::npos )
                    {
                        DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 16 ), aerodynamicCenter,
                                                     aerodynamicCenter + ( upFull * 1.0f ), 0xFFFF00 );
                    }

                    if( getName().find( "STAB" ) != std::string::npos )
                    {
                        DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 16 ), aerodynamicCenter,
                                                     aerodynamicCenter + ( upFull * 1.0f ), 0xFFFFFF );
                    }
                }

                // Find the angle of attack.
                auto relativeWindNormalised = relativeWind.normaliseCopy();

                auto chordLineDotRelWind10 = chordLine10.dotProduct( -relativeWindNormalised );
                auto upDotRelWind10 = up10.dotProduct( -relativeWindNormalised );
                auto angleOfAttackRads10 =
                    Math<real_Num>::ATan2( upDotRelWind10, chordLineDotRelWind10 );
                angleOfAttackRads10 *= m_aoaMultiplier;
                auto angleOfAttack10 = Math<real_Num>::RadToDeg( angleOfAttackRads10 );
                angleOfAttack10 = Math<real_Num>::clamp(
                    angleOfAttack10, -static_cast<real_Num>( 180.0 ), static_cast<real_Num>( 180.0 ) );

                auto chordLineDotRelWindZero = chordLineZero.dotProduct( -relativeWindNormalised );
                auto upDotRelWindZero = upZero.dotProduct( -relativeWindNormalised );
                auto angleOfAttackRadsZero =
                    Math<real_Num>::ATan2( upDotRelWindZero, chordLineDotRelWindZero );
                angleOfAttackRadsZero *= m_aoaMultiplier;
                auto angleOfAttackZero = Math<real_Num>::RadToDeg( angleOfAttackRadsZero );
                angleOfAttackZero = Math<real_Num>::clamp(
                    angleOfAttackZero, -static_cast<real_Num>( 180.0 ), static_cast<real_Num>( 180.0 ) );

                auto chordLineDotRelWindFull = chordLineFull.dotProduct( -relativeWindNormalised );
                auto upDotRelWindFull = upFull.dotProduct( -relativeWindNormalised );
                auto angleOfAttackRadsFull =
                    Math<real_Num>::ATan2( upDotRelWindFull, chordLineDotRelWindFull );
                angleOfAttackRadsFull *= m_aoaMultiplier;
                auto angleOfAttackFull = Math<real_Num>::RadToDeg( angleOfAttackRadsFull );
                angleOfAttackFull = Math<real_Num>::clamp(
                    angleOfAttackFull, -static_cast<real_Num>( 180.0 ), static_cast<real_Num>( 180.0 ) );
                auto aoaNormalised = Math<real_Num>::clamp(
                    angleOfAttackFull / static_cast<real_Num>( 180.0 ), -1.0, 1.0 );

                auto fStallControlCL = static_cast<real_Num>( 0.0 );
                auto fStallControlCD = static_cast<real_Num>( 0.0 );
                auto fStallControlCM = static_cast<real_Num>( 0.0 );

#if defined _DEBUG
                if( !Math<real_Num>::equals( m_stallControlCL, 0.0 ) )
                {
                    int stop = 0;
                    stop = 0;
                }
#endif

                if( chordLineDotRelWindFull > std::numeric_limits<real_Num>::epsilon() )
                {
                    if( attachedControlSurface != nullptr )
                    {
                        if( aoaNormalised > m_stallThreshold )
                        {
                            real_Num range = static_cast<real_Num>( 1.0 ) - m_stallThreshold;
                            real_Num fUpDotRelWind = aoaNormalised - m_stallThreshold;
                            if( range > std::numeric_limits<real_Num>::epsilon() )
                            {
                                fStallControlCL = ( fUpDotRelWind / range ) * m_stallControlCL;
                                fStallControlCD = ( fUpDotRelWind / range ) * m_stallControlCD;
                                fStallControlCM = ( fUpDotRelWind / range ) * m_stallControlCM;
                            }
                        }
                        else if( aoaNormalised < -m_stallThreshold )
                        {
                            real_Num range = static_cast<real_Num>( 1.0 ) - m_stallThreshold;
                            real_Num fUpDotRelWind = aoaNormalised + m_stallThreshold;
                            if( range > std::numeric_limits<real_Num>::epsilon() )
                            {
                                fStallControlCL = ( fUpDotRelWind / range ) * m_stallControlCL;
                                fStallControlCD = ( fUpDotRelWind / range ) * m_stallControlCD;
                                fStallControlCM = ( fUpDotRelWind / range ) * m_stallControlCM;
                            }
                        }
                    }
                }

#if defined _DEBUG
                if( getName().find( "STAB" ) != std::string::npos )
                {
                    // DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 9), aerodynamicCenter, aerodynamicCenter +
                    // (relativeWind * 1.0f), 0xFF0000);
                    int stop = 0;
                    stop = 0;
                }

                if( getName().find( "WING" ) != std::string::npos )
                {
                    // DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 9), aerodynamicCenter, aerodynamicCenter +
                    // (relativeWind * 1.0f), 0xFF0000);
                    int stop = 0;
                    stop = 0;
                }
#endif

                real_Num totalLift = static_cast<real_Num>( 0.0 );
                real_Num totalDrag = static_cast<real_Num>( 0.0 );
                real_Num cM = static_cast<real_Num>( 0.0 );

                real_Num clGroundEffectMult = static_cast<real_Num>( 1.0 );
                real_Num cdGroundEffectMult = static_cast<real_Num>( 1.0 );

                if( m_groundEffect )
                {
                    m_groundEffect->getGroundEffectCoefficients( a, b, c, d, clGroundEffectMult,
                                                                 cdGroundEffectMult );
                }

                real_Num cL = static_cast<real_Num>( 0.0 );

                if( aerofoil )
                {
#if 0
				auto CL = aerofoil->getCL();
				auto CD = aerofoil->getCD();
				auto CM = aerofoil->getCM();

				auto pclPlus10 = aerofoil->m_clPlus10;
				auto pcdPlus10 = aerofoil->m_cdPlus10;
				auto pcmPlus10 = aerofoil->m_cmPlus10;

				auto pclMinus10 = aerofoil->m_clMinus10;
				auto pcdMinus10 = aerofoil->m_cdMinus10;
				auto pcmMinus10 = aerofoil->m_cmMinus10;

				auto clZero = CL->interpolate(angleOfAttackZero);
				auto cdZero = CD->interpolate(angleOfAttackZero);
				auto cmZero = CM->interpolate(angleOfAttackZero);

				auto clPlus10 = pclPlus10->interpolate(angleOfAttackZero);
				auto cdPlus10 = pcdPlus10->interpolate(angleOfAttackZero);
				auto cmPlus10 = pcmPlus10->interpolate(angleOfAttackZero);

				auto clMinus10 = pclMinus10->interpolate(angleOfAttackZero);
				auto cdMinus10 = pcdMinus10->interpolate(angleOfAttackZero);
				auto cmMinus10 = pcmMinus10->interpolate(angleOfAttackZero);

				if (m_controlSurface)
				{
					auto controlAoaMultuplier = m_controlSurface->getAoAMultiplier();
					auto lookup = angleOfAttackZero * controlAoaMultuplier;

					clPlus10 = pclPlus10->interpolate(lookup);
					cdPlus10 = pcdPlus10->interpolate(lookup);
					cmPlus10 = pcmPlus10->interpolate(lookup);

					clMinus10 = pclMinus10->interpolate(lookup);
					cdMinus10 = pcdMinus10->interpolate(lookup);
					cmMinus10 = pcmMinus10->interpolate(lookup);
				}

				//auto cl10 = CL->interpolate(angleOfAttack10);
				//auto cd10 = CD->interpolate(angleOfAttack10);
				//auto cm10 = CM->interpolate(angleOfAttack10);

				auto camber = real_Num(0.0);
				auto camberNormalised = real_Num(0.0);

				if (m_controlSurface)
				{
					real_Num fCurrentDeflection = m_controlSurface->getCurrentDeflection() * real_Num(2.0);

#    if defined _DEBUG
					if (getName().find("STAB") != std::string::npos)
					{
						//DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 9), aerodynamicCenter, aerodynamicCenter + (relativeWind * 1.0f), 0xFF0000);
						int stop = 0;
						stop = 0;
					}

					if (getName().find("WING") != std::string::npos)
					{
						//DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 9), aerodynamicCenter, aerodynamicCenter + (relativeWind * 1.0f), 0xFF0000);
						int stop = 0;
						stop = 0;
					}
#    endif

					if (m_localTransform->getScale().X() > 0)
					{
						if (m_controlSurface->isReversed())
						{
							camberNormalised = -fCurrentDeflection;
						}
						else
						{
							camberNormalised = fCurrentDeflection;
						}
					}
					else
					{
						if (m_controlSurface->isReversed())
						{
							camberNormalised = fCurrentDeflection;
						}
						else
						{
							camberNormalised = -fCurrentDeflection;
						}
					}

					//if (m_controlSurface->isReversed())
					//{
					//	if (m_localTransform->getScale().X() > 0)
					//	{
					//		camberNormalised = -fCurrentDeflection;
					//		camber = -fCurrentDeflection;
					//	}
					//	else
					//	{
					//		fCurrentDeflection = -fCurrentDeflection;
					//		camberNormalised = fCurrentDeflection;
					//		camber = fCurrentDeflection;
					//	}
					//}
					//else
					//{
					//	if (m_localTransform->getScale().X() > 0)
					//	{
					//		camberNormalised = fCurrentDeflection;
					//		camber = fCurrentDeflection;
					//	}
					//	else
					//	{
					//		fCurrentDeflection = -fCurrentDeflection;
					//		camberNormalised = -fCurrentDeflection;
					//		camber = -fCurrentDeflection;
					//	}
					//}

					camber = camberNormalised > real_Num(0.0) ?
						Math<real_Num>::Abs(fCurrentDeflection) * m_controlSurface->getMaxDeflectionDegrees() :
						Math<real_Num>::Abs(fCurrentDeflection) * m_controlSurface->getMinDeflectionDegrees();
				}

				//Use aerofoil..
				//L = cl * a * 0.5f * r * v^2
				WP_ASSERT(CL);
				if (m_controlSurface)
				{
					if (camber > real_Num(0.0))
					{
						real_Num delta = (camberNormalised * m_controlSurface->getClMultiplier() * m_controlSurface->getClLookup()->interpolate(camber));
						cL = clZero + (clPlus10 - clZero) * Math<real_Num>::Abs(delta);
						cL += (clPlus10 - clZero) * fStallControlCL;
					}
					else
					{
						real_Num delta = (camberNormalised * m_controlSurface->getClMultiplier() * m_controlSurface->getClLookup()->interpolate(camber));
						cL = clZero + (clMinus10 - clZero) * Math<real_Num>::Abs(delta);
						cL += (clMinus10 - clZero) * fStallControlCL;
					}
				}
				else
				{
					cL = clZero;
				}

				cL *= clGroundEffectMult;
				cL *= m_clMultiplier;

				//WP_ASSERT(cL < 1.5);

				const real_Num r = aircraft->getAirDensity();
				real_Num v = relativeWind.length();

				totalLift = cL * areaZero * real_Num(0.5) * r * (v * v);

				//D = 0.5 * cd * r * (v * v) * a;
				WP_ASSERT(CD);
				real_Num cD = 0.0;
				if (m_controlSurface)
				{
					if (camber > real_Num(0.0))
					{
						auto delta = (camberNormalised * m_controlSurface->getCdMultiplier() * m_controlSurface->getCdLookup()->interpolate(camber));
						cD = cdZero + (cdPlus10 - cdZero) * Math<real_Num>::Abs(delta);
						cD += (cdPlus10 - cdZero) * fStallControlCD;
					}
					else
					{
						auto delta = (camberNormalised * m_controlSurface->getCdMultiplier() * m_controlSurface->getCdLookup()->interpolate(camber));
						cD = cdZero + (cdMinus10 - cdZero) * Math<real_Num>::Abs(delta);
						cD += (cdMinus10 - cdZero) * fStallControlCD;
					}
				}
				else
				{
					cD = cdZero;
				}

				cD *= cdGroundEffectMult;
				cD *= m_cdMultiplier;

				totalDrag = real_Num(0.5) * cD * r * (v * v) * areaZero;

				WP_ASSERT(CM);
				if (m_controlSurface)
				{
					if (camber > real_Num(0.0))
					{
						auto delta = (camberNormalised * m_controlSurface->getCmMultiplier() * m_controlSurface->getCmLookup()->interpolate(camber));
						cM = cmZero + (cmPlus10 - cmZero) * Math<real_Num>::Abs(delta);
						cM += (cmPlus10 - cmZero) * fStallControlCM;
					}
					else
					{
						auto delta = (camberNormalised * m_controlSurface->getCmMultiplier() * m_controlSurface->getCmLookup()->interpolate(camber));
						cM = cmZero + (cmMinus10 - cmZero) * Math<real_Num>::Abs(delta);
						cM += (cmMinus10 - cmZero) * fStallControlCM;
					}
				}
				else
				{
					cM = cmZero;
				}

				cM *= m_cmMultiplier;

				WP_ASSERT(Math<real_Num>::Abs(totalLift) < real_Num(10000.0));
				WP_ASSERT(Math<real_Num>::Abs(totalDrag) < real_Num(10000.0));

				WP_ASSERT(Math<real_Num>::Abs(cL) < real_Num(1000.0));
				WP_ASSERT(Math<real_Num>::Abs(cD) < real_Num(1000.0));
				WP_ASSERT(Math<real_Num>::Abs(cM) < real_Num(1000.0));
#else
                    auto CL = aerofoil->getCL();
                    auto CD = aerofoil->getCD();
                    auto CM = aerofoil->getCM();

                    auto clZero = CL->interpolate( angleOfAttackZero );
                    auto cdZero = CD->interpolate( angleOfAttackZero );
                    auto cmZero = CM->interpolate( angleOfAttackZero );

                    auto cl10 = CL->interpolate( angleOfAttack10 );
                    auto cd10 = CD->interpolate( angleOfAttack10 );
                    auto cm10 = CM->interpolate( angleOfAttack10 );

                    auto clMax = CL->interpolate( angleOfAttackFull );
                    auto cdMax = CD->interpolate( angleOfAttackFull );
                    auto cmMax = CM->interpolate( angleOfAttackFull );

                    auto camber = static_cast<real_Num>( 0.0 );
                    auto camberNormalised = static_cast<real_Num>( 0.0 );

                    if( m_controlSurface )
                    {
                        real_Num fCurrentDeflection =
                            m_controlSurface->getCurrentDeflection() * static_cast<real_Num>( 2.0 );

                        if( m_controlSurface->isReversed() )
                        {
                            camberNormalised = -fCurrentDeflection;
                            camber = -fCurrentDeflection;
                        }
                        else
                        {
                            camberNormalised = fCurrentDeflection;
                            camber = fCurrentDeflection;
                        }

                        camber = fCurrentDeflection > static_cast<real_Num>( 0.0 )
                                   ? Math<real_Num>::Abs( fCurrentDeflection ) *
                                         m_controlSurface->getMaxDeflectionDegrees()
                                   : Math<real_Num>::Abs( fCurrentDeflection ) *
                                         m_controlSurface->getMinDeflectionDegrees();
                    }

                    // Use aerofoil..
                    // L = cl * a * 0.5f * r * v^2
                    WP_ASSERT( CL );
                    if( m_controlSurface )
                    {
                        auto clLookup = m_controlSurface->getClLookup();
                        auto clLookupMultiplier = clLookup->interpolate( camber );
                        auto clMultiplier = m_controlSurface->getClMultiplier();
                        auto delta = camberNormalised * clMultiplier * clLookupMultiplier;
                        cL = clZero + ( cl10 - clZero ) * Math<real_Num>::Abs( delta );
                        cL += ( clMax - clZero ) * Math<real_Num>::Abs( fStallControlCL );
                    }
                    else
                    {
                        cL = clZero;
                    }

                    cL *= clGroundEffectMult;
                    cL *= m_clMultiplier;

                    const real_Num r = aircraft->getAirDensity();
                    real_Num       v = relativeWind.length();

                    const auto maxCL = static_cast<real_Num>( 2.0 );
                    WP_ASSERT( cL < maxCL );
                    cL = Math<real_Num>::clamp( cL, -maxCL, maxCL );
                    totalLift = cL * areaZero * static_cast<real_Num>( 0.5 ) * r * ( v * v );

                    // D = 0.5 * cd * r * (v * v) * a;
                    WP_ASSERT( CD );
                    real_Num cD = 0.0;
                    if( m_controlSurface )
                    {
                        auto cdLookup = m_controlSurface->getCdLookup();
                        auto cdLookupValue = cdLookup->interpolate( camber );
                        auto cdMultiplier = m_controlSurface->getCdMultiplier();
                        auto delta = camberNormalised * cdMultiplier * cdLookupValue;
                        cD = cdZero + ( cd10 - cdZero ) * Math<real_Num>::Abs( delta );
                        cD += ( cdMax - cdZero ) * Math<real_Num>::Abs( fStallControlCD );
                    }
                    else
                    {
                        cD = cdZero;
                    }

                    cD *= cdGroundEffectMult;
                    cD *= m_cdMultiplier;

                    const auto maxCD = static_cast<real_Num>( 2.0 );
                    WP_ASSERT( cD < maxCD );
                    cD = Math<real_Num>::clamp( cD, -maxCD, maxCD );
                    totalDrag = static_cast<real_Num>( 0.5 ) * cD * r * ( v * v ) * areaZero;

                    WP_ASSERT( CM );
                    if( m_controlSurface )
                    {
                        auto cmLookup = m_controlSurface->getCmLookup();
                        auto cmLookupValue = cmLookup->interpolate( camber );
                        auto cmMultiplier = m_controlSurface->getCmMultiplier();
                        auto delta = camberNormalised * cmMultiplier * cmLookupValue;
                        cM = cmZero + ( cm10 - cmZero ) * Math<real_Num>::Abs( delta );
                        cM += ( cmMax - cmZero ) * Math<real_Num>::Abs( fStallControlCM );
                    }
                    else
                    {
                        cM = cmZero;
                    }

                    cM *= m_cmMultiplier;

                    WP_ASSERT( Math<real_Num>::Abs( totalLift ) < static_cast<real_Num>( 10000.0 ) );
                    WP_ASSERT( Math<real_Num>::Abs( totalDrag ) < static_cast<real_Num>( 10000.0 ) );

                    WP_ASSERT( Math<real_Num>::Abs( cL ) < static_cast<real_Num>( 1000.0 ) );
                    WP_ASSERT( Math<real_Num>::Abs( cD ) < static_cast<real_Num>( 1000.0 ) );
                    WP_ASSERT( Math<real_Num>::Abs( cM ) < static_cast<real_Num>( 1000.0 ) );
#endif
                }
                else
                {
                    // Fall back to basic l/d equations..
                    // L = cl * a * 0.5f * r * v^2
                    // Approximate Cl using the following formula - Cl = 2 * pi * angle (in radians)
                    real_Num cL = static_cast<real_Num>( 2.0 ) * Math<real_Num>::pi() *
                                  Math<real_Num>::DegToRad( angleOfAttackFull );
                    cL *= clGroundEffectMult;

                    real_Num r = static_cast<real_Num>( 1.29 );
                    real_Num v = relativeWind.length();
                    totalLift = cL * areaZero * static_cast<real_Num>( 0.5 ) * r * ( v * v );

                    static real_Num maxLift = static_cast<real_Num>( 1.0 );
                    // totalLift = MathF::clamp(totalLift, -maxLift, maxLift);

                    // D = 0.5f * cd * r * v2 * a;
                    // Typical aerofoil drag co efficient is .045;
                    real_Num cD = m_cdOverride; // Typical aerofoil drag co efficient
                    cD *= cdGroundEffectMult;

                    totalDrag = static_cast<real_Num>( 0.5 ) * cD * r * ( v * v ) * areaZero;

                    static real_Num maxDrag = static_cast<real_Num>( 1.0 );
                    totalDrag = Math<real_Num>::clamp( totalDrag, -maxDrag, maxDrag );

                    cM = static_cast<real_Num>( 0.0 );

                    WP_ASSERT( cL < static_cast<real_Num>( 1000.0 ) );
                    WP_ASSERT( cD < static_cast<real_Num>( 1000.0 ) );
                    WP_ASSERT( cM < static_cast<real_Num>( 1000.0 ) );
                }

                // real_Num fVorticity = real_Num(0.5) * chordLength * cL *
                // chordLine.dotProduct(-relativeWind);

                // Build Lift vector.
                // auto worldRight = m_worldTransform->right();
                // auto localRight = aircraft->getBodyTransform()->inverseTransformVector(worldRight);
                // Vector3<real_Num> liftVector = localRight.crossProduct(-relativeWind.normaliseCopy());
                Vector3<real_Num> liftVector =
                    m_localWingRight.crossProduct( -relativeWind.normaliseCopy() );
                // Vector3<real_Num> liftForce = relativeWind.normaliseCopy().crossProduct(wingRight);
                liftVector.normalise();
                // WP_ASSERT(liftForce.dotProduct(worldTransform->up()) < real_Num(-0.5));

                Vector3<real_Num> liftForce = liftVector * totalLift; // *fVorticity;

                if( aircraft->getDisplayDebugData() )
                {
                    if( getName().find( "STAB" ) != std::string::npos )
                    {
                        if( aircraft->getDisplayDebugData() )
                        {
                            DEBUG_DRAW_LOCAL_LINE_BY_ID(
                                getDebugId( sectionIdx, 9 ), aerodynamicCenter,
                                aerodynamicCenter + ( liftForce * debugLineScale ), 0xF00000 );
                        }
                    }

                    if( getName().find( "WING" ) != std::string::npos )
                    {
                        if( aircraft->getDisplayDebugData() )
                        {
                            DEBUG_DRAW_LOCAL_LINE_BY_ID(
                                getDebugId( sectionIdx, 9 ), aerodynamicCenter,
                                aerodynamicCenter + ( liftForce * debugLineScale ), 0x00FF00 );
                        }
                    }

                    if( getName().find( "RUDDER" ) != std::string::npos )
                    {
                        if( aircraft->getDisplayDebugData() )
                        {
                            DEBUG_DRAW_LOCAL_LINE_BY_ID(
                                getDebugId( sectionIdx, 9 ), aerodynamicCenter,
                                aerodynamicCenter + ( liftForce * debugLineScale ), 0x00FF00 );
                        }
                    }
                }

                // Vector3<real_Num> localRelativeWind =
                // aircraftTransform->inverseTransformVector(relativeWind); Vector3<real_Num> localRight
                // = localTransform->right(); Vector3<real_Num> localLiftForce =
                // localRight.crossProduct(localRelativeWind.normaliseCopy());
                // localLiftForce.normalise();
                // localLiftForce *= totalLift;

                // Drag vector.
                auto dragForce = relativeWind.normaliseCopy() * totalDrag;

                // Debug.DrawLine(aerodynamicCenter, aerodynamicCenter +
                // (dragForce*Time.deltaTime*debugLineScale), Color.red);

                // Find wing pitching moment...
                //  0.5 RoAir*Sqr(Flowpeed)
                real_Num q = ( static_cast<real_Num>( 0.5 ) * aircraft->getAirDensity() *
                               ( relativeWind.length() * relativeWind.length() ) );
                real_Num wingPitchingMoment = cM * chordLengthZero * q * areaZero;
                // Vector3<real_Num> pitchAxis = chordLine.crossProduct(liftForce.normaliseCopy());
                Vector3<real_Num> pitchAxis = liftForce.normaliseCopy().crossProduct( chordLineZero );
                pitchAxis.normalise();
                pitchAxis *= wingPitchingMoment;

                WP_ASSERT( Math<real_Num>::Abs( liftForce.Z() ) < static_cast<real_Num>( 2000.0 ) );
                WP_ASSERT( Math<real_Num>::Abs( dragForce.Y() ) < static_cast<real_Num>( 2000.0 ) );

                // liftForce.X() = real_Num(0.0); // todo check
                // liftForce.Z() = real_Num(0.0); // todo check
                // dragForce.Y() = real_Num(0.0); // todo check

                // WP_ASSERT(localLiftForce.length() < real_Num(2000.0));
                // WP_ASSERT(localDragForce.length() < real_Num(2000.0));
                // WP_ASSERT(localAerodynamicCenter.length() < real_Num(2.0));

                if( aircraft->getEnablePowerUnit() )
                {
                    aircraftBody->addLocalForceAtLocalPosition( liftForce, aerodynamicCenter );
                    aircraftBody->addLocalForceAtLocalPosition( dragForce, aerodynamicCenter );
                    aircraftBody->addLocalTorque( pitchAxis );
                }
            }
        }

        void CAircraftWingFast::updateSeparate( const double &t, const double &fDT )
        {
            SmartPtr<IVehicleBody> aircraftBody = m_parent;
            auto                   aircraft = m_parentAircraft;

            // const SmartPtr<Transform3<real_Num>>& aircraftTransform = aircraft->getBodyTransform();
            // SmartPtr<Transform3<real_Num>>& worldTransform = getWorldTransform();
            auto localTransform = getLocalTransform();
            // SmartPtr<Transform3<real_Num>>& localBodyTransform = getLocalBodyTransform();
            SmartPtr<IAerodymanicsWind> pWind = aircraft->getWind();

            auto wind = pWind;
            auto attachedControlSurface = m_controlSurface;
            auto propwash = m_propWash;
            auto aerofoil = m_aerofoil;

            float debugLineScale = 5.0f; // 1.0f / 30.0f;

            s32 lineId = 0;

            auto localWingRootLeadingEdge = m_rootLeadingEdge;
            auto localWingRootTrailingEdge = m_rootTrailingEdge;
            auto localWingTipLeadingEdge = m_tipLeadingEdge;
            auto localWingTipTrailingEdge = m_tipTrailingEdge;
            auto localRootLiftPosition = m_rootLiftPosition;
            auto localTipLiftPosition = m_tipLiftPosition;
            auto liftLineChordPosition = m_liftLineChordPosition;
            auto sectionCount = m_sectionCount; // *aircraft->getSectionMultiplier();

            // m_windRotation.X() += 100.0 * fDT;
            // m_windRotation.X() = Math<real_Num>::wrap(m_windRotation.X(), -25.0, 25.0);

            static real_Num angle = 45.0;
            m_windRotation.X() = angle;

            DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( 11 ), m_tipLiftPosition,
                                         m_tipLiftPosition + ( m_localWingRight * 0.1 ), 0x000000 );

            // Per section update.
            for( s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx )
            {
                // R A-----------------B (Leading edge)
                // O |                 |
                // O |                 |
                // T D-----------------C (Trailing edge

                // Find points a,b,c & d for this chunk of wing.
                const Vector3<real_Num> sectionA =
                    localWingRootLeadingEdge +
                    ( ( localWingTipLeadingEdge - localWingRootLeadingEdge ) *
                      static_cast<real_Num>( sectionIdx ) / static_cast<real_Num>( sectionCount ) );
                const Vector3<real_Num> sectionB =
                    localWingRootLeadingEdge +
                    ( ( localWingTipLeadingEdge - localWingRootLeadingEdge ) *
                      static_cast<real_Num>( sectionIdx + 1 ) / static_cast<real_Num>( sectionCount ) );
                const Vector3<real_Num> sectionC =
                    localWingRootTrailingEdge +
                    ( ( localWingTipTrailingEdge - localWingRootTrailingEdge ) *
                      static_cast<real_Num>( sectionIdx + 1 ) / static_cast<real_Num>( sectionCount ) );
                const Vector3<real_Num> sectionD =
                    localWingRootTrailingEdge +
                    ( ( localWingTipTrailingEdge - localWingRootTrailingEdge ) *
                      static_cast<real_Num>( sectionIdx ) / static_cast<real_Num>( sectionCount ) );

                auto a = sectionA;
                auto b = sectionB;
                auto c = sectionC;
                auto d = sectionD;

                //////Draw premodified wing.
                // DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 0), a, b);
                // DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 1), b, c);
                // DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 2), c, d);
                // DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 3), d, a);

                // if (getName() == "WING LOWER LEFT")
                //{
                //	int stop = 0;
                //	stop = 0;
                // }
                real_Num          areaZero = calculateArea( a, b, c, d );
                Vector3<real_Num> sectionRootLiftPositionZero =
                    d + ( ( a - d ) * liftLineChordPosition );
                Vector3<real_Num> sectionTipLiftPositionZero = c + ( ( b - c ) * liftLineChordPosition );

                Vector3<real_Num> chordLineZero = ( a + ( ( b - a ) * static_cast<real_Num>( 0.5 ) ) ) -
                                                  ( d + ( ( c - d ) * static_cast<real_Num>( 0.5 ) ) );
                real_Num          chordLengthZero = chordLineZero.length();
                chordLineZero.normalise();

                Vector3<real_Num> upZero = chordLineZero.crossProduct(
                    ( sectionTipLiftPositionZero - sectionRootLiftPositionZero ).normaliseCopy() );
                // Vector3<real_Num> up = (sectionTipLiftPosition -
                // sectionRootLiftPosition).normaliseCopy().crossProduct(chordLine);
                upZero.normalise();

                Vector3<real_Num> localScaleZero = localTransform.getScale();
                if( localScaleZero.X() < static_cast<real_Num>( 0.0 ) )
                {
                    upZero = -upZero;
                }

                // Find the aerodynamic center.
                Vector3<real_Num> aerodynamicCenter =
                    sectionRootLiftPositionZero +
                    ( ( sectionTipLiftPositionZero - sectionRootLiftPositionZero ) *
                      static_cast<real_Num>( 0.5 ) );

                if( attachedControlSurface )
                {
                    attachedControlSurface->modifyWingGeometry( sectionIdx, a, b, c, d );
                }

                real_Num          areaFull = calculateArea( a, b, c, d );
                Vector3<real_Num> sectionRootLiftPositionFull =
                    d + ( ( a - d ) * liftLineChordPosition );
                Vector3<real_Num> sectionTipLiftPositionFull = c + ( ( b - c ) * liftLineChordPosition );

                Vector3<real_Num> chordLineFull = ( a + ( ( b - a ) * static_cast<real_Num>( 0.5 ) ) ) -
                                                  ( d + ( ( c - d ) * static_cast<real_Num>( 0.5 ) ) );
                chordLineFull.normalise();

                Vector3<real_Num> upFull = chordLineFull.crossProduct(
                    ( sectionTipLiftPositionFull - sectionRootLiftPositionFull ).normaliseCopy() );
                // Vector3<real_Num> up = (sectionTipLiftPosition -
                // sectionRootLiftPosition).normaliseCopy().crossProduct(chordLine);
                upFull.normalise();

                Vector3<real_Num> localScaleFull = localTransform.getScale();
                if( localScaleFull.X() < static_cast<real_Num>( 0.0 ) )
                {
                    upFull = -upFull;
                }

                //////Draw modified wing.
                if( aircraft->getDisplayDebugData() )
                {
                    DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 4 ), a, b, 0x00FF00 );
                    DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 5 ), b, c, 0xFF0000 );
                    DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 6 ), c, d, 0x0000FF );
                    DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 7 ), d, a, 0x00FFFF );
                }

                a = sectionA;
                b = sectionB;
                c = sectionC;
                d = sectionD;

                // If we have a control surface attached update the geometry based on control surface
                // inputs.
                if( attachedControlSurface )
                {
                    attachedControlSurface->modifyWingGeometry( sectionIdx, a, b, c, d,
                                                                static_cast<real_Num>( 0.1 ) );
                }

                real_Num area10 = calculateArea( a, b, c, d );

                // WP_ASSERT(Math<real_Num>::equals(area, area2));
                // if (aircraft->getDisplayDebugData())
                //{
                //	DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(i, 14), a, b, 0x00FF00);
                //	DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(i, 15), b, c, 0xFF0000);
                //	DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(i, 16), c, d, 0x0000FF);
                //	DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(i, 17), d, a, 0x00FFFF);
                // }

                // Recalculate lift positions..
                Vector3<real_Num> sectionRootLiftPosition10 = d + ( ( a - d ) * liftLineChordPosition );
                Vector3<real_Num> sectionTipLiftPosition10 = c + ( ( b - c ) * liftLineChordPosition );

                // Find the chord line.
                Vector3<real_Num> chordLine10 = ( a + ( ( b - a ) * static_cast<real_Num>( 0.5 ) ) ) -
                                                ( d + ( ( c - d ) * static_cast<real_Num>( 0.5 ) ) );
                real_Num          chordLength10 = chordLine10.length();
                chordLine10.normalise();

                // aerodynamicCenter += chordLine * chordLength * real_Num(0.5);

                // if (aircraft->getDisplayDebugData())
                //{
                //	if (getName().find("WING") != std::string::npos)
                //	{
                //		DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(i, 8), aerodynamicCenter,
                // aerodynamicCenter + chordLine, 			0x0000FF);
                //	}
                // }

                if( aircraft->getDisplayDebugData() )
                {
                    if( getName().find( "WING" ) != std::string::npos )
                    {
                        DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 8 ), aerodynamicCenter,
                                                     aerodynamicCenter + chordLineFull, 0x0000FF );
                    }
                }

                Vector3<real_Num> windVector = wind->getWind( aerodynamicCenter.Y(), t );
                Vector3<real_Num> relativeWind =
                    calculateRelativeWind( sectionIdx, aerodynamicCenter, windVector );

                if( !aircraft->getEnablePowerUnit() )
                {
                    static auto w = Vector3<real_Num>( 0, 0, -5 );

                    Quaternion<real_Num> q;
                    q.fromDegrees( m_windRotation );

                    relativeWind = q * w;
                    // relativeWind.normalise();

                    // relativeWind = aircraft->getBodyTransform()->inverseTransformVector(relativeWind);
                }

                // const double AirViscosity = 18.325E-6;
                // m_re[i] = aircraft->getAirDensity() * relativeWind.length() * chordLength /
                // AirViscosity;

                if( aircraft->getDisplayDebugData() )
                {
                    if( getName().find( "STAB" ) != std::string::npos )
                    {
                        DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 14 ), aerodynamicCenter,
                                                     aerodynamicCenter + ( relativeWind * 1.0f ),
                                                     0xFF0000 );
                    }

                    if( getName().find( "WING" ) != std::string::npos )
                    {
                        DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 30 ), aerodynamicCenter,
                                                     aerodynamicCenter + ( relativeWind * 1.0f ),
                                                     0xFF0000 );
                    }
                }

                // Vector3<real_Num> localChordLine =
                // aircraftTransform->inverseTransformVector(chordLine); Vector3<real_Num>
                // localRelativeWind = aircraftTransform->inverseTransformVector(relativeWind);

                // localChordLine = localChordLine.normaliseCopy();
                // localRelativeWind = localRelativeWind.normaliseCopy();

                Vector3<real_Num> up10 = chordLine10.crossProduct(
                    ( sectionTipLiftPosition10 - sectionRootLiftPosition10 ).normaliseCopy() );
                // Vector3<real_Num> up = (sectionTipLiftPosition -
                // sectionRootLiftPosition).normaliseCopy().crossProduct(chordLine);
                up10.normalise();

                // WP_ASSERT(up.Y() > real_Num(0.0));

                Vector3<real_Num> localScale = localTransform.getScale();
                if( localScale.X() < static_cast<real_Num>( 0.0 ) )
                {
                    up10 = -up10;
                }

                if( aircraft->getDisplayDebugData() )
                {
                    if( getName().find( "WING" ) != std::string::npos )
                    {
                        DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 16 ), aerodynamicCenter,
                                                     aerodynamicCenter + ( upFull * 1.0f ), 0xFFFF00 );
                    }

                    if( getName().find( "STAB" ) != std::string::npos )
                    {
                        DEBUG_DRAW_LOCAL_LINE_BY_ID( getDebugId( sectionIdx, 16 ), aerodynamicCenter,
                                                     aerodynamicCenter + ( upFull * 1.0f ), 0xFFFFFF );
                    }
                }

                // Find the angle of attack.
                auto relativeWindNormalised = relativeWind.normaliseCopy();

                auto chordLineDotRelWind10 = chordLine10.dotProduct( -relativeWindNormalised );
                auto upDotRelWind10 = up10.dotProduct( -relativeWindNormalised );
                auto angleOfAttackRads10 =
                    Math<real_Num>::ATan2( upDotRelWind10, chordLineDotRelWind10 );
                angleOfAttackRads10 *= m_aoaMultiplier;
                auto angleOfAttack10 = Math<real_Num>::RadToDeg( angleOfAttackRads10 );
                angleOfAttack10 = Math<real_Num>::clamp(
                    angleOfAttack10, -static_cast<real_Num>( 180.0 ), static_cast<real_Num>( 180.0 ) );

                auto chordLineDotRelWindZero = chordLineZero.dotProduct( -relativeWindNormalised );
                auto upDotRelWindZero = upZero.dotProduct( -relativeWindNormalised );
                auto angleOfAttackRadsZero =
                    Math<real_Num>::ATan2( upDotRelWindZero, chordLineDotRelWindZero );
                angleOfAttackRadsZero *= m_aoaMultiplier;
                auto angleOfAttackZero = Math<real_Num>::RadToDeg( angleOfAttackRadsZero );
                angleOfAttackZero = Math<real_Num>::clamp(
                    angleOfAttackZero, -static_cast<real_Num>( 180.0 ), static_cast<real_Num>( 180.0 ) );

                auto chordLineDotRelWindFull = chordLineFull.dotProduct( -relativeWindNormalised );
                auto upDotRelWindFull = upFull.dotProduct( -relativeWindNormalised );
                auto angleOfAttackRadsFull =
                    Math<real_Num>::ATan2( upDotRelWindFull, chordLineDotRelWindFull );
                angleOfAttackRadsFull *= m_aoaMultiplier;
                auto angleOfAttackFull = Math<real_Num>::RadToDeg( angleOfAttackRadsFull );
                angleOfAttackFull = Math<real_Num>::clamp(
                    angleOfAttackFull, -static_cast<real_Num>( 180.0 ), static_cast<real_Num>( 180.0 ) );
                auto aoaNormalised = Math<real_Num>::clamp(
                    angleOfAttackFull / static_cast<real_Num>( 180.0 ), -1.0, 1.0 );

                auto fStallControlCL = static_cast<real_Num>( 0.0 );
                auto fStallControlCD = static_cast<real_Num>( 0.0 );
                auto fStallControlCM = static_cast<real_Num>( 0.0 );

#if defined _DEBUG
                if( !Math<real_Num>::equals( m_stallControlCL, 0.0 ) )
                {
                    int stop = 0;
                    stop = 0;
                }
#endif

                if( chordLineDotRelWindFull > std::numeric_limits<real_Num>::epsilon() )
                {
                    if( attachedControlSurface != nullptr )
                    {
                        if( aoaNormalised > m_stallThreshold )
                        {
                            real_Num range = static_cast<real_Num>( 1.0 ) - m_stallThreshold;
                            real_Num fUpDotRelWind = aoaNormalised - m_stallThreshold;
                            if( range > std::numeric_limits<real_Num>::epsilon() )
                            {
                                fStallControlCL = ( fUpDotRelWind / range ) * m_stallControlCL;
                                fStallControlCD = ( fUpDotRelWind / range ) * m_stallControlCD;
                                fStallControlCM = ( fUpDotRelWind / range ) * m_stallControlCM;
                            }
                        }
                        else if( aoaNormalised < -m_stallThreshold )
                        {
                            real_Num range = static_cast<real_Num>( 1.0 ) - m_stallThreshold;
                            real_Num fUpDotRelWind = aoaNormalised + m_stallThreshold;
                            if( range > std::numeric_limits<real_Num>::epsilon() )
                            {
                                fStallControlCL = ( fUpDotRelWind / range ) * m_stallControlCL;
                                fStallControlCD = ( fUpDotRelWind / range ) * m_stallControlCD;
                                fStallControlCM = ( fUpDotRelWind / range ) * m_stallControlCM;
                            }
                        }
                    }
                }

#if defined _DEBUG
                if( getName().find( "STAB" ) != std::string::npos )
                {
                    // DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 9), aerodynamicCenter, aerodynamicCenter +
                    // (relativeWind * 1.0f), 0xFF0000);
                    int stop = 0;
                    stop = 0;
                }

                if( getName().find( "WING" ) != std::string::npos )
                {
                    // DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 9), aerodynamicCenter, aerodynamicCenter +
                    // (relativeWind * 1.0f), 0xFF0000);
                    int stop = 0;
                    stop = 0;
                }
#endif

                real_Num totalLift = static_cast<real_Num>( 0.0 );
                real_Num totalDrag = static_cast<real_Num>( 0.0 );
                real_Num cM = static_cast<real_Num>( 0.0 );

                real_Num clGroundEffectMult = static_cast<real_Num>( 1.0 );
                real_Num cdGroundEffectMult = static_cast<real_Num>( 1.0 );

                if( m_groundEffect )
                {
                    m_groundEffect->getGroundEffectCoefficients( a, b, c, d, clGroundEffectMult,
                                                                 cdGroundEffectMult );
                }

                real_Num cL = static_cast<real_Num>( 0.0 );

                if( aerofoil )
                {
#if 0
				auto CL = aerofoil->getCL();
				auto CD = aerofoil->getCD();
				auto CM = aerofoil->getCM();

				auto pclPlus10 = aerofoil->m_clPlus10;
				auto pcdPlus10 = aerofoil->m_cdPlus10;
				auto pcmPlus10 = aerofoil->m_cmPlus10;

				auto pclMinus10 = aerofoil->m_clMinus10;
				auto pcdMinus10 = aerofoil->m_cdMinus10;
				auto pcmMinus10 = aerofoil->m_cmMinus10;

				auto clZero = CL->interpolate(angleOfAttackZero);
				auto cdZero = CD->interpolate(angleOfAttackZero);
				auto cmZero = CM->interpolate(angleOfAttackZero);

				auto clPlus10 = pclPlus10->interpolate(angleOfAttackZero);
				auto cdPlus10 = pcdPlus10->interpolate(angleOfAttackZero);
				auto cmPlus10 = pcmPlus10->interpolate(angleOfAttackZero);

				auto clMinus10 = pclMinus10->interpolate(angleOfAttackZero);
				auto cdMinus10 = pcdMinus10->interpolate(angleOfAttackZero);
				auto cmMinus10 = pcmMinus10->interpolate(angleOfAttackZero);

				if (m_controlSurface)
				{
					auto controlAoaMultuplier = m_controlSurface->getAoAMultiplier();
					auto lookup = angleOfAttackZero * controlAoaMultuplier;

					clPlus10 = pclPlus10->interpolate(lookup);
					cdPlus10 = pcdPlus10->interpolate(lookup);
					cmPlus10 = pcmPlus10->interpolate(lookup);

					clMinus10 = pclMinus10->interpolate(lookup);
					cdMinus10 = pcdMinus10->interpolate(lookup);
					cmMinus10 = pcmMinus10->interpolate(lookup);
				}

				//auto cl10 = CL->interpolate(angleOfAttack10);
				//auto cd10 = CD->interpolate(angleOfAttack10);
				//auto cm10 = CM->interpolate(angleOfAttack10);

				auto camber = real_Num(0.0);
				auto camberNormalised = real_Num(0.0);

				if (m_controlSurface)
				{
					real_Num fCurrentDeflection = m_controlSurface->getCurrentDeflection() * real_Num(2.0);

#    if defined _DEBUG
					if (getName().find("STAB") != std::string::npos)
					{
						//DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 9), aerodynamicCenter, aerodynamicCenter + (relativeWind * 1.0f), 0xFF0000);
						int stop = 0;
						stop = 0;
					}

					if (getName().find("WING") != std::string::npos)
					{
						//DEBUG_DRAW_LINE_BY_ID(getDebugId(i, 9), aerodynamicCenter, aerodynamicCenter + (relativeWind * 1.0f), 0xFF0000);
						int stop = 0;
						stop = 0;
					}
#    endif

					if (m_localTransform->getScale().X() > 0)
					{
						if (m_controlSurface->isReversed())
						{
							camberNormalised = -fCurrentDeflection;
						}
						else
						{
							camberNormalised = fCurrentDeflection;
						}
					}
					else
					{
						if (m_controlSurface->isReversed())
						{
							camberNormalised = fCurrentDeflection;
						}
						else
						{
							camberNormalised = -fCurrentDeflection;
						}
					}

					//if (m_controlSurface->isReversed())
					//{
					//	if (m_localTransform->getScale().X() > 0)
					//	{
					//		camberNormalised = -fCurrentDeflection;
					//		camber = -fCurrentDeflection;
					//	}
					//	else
					//	{
					//		fCurrentDeflection = -fCurrentDeflection;
					//		camberNormalised = fCurrentDeflection;
					//		camber = fCurrentDeflection;
					//	}
					//}
					//else
					//{
					//	if (m_localTransform->getScale().X() > 0)
					//	{
					//		camberNormalised = fCurrentDeflection;
					//		camber = fCurrentDeflection;
					//	}
					//	else
					//	{
					//		fCurrentDeflection = -fCurrentDeflection;
					//		camberNormalised = -fCurrentDeflection;
					//		camber = -fCurrentDeflection;
					//	}
					//}

					camber = camberNormalised > real_Num(0.0) ?
						Math<real_Num>::Abs(fCurrentDeflection) * m_controlSurface->getMaxDeflectionDegrees() :
						Math<real_Num>::Abs(fCurrentDeflection) * m_controlSurface->getMinDeflectionDegrees();
				}

				//Use aerofoil..
				//L = cl * a * 0.5f * r * v^2
				WP_ASSERT(CL);
				if (m_controlSurface)
				{
					if (camber > real_Num(0.0))
					{
						real_Num delta = (camberNormalised * m_controlSurface->getClMultiplier() * m_controlSurface->getClLookup()->interpolate(camber));
						cL = clZero + (clPlus10 - clZero) * Math<real_Num>::Abs(delta);
						cL += (clPlus10 - clZero) * fStallControlCL;
					}
					else
					{
						real_Num delta = (camberNormalised * m_controlSurface->getClMultiplier() * m_controlSurface->getClLookup()->interpolate(camber));
						cL = clZero + (clMinus10 - clZero) * Math<real_Num>::Abs(delta);
						cL += (clMinus10 - clZero) * fStallControlCL;
					}
				}
				else
				{
					cL = clZero;
				}

				cL *= clGroundEffectMult;
				cL *= m_clMultiplier;

				//WP_ASSERT(cL < 1.5);

				const real_Num r = aircraft->getAirDensity();
				real_Num v = relativeWind.length();

				totalLift = cL * areaZero * real_Num(0.5) * r * (v * v);

				//D = 0.5 * cd * r * (v * v) * a;
				WP_ASSERT(CD);
				real_Num cD = 0.0;
				if (m_controlSurface)
				{
					if (camber > real_Num(0.0))
					{
						auto delta = (camberNormalised * m_controlSurface->getCdMultiplier() * m_controlSurface->getCdLookup()->interpolate(camber));
						cD = cdZero + (cdPlus10 - cdZero) * Math<real_Num>::Abs(delta);
						cD += (cdPlus10 - cdZero) * fStallControlCD;
					}
					else
					{
						auto delta = (camberNormalised * m_controlSurface->getCdMultiplier() * m_controlSurface->getCdLookup()->interpolate(camber));
						cD = cdZero + (cdMinus10 - cdZero) * Math<real_Num>::Abs(delta);
						cD += (cdMinus10 - cdZero) * fStallControlCD;
					}
				}
				else
				{
					cD = cdZero;
				}

				cD *= cdGroundEffectMult;
				cD *= m_cdMultiplier;

				totalDrag = real_Num(0.5) * cD * r * (v * v) * areaZero;

				WP_ASSERT(CM);
				if (m_controlSurface)
				{
					if (camber > real_Num(0.0))
					{
						auto delta = (camberNormalised * m_controlSurface->getCmMultiplier() * m_controlSurface->getCmLookup()->interpolate(camber));
						cM = cmZero + (cmPlus10 - cmZero) * Math<real_Num>::Abs(delta);
						cM += (cmPlus10 - cmZero) * fStallControlCM;
					}
					else
					{
						auto delta = (camberNormalised * m_controlSurface->getCmMultiplier() * m_controlSurface->getCmLookup()->interpolate(camber));
						cM = cmZero + (cmMinus10 - cmZero) * Math<real_Num>::Abs(delta);
						cM += (cmMinus10 - cmZero) * fStallControlCM;
					}
				}
				else
				{
					cM = cmZero;
				}

				cM *= m_cmMultiplier;

				WP_ASSERT(Math<real_Num>::Abs(totalLift) < real_Num(10000.0));
				WP_ASSERT(Math<real_Num>::Abs(totalDrag) < real_Num(10000.0));

				WP_ASSERT(Math<real_Num>::Abs(cL) < real_Num(1000.0));
				WP_ASSERT(Math<real_Num>::Abs(cD) < real_Num(1000.0));
				WP_ASSERT(Math<real_Num>::Abs(cM) < real_Num(1000.0));
#else
                    auto CL = aerofoil->getCL();
                    auto CD = aerofoil->getCD();
                    auto CM = aerofoil->getCM();

                    auto clZero = CL->interpolate( angleOfAttackZero );
                    auto cdZero = CD->interpolate( angleOfAttackZero );
                    auto cmZero = CM->interpolate( angleOfAttackZero );

                    auto cl10 = CL->interpolate( angleOfAttack10 );
                    auto cd10 = CD->interpolate( angleOfAttack10 );
                    auto cm10 = CM->interpolate( angleOfAttack10 );

                    auto clMax = CL->interpolate( angleOfAttackFull );
                    auto cdMax = CD->interpolate( angleOfAttackFull );
                    auto cmMax = CM->interpolate( angleOfAttackFull );

                    auto camber = static_cast<real_Num>( 0.0 );
                    auto camberNormalised = static_cast<real_Num>( 0.0 );

                    if( m_controlSurface )
                    {
                        real_Num fCurrentDeflection =
                            m_controlSurface->getCurrentDeflection() * static_cast<real_Num>( 2.0 );

                        if( m_controlSurface->isReversed() )
                        {
                            camberNormalised = -fCurrentDeflection;
                            camber = -fCurrentDeflection;
                        }
                        else
                        {
                            camberNormalised = fCurrentDeflection;
                            camber = fCurrentDeflection;
                        }

                        camber = fCurrentDeflection > static_cast<real_Num>( 0.0 )
                                   ? Math<real_Num>::Abs( fCurrentDeflection ) *
                                         m_controlSurface->getMaxDeflectionDegrees()
                                   : Math<real_Num>::Abs( fCurrentDeflection ) *
                                         m_controlSurface->getMinDeflectionDegrees();
                    }

                    // Use aerofoil..
                    // L = cl * a * 0.5f * r * v^2
                    WP_ASSERT( CL );
                    if( m_controlSurface )
                    {
                        auto clLookup = m_controlSurface->getClLookup();
                        auto clLookupMultiplier = clLookup->interpolate( camber );
                        auto clMultiplier = m_controlSurface->getClMultiplier();
                        auto delta = camberNormalised * clMultiplier * clLookupMultiplier;
                        cL = clZero + ( cl10 - clZero ) * Math<real_Num>::Abs( delta );
                        cL += ( clMax - clZero ) * Math<real_Num>::Abs( fStallControlCL );
                    }
                    else
                    {
                        cL = clZero;
                    }

                    cL *= clGroundEffectMult;
                    cL *= m_clMultiplier;

                    const real_Num r = aircraft->getAirDensity();
                    real_Num       v = relativeWind.length();

                    const auto maxCL = static_cast<real_Num>( 2.0 );
                    WP_ASSERT( cL < maxCL );
                    cL = Math<real_Num>::clamp( cL, -maxCL, maxCL );
                    totalLift = cL * areaZero * static_cast<real_Num>( 0.5 ) * r * ( v * v );

                    // D = 0.5 * cd * r * (v * v) * a;
                    WP_ASSERT( CD );
                    real_Num cD = 0.0;
                    if( m_controlSurface )
                    {
                        auto cdLookup = m_controlSurface->getCdLookup();
                        auto cdLookupValue = cdLookup->interpolate( camber );
                        auto cdMultiplier = m_controlSurface->getCdMultiplier();
                        auto delta = camberNormalised * cdMultiplier * cdLookupValue;
                        cD = cdZero + ( cd10 - cdZero ) * Math<real_Num>::Abs( delta );
                        cD += ( cdMax - cdZero ) * Math<real_Num>::Abs( fStallControlCD );
                    }
                    else
                    {
                        cD = cdZero;
                    }

                    cD *= cdGroundEffectMult;
                    cD *= m_cdMultiplier;

                    const auto maxCD = static_cast<real_Num>( 2.0 );
                    WP_ASSERT( cD < maxCD );
                    cD = Math<real_Num>::clamp( cD, -maxCD, maxCD );
                    totalDrag = static_cast<real_Num>( 0.5 ) * cD * r * ( v * v ) * areaZero;

                    WP_ASSERT( CM );
                    if( m_controlSurface )
                    {
                        auto cmLookup = m_controlSurface->getCmLookup();
                        auto cmLookupValue = cmLookup->interpolate( camber );
                        auto cmMultiplier = m_controlSurface->getCmMultiplier();
                        auto delta = camberNormalised * cmMultiplier * cmLookupValue;
                        cM = cmZero + ( cm10 - cmZero ) * Math<real_Num>::Abs( delta );
                        cM += ( cmMax - cmZero ) * Math<real_Num>::Abs( fStallControlCM );
                    }
                    else
                    {
                        cM = cmZero;
                    }

                    cM *= m_cmMultiplier;

                    WP_ASSERT( Math<real_Num>::Abs( totalLift ) < static_cast<real_Num>( 10000.0 ) );
                    WP_ASSERT( Math<real_Num>::Abs( totalDrag ) < static_cast<real_Num>( 10000.0 ) );

                    WP_ASSERT( Math<real_Num>::Abs( cL ) < static_cast<real_Num>( 1000.0 ) );
                    WP_ASSERT( Math<real_Num>::Abs( cD ) < static_cast<real_Num>( 1000.0 ) );
                    WP_ASSERT( Math<real_Num>::Abs( cM ) < static_cast<real_Num>( 1000.0 ) );
#endif
                }
                else
                {
                    // Fall back to basic l/d equations..
                    // L = cl * a * 0.5f * r * v^2
                    // Approximate Cl using the following formula - Cl = 2 * pi * angle (in radians)
                    real_Num cL = static_cast<real_Num>( 2.0 ) * Math<real_Num>::pi() *
                                  Math<real_Num>::DegToRad( angleOfAttackFull );
                    cL *= clGroundEffectMult;

                    real_Num r = static_cast<real_Num>( 1.29 );
                    real_Num v = relativeWind.length();
                    totalLift = cL * areaZero * static_cast<real_Num>( 0.5 ) * r * ( v * v );

                    static real_Num maxLift = static_cast<real_Num>( 1.0 );
                    // totalLift = MathF::clamp(totalLift, -maxLift, maxLift);

                    // D = 0.5f * cd * r * v2 * a;
                    // Typical aerofoil drag co efficient is .045;
                    real_Num cD = m_cdOverride; // Typical aerofoil drag co efficient
                    cD *= cdGroundEffectMult;

                    totalDrag = static_cast<real_Num>( 0.5 ) * cD * r * ( v * v ) * areaZero;

                    static real_Num maxDrag = static_cast<real_Num>( 1.0 );
                    totalDrag = Math<real_Num>::clamp( totalDrag, -maxDrag, maxDrag );

                    cM = static_cast<real_Num>( 0.0 );

                    WP_ASSERT( cL < static_cast<real_Num>( 1000.0 ) );
                    WP_ASSERT( cD < static_cast<real_Num>( 1000.0 ) );
                    WP_ASSERT( cM < static_cast<real_Num>( 1000.0 ) );
                }

                // real_Num fVorticity = real_Num(0.5) * chordLength * cL *
                // chordLine.dotProduct(-relativeWind);

                // Build Lift vector.
                // auto worldRight = m_worldTransform->right();
                // auto localRight = aircraft->getBodyTransform()->inverseTransformVector(worldRight);
                // Vector3<real_Num> liftVector = localRight.crossProduct(-relativeWind.normaliseCopy());
                Vector3<real_Num> liftVector =
                    m_localWingRight.crossProduct( -relativeWind.normaliseCopy() );
                // Vector3<real_Num> liftForce = relativeWind.normaliseCopy().crossProduct(wingRight);
                liftVector.normalise();
                // WP_ASSERT(liftForce.dotProduct(worldTransform->up()) < real_Num(-0.5));

                Vector3<real_Num> liftForce = liftVector * totalLift; // *fVorticity;

                if( aircraft->getDisplayDebugData() )
                {
                    if( getName().find( "STAB" ) != std::string::npos )
                    {
                        if( aircraft->getDisplayDebugData() )
                        {
                            DEBUG_DRAW_LOCAL_LINE_BY_ID(
                                getDebugId( sectionIdx, 9 ), aerodynamicCenter,
                                aerodynamicCenter + ( liftForce * debugLineScale ), 0xF00000 );
                        }
                    }

                    if( getName().find( "WING" ) != std::string::npos )
                    {
                        if( aircraft->getDisplayDebugData() )
                        {
                            DEBUG_DRAW_LOCAL_LINE_BY_ID(
                                getDebugId( sectionIdx, 9 ), aerodynamicCenter,
                                aerodynamicCenter + ( liftForce * debugLineScale ), 0x00FF00 );
                        }
                    }

                    if( getName().find( "RUDDER" ) != std::string::npos )
                    {
                        if( aircraft->getDisplayDebugData() )
                        {
                            DEBUG_DRAW_LOCAL_LINE_BY_ID(
                                getDebugId( sectionIdx, 9 ), aerodynamicCenter,
                                aerodynamicCenter + ( liftForce * debugLineScale ), 0x00FF00 );
                        }
                    }
                }

                // Vector3<real_Num> localRelativeWind =
                // aircraftTransform->inverseTransformVector(relativeWind); Vector3<real_Num> localRight
                // = localTransform->right(); Vector3<real_Num> localLiftForce =
                // localRight.crossProduct(localRelativeWind.normaliseCopy());
                // localLiftForce.normalise();
                // localLiftForce *= totalLift;

                // Drag vector.
                auto dragForce = relativeWind.normaliseCopy() * totalDrag;

                // Debug.DrawLine(aerodynamicCenter, aerodynamicCenter +
                // (dragForce*Time.deltaTime*debugLineScale), Color.red);

                // Find wing pitching moment...
                //  0.5 RoAir*Sqr(Flowpeed)
                real_Num q = ( static_cast<real_Num>( 0.5 ) * aircraft->getAirDensity() *
                               ( relativeWind.length() * relativeWind.length() ) );
                real_Num wingPitchingMoment = cM * chordLengthZero * q * areaZero;
                // Vector3<real_Num> pitchAxis = chordLine.crossProduct(liftForce.normaliseCopy());
                Vector3<real_Num> pitchAxis = liftForce.normaliseCopy().crossProduct( chordLineZero );
                pitchAxis.normalise();
                pitchAxis *= wingPitchingMoment;

                WP_ASSERT( Math<real_Num>::Abs( liftForce.Z() ) < static_cast<real_Num>( 2000.0 ) );
                WP_ASSERT( Math<real_Num>::Abs( dragForce.Y() ) < static_cast<real_Num>( 2000.0 ) );

                // liftForce.X() = real_Num(0.0); // todo check
                // liftForce.Z() = real_Num(0.0); // todo check
                // dragForce.Y() = real_Num(0.0); // todo check

                // WP_ASSERT(localLiftForce.length() < real_Num(2000.0));
                // WP_ASSERT(localDragForce.length() < real_Num(2000.0));
                // WP_ASSERT(localAerodynamicCenter.length() < real_Num(2.0));

                if( aircraft->getEnablePowerUnit() )
                {
                    aircraftBody->addLocalForceAtLocalPosition( liftForce, aerodynamicCenter );
                    aircraftBody->addLocalForceAtLocalPosition( dragForce, aerodynamicCenter );
                    aircraftBody->addLocalTorque( pitchAxis );
                }
            }
        }

        SmartPtr<IAircraftControlSurface> CAircraftWingFast::getAttachedControlSurface() const
        {
            return m_controlSurface;
        }

        void CAircraftWingFast::setAttachedControlSurface(
            SmartPtr<IAircraftControlSurface> attachedControlSurface )
        {
            m_controlSurface = attachedControlSurface;

            if( !isControlSurface() )
            {
                auto pThis = getSharedFromThis<IAircraftWing>();
                m_controlSurface->setMainWing( pThis );
            }
        }

        void CAircraftWingFast::load( void *pData )
        {
            // setData( pData );

            // auto data = static_cast<data::aircraft_wing_data *>(pData);

            // m_sectionCount = data->sectionCount * m_parentAircraft->getSectionMultiplier();

            // m_wingTipWidthZeroToOne = data->wingTipWidthZeroToOne;
            // m_wingTipSweep = data->wingTipSweep;
            // m_wingTipAngle = data->wingTipAngle;
            // m_cdOverride = data->cdOverride;
            // m_liftLineChordPosition = data->liftLineChordPosition;

            // m_aoaMultiplier = data->aoaMultiplier;
            // m_cdMultiplier = data->cdMultiplier;
            // m_clMultiplier = data->clMultiplier;
            // m_cmMultiplier = data->cmMultiplier;

            // m_stallControlCL = data->stallControlCL;
            // m_stallControlCD = data->stallControlCD;
            // m_stallControlCM = data->stallControlCM;
            // m_stallThreshold = data->stallThreshold;

            // data::vec4 p = data->localTransform.position;
            // data::vec4 q = data->localTransform.orientation;
            // data::vec4 s = data->localTransform.scale;

            // auto vPos = Vector3<real_Num>( p.x, p.y, -p.z );
            // auto vScale = Vector3<real_Num>( s.x, s.y, s.z );
            // auto qRot = Quaternion<real_Num>( q.w, -q.x, -q.y, q.z );

            // WP_ASSERT( vScale.length() > std::numeric_limits<f32>::epsilon() );

            // m_localTransform.setPosition( vPos );
            // m_localTransform.setScale( vScale );
            // m_localTransform.setOrientation( qRot );

            // if(!StringUtil::isNullOrEmpty( data->aerofoilName ))
            //{
            //     SmartPtr<CAerofoil> aerofoil = fb::make_ptr<CAerofoil>();
            //     aerofoil->setAircraft( m_parentAircraft );
            //     aerofoil->load( data->aerofoilName + ".airfoil" );
            //     m_aerofoil = aerofoil;
            // }

            // int id = m_idExt++;

            // m_ids.resize( 100 );

            // m_debugIds.resize( m_sectionCount );

            // for(Array<int> &d : m_debugIds)
            //{
            //     d.resize( 200 );
            // }

            // for(int i = 0; i < m_sectionCount; i++)
            //{
            //     for(int x = 0; x < 200; x++)
            //     {
            //         std::string name = StringUtil::toString( id ) + data->name + "_Section_" +
            //                            StringUtil::toString( i ) + "_Line_" + StringUtil::
            //                            toString( x );
            //         s32 hash = StringUtil::getHash( name );
            //         m_debugIds[i][x] = hash;
            //     }
            // }

            // for(size_t i = 0; i < m_ids.size(); ++i)
            //{
            //     m_ids[i] = StringUtil::getHash(
            //         data->name + "_" + StringUtil::toString( static_cast<s32>(i) ) );
            // }

            // updateGeometry();

            // m_windRotation.X() = 10.0;

            // m_aoa.resize(m_sectionCount);
            // m_relativeWind.resize(m_sectionCount);
            // m_currentRelativeWind.resize(m_sectionCount);
            // m_re.resize(m_sectionCount);

            // m_sectionA.resize(m_sectionCount);
            // m_sectionB.resize(m_sectionCount);
            // m_sectionC.resize(m_sectionCount);
            // m_sectionD.resize(m_sectionCount);
            // m_aerodynamicCenters.resize(m_sectionCount);
            // m_chordLines.resize(m_sectionCount);
            // m_chordLengths.resize(m_sectionCount);

            setLoadingState( LoadingState::Loaded );

            // m_workerThread = std::thread(&CAircraftWingFast::updateWorker, this);
        }

        void CAircraftWingFast::unload( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Unloading );
            m_controlSurface = nullptr;
            setLoadingState( LoadingState::Unloaded );
        }

        void CAircraftWingFast::load( SmartPtr<ISharedObject> data )
        {
        }

        SmartPtr<IAircraftPropWash> CAircraftWingFast::getAttachedPropWash() const
        {
            return m_propWash;
        }

        void CAircraftWingFast::setAttachedPropWash( SmartPtr<IAircraftPropWash> attachedPropWash )
        {
            WP_ASSERT( m_propWash == nullptr );

            m_propWash = attachedPropWash;

            WP_ASSERT( getName().find( m_propWash->getName() ) != String::npos );
        }

        void CAircraftWingFast::setControlSurface( bool bIsControlSurface )
        {
            m_isControlSurface = bIsControlSurface;
        }

        bool CAircraftWingFast::useCombinedControlSurface() const
        {
            return m_useCombinedControlSurface;
        }

        void CAircraftWingFast::setUserCombinedControlSurface( bool bUseCombinedControlSurface )
        {
            m_useCombinedControlSurface = bUseCombinedControlSurface;
        }
    } // namespace vehicle
} // namespace workphone
