#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/CAircraftWing.hpp"
#include "WPVehiclePhysics/CAircraftBody.hpp"
#include "WPVehiclePhysics/CAircraft.hpp"
#include "WPVehiclePhysics/CAerofoil.hpp"
#include "WPVehiclePhysics/GroundEffect.hpp"
#include "WPVehiclePhysics/CAircraftPropWash.hpp"
#include "WPVehiclePhysics/EngineSimple.hpp"
#include "WPVehiclePhysics/CAircraftControlSurface.hpp"
#include <Workphone/Workphone.hpp>
#include <iostream>

namespace workphone::vehicle
{
    u32 CAircraftWing::m_idExt = 0;

    CAircraftWing::CAircraftWing()
    {
        m_sectionCount = 10;
        m_wingTipWidthZeroToOne = static_cast<real_Num>(1.0);
        m_wingTipSweep = static_cast<real_Num>(0.0);
        m_wingTipAngle = static_cast<real_Num>(0.0);
        m_cdOverride = static_cast<real_Num>(0.045);

        m_wingArea = static_cast<real_Num>(0.0);

        m_rootLeadingEdge = Vector3<real_Num>::zero();
        m_rootTrailingEdge = Vector3<real_Num>::zero();
        m_tipLeadingEdge = Vector3<real_Num>::zero();
        m_tipTrailingEdge = Vector3<real_Num>::zero();
        m_rootLiftPosition = Vector3<real_Num>::zero();
        m_tipLiftPosition = Vector3<real_Num>::zero();
        m_liftLineChordPosition = static_cast<real_Num>(0.5);

        m_aoa = std::array<real_Num, m_maxSections>({ 0 });
        m_chordLengths = std::array<real_Num, m_maxSections>({ 0 });

        m_re = std::array<real_Num, m_maxSections>({ 0 });
        m_area = std::array<real_Num, m_maxSections>({ 0 });

        m_totalLift = std::array<real_Num, m_maxSections>({ 0 });
        m_totalDrag = std::array<real_Num, m_maxSections>({ 0 });
        m_totalPitch = std::array<real_Num, m_maxSections>({ 0 });

        m_fStallControlCL = std::array<real_Num, m_maxSections>({ 0 });
        m_fStallControlCD = std::array<real_Num, m_maxSections>({ 0 });
        m_fStallControlCM = std::array<real_Num, m_maxSections>({ 0 });
    }

    CAircraftWing::~CAircraftWing()
    {
    }

    void CAircraftWing::updateGeometry()
    {
        auto transform = getLocalTransform();

        auto p = transform.getPosition();
        auto r = transform.getOrientation();
        auto scale = transform.getScale();

        auto right = transform.right();
        auto forward = transform.forward();

        // WP_ASSERT(Math<real_Num>::equals(right.length(), real_Num(1.0)));
        // WP_ASSERT(Math<real_Num>::equals(forward.length(), real_Num(1.0)));

        right.normalise();
        forward.normalise();

        WP_ASSERT(Math<real_Num>::equals( right.length(), 1.0 ));
        WP_ASSERT(Math<real_Num>::equals( forward.length(), 1.0 ));

        forward = -forward; // todo

        // if (scale.X() < real_Num(0.0))
        //{
        //	right = -right;
        // }

        WP_ASSERT(scale.length() > std::numeric_limits<f32>::epsilon());

        // Calculate root and tip center points.
        auto wingRootCenter = p - (right * (scale.X() * static_cast<real_Num>(0.5)));
        auto wingTipCenter = p + (right * (scale.X() * static_cast<real_Num>(0.5)));
        wingTipCenter += forward * m_wingTipSweep;

        // Calculate corners.
        m_rootLeadingEdge =
            wingRootCenter + (forward * (scale.Z() * static_cast<real_Num>(0.5)));
        m_rootTrailingEdge =
            wingRootCenter - (forward * (scale.Z() * static_cast<real_Num>(0.5)));
        m_tipLeadingEdge =
            wingTipCenter +
            (forward * ((scale.Z() * static_cast<real_Num>(0.5)) * m_wingTipWidthZeroToOne));
        m_tipTrailingEdge =
            wingTipCenter -
            (forward * ((scale.Z() * static_cast<real_Num>(0.5)) * m_wingTipWidthZeroToOne));

        // Tweak tip corners based on the angle between them.
        auto tipTrailingEdgeToTipLeadingEdge = m_tipLeadingEdge - m_tipTrailingEdge;

        auto axis = (r * Vector3<real_Num>::right()).normaliseCopy();
        auto rotation = Quaternion<real_Num>::angleAxis(m_wingTipAngle, axis);
        tipTrailingEdgeToTipLeadingEdge = rotation * tipTrailingEdgeToTipLeadingEdge;
        m_tipTrailingEdge =
            wingTipCenter - (tipTrailingEdgeToTipLeadingEdge * 0.5);
        m_tipLeadingEdge =
            wingTipCenter + (tipTrailingEdgeToTipLeadingEdge * 0.5);

        auto rootDistance = static_cast<real_Num>(0.0);
        auto tipDistance = static_cast<real_Num>(0.0);

        auto attachedControlSurface = getAttachedControlSurface();
        if(attachedControlSurface)
        {
            if(!isControlSurface())
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

                auto pMainWing = attachedControlSurface->getMainWing();
                auto mainWing = workphone::static_pointer_cast<CAircraftWing>(pMainWing);
                if(mainWing)
                {
                    m_rootLeadingEdge =
                        mainWing->m_rootTrailingEdge +
                        (mainWing->m_rootLeadingEdge - mainWing->m_rootTrailingEdge) *
                        rootDistance;
                    m_tipLeadingEdge =
                        mainWing->m_tipTrailingEdge +
                        (mainWing->m_tipLeadingEdge - mainWing->m_tipTrailingEdge) * tipDistance;
                }
            }
        }

        m_rootLiftPosition = m_rootTrailingEdge + ((m_rootLeadingEdge - m_rootTrailingEdge) *
                                                   m_liftLineChordPosition);
        m_tipLiftPosition = m_tipTrailingEdge +
                            ((m_tipLeadingEdge - m_tipTrailingEdge) * m_liftLineChordPosition);

        m_rootLiftPositionReversed =
            m_rootTrailingEdge +
            ((m_rootLeadingEdge - m_rootTrailingEdge) * m_liftLineChordPositionReversed);
        m_tipLiftPositionReversed = m_tipTrailingEdge + ((m_tipLeadingEdge - m_tipTrailingEdge) *
                                                         m_liftLineChordPositionReversed);

        // m_localWingRight = (wingRootCenter - wingTipCenter).normaliseCopy();
        m_localWingRight = (wingTipCenter - wingRootCenter).normaliseCopy();
        // m_localWingRight = transform->transformVector(m_localWingRight);

        // m_localWingRight =
        // m_parentAircraft->getWorldTransform()->inverseTransformVector(getWorldTransform()->right());
        m_localWingRight.normalise();

        if(scale.X() < static_cast<real_Num>(0.0))
        {
            m_localWingRight = -m_localWingRight;
        }

        // Calculate wing area.
        m_wingArea = calculateArea(m_rootLeadingEdge, m_tipLeadingEdge, m_tipTrailingEdge,
                                   m_rootTrailingEdge);
        WP_ASSERT(m_wingArea < static_cast<real_Num>( 10.0 ));
    }

    real_Num CAircraftWing::calculateArea(const Vector3<real_Num> &pointA,
                                          const Vector3<real_Num> &pointB,
                                          const Vector3<real_Num> &pointC,
                                          const Vector3<real_Num> &pointD)
    {
        auto ab = (pointB - pointA).length();
        auto bc = (pointC - pointB).length();
        auto cd = (pointD - pointC).length();
        auto da = (pointA - pointD).length();

        WP_ASSERT(ab < static_cast<real_Num>( 10.0 ));
        WP_ASSERT(bc < static_cast<real_Num>( 10.0 ));
        WP_ASSERT(cd < static_cast<real_Num>( 10.0 ));
        WP_ASSERT(da < static_cast<real_Num>( 10.0 ));

        auto s = (ab + bc + cd + da) * static_cast<real_Num>(0.5);
        WP_ASSERT(Math<real_Num>::isFinite( s ));

        auto squareArea = (s - ab) * (s - bc) * (s - cd) * (s - da);
        WP_ASSERT(Math<real_Num>::isFinite( squareArea ));

        WP_ASSERT(Math<real_Num>::isFinite( Math<real_Num>::Sqrt( squareArea ) ));
        return Math<real_Num>::Sqrt(squareArea);
    }

    s32 CAircraftWing::getDebugId(s32 section, s32 index) const
    {
        if(section < m_debugIds.size())
        {
            if(index < m_debugIds[section].size())
            {
                int id = m_debugIds[section][index];
                WP_ASSERT(id != 0);
                return id;
            }
        }

        return 0;
    }

    s32 CAircraftWing::getDebugId(s32 i) const
    {
        if(i < m_ids.size())
        {
            return m_ids[i];
        }

        return 0;
    }

    void CAircraftWing::drawGizmos()
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

    Vector3<real_Num> CAircraftWing::calculateRelativeWind(
        s32 sectionIndex, const Vector3<real_Num> &aerodynamicCenter, const Vector3<real_Num> &wind)
    {
        const auto aircraftBody = m_parent;
        auto aircraft = m_parentAircraft;

        const auto aircraftTransform = aircraft->getBodyTransform();
        const auto worldTransform = getWorldTransform();
        const auto localTransform = getLocalTransform();

        auto worldAerodynamicCenter = aircraftTransform.transformPoint(aerodynamicCenter);
        auto relativeWind = wind - aircraft->getPointVelocity(worldAerodynamicCenter);

        constexpr auto maxVelocity = static_cast<real_Num>(10000.0);
        relativeWind.X() = Math<real_Num>::clamp(relativeWind.X(), -maxVelocity, maxVelocity);
        relativeWind.Y() = Math<real_Num>::clamp(relativeWind.Y(), -maxVelocity, maxVelocity);
        relativeWind.Z() = Math<real_Num>::clamp(relativeWind.Z(), -maxVelocity, maxVelocity);

        return aircraftTransform.inverseTransformVector(relativeWind);
    }

    void CAircraftWing::calculateRelativeWindDebug(double dt)
    {
        const auto aircraftBody = m_parent;
        const auto aircraft = m_parentAircraft;

        auto aircraftTransform = aircraft->getBodyTransform();
        // SmartPtr<Transform3<real_Num>>& worldTransform = getWorldTransform();
        auto localTransform = getLocalTransform();
        // SmartPtr<Transform3<real_Num>>& localBodyTransform = getLocalBodyTransform();
        auto wind = aircraft->getWind();

        auto aircraftWorldPosition = aircraftTransform.getPosition();
        auto windVector = wind->getWind(aircraftWorldPosition.Y(), 0.0);
        windVector = aircraftTransform.inverseTransformVector(windVector);
        windVector = Vector3<real_Num>::zero();

        auto attachedControlSurface = m_controlSurface;
        auto propwash = m_propWash;
        auto aerofoil = m_aerofoil;

        constexpr float debugLineScale = 5.0f; // 1.0f / 30.0f;

        s32 lineId = 0;

        auto airDensity = aircraft->getAirDensity();

        auto localWingRootLeadingEdge = m_rootLeadingEdge;
        auto localWingRootTrailingEdge = m_rootTrailingEdge;
        auto localWingTipLeadingEdge = m_tipLeadingEdge;
        auto localWingTipTrailingEdge = m_tipTrailingEdge;
        // auto localRootLiftPosition = m_rootLiftPosition;
        // auto localTipLiftPosition = m_tipLiftPosition;
        auto liftLineChordPosition = m_liftLineChordPosition;
        auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

        for(s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx)
        {
            if(m_controlSurface)
            {
                if(isControlSurface())
                {
                    if(!m_controlSurface->isAffectedSection(sectionIdx))
                    {
                        continue;
                    }
                }
            }

            const auto aerodynamicCenter = m_aerodynamicCenters[sectionIdx];

            auto relativeWind = calculateRelativeWind3(sectionIdx, aerodynamicCenter, windVector);
            if(m_propWash)
            {
                auto propwash = m_propWash->getPropWash(sectionIdx);
                relativeWind += propwash;
            }

            if(!aircraft->getEnablePowerUnit())
            {
                static auto strength = -10.0;
                auto w = Vector3<real_Num>(0, 0, strength);
                // m_windRotation.Y() += 1.0 * dt;

                static auto windY = 0.0;
                m_windRotation.Y() = windY;

                Quaternion<real_Num> q;
                q.fromDegrees(m_windRotation);

                relativeWind = q * w;
                // relativeWind.normalise();

                // relativeWind = aircraft->getBodyTransform()->inverseTransformVector(relativeWind);

                auto fromCOMToAerodynamicCenter = aerodynamicCenter - aircraft->getCG();
                auto angularVelocity = aircraftBody->getLocalAngularVelocity();

                //// hack
                // if (angularVelocity.Y() < 0.0)
                //{
                //	angularVelocity.Y() = angularVelocity.Y() * 0.985;
                // }

                auto localRelativeWind = fromCOMToAerodynamicCenter.crossProduct(angularVelocity);
                // localRelativeWind *= aircraft->getRollwiseDamping();
                localRelativeWind = localRelativeWind.normaliseCopy() * angularVelocity.length() *
                                    fromCOMToAerodynamicCenter.length();

                relativeWind += localRelativeWind;

                if(m_propWash)
                {
                    static auto propwashStrength = 10.0;
                    auto propwash = m_propWash->getPropWash(sectionIdx);
                    // auto propwash = Vector3<real_Num>::UNIT_Z * -propwashStrength;
                    relativeWind += propwash;
                }
            }

            // if (aircraft->getDisplayDebugData())
            //{
            //	aircraft->drawLocalPoint(0, getDebugId(1), relativeWind * 0.5f, 0xFF0000);
            // }

            // Find the angle of attack.
            m_relativeWind[sectionIdx] = relativeWind;
        }

        for(s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx)
        {
            if(m_controlSurface)
            {
                if(isControlSurface())
                {
                    if(!m_controlSurface->isAffectedSection(sectionIdx))
                    {
                        continue;
                    }
                }
            }

            const auto aerodynamicCenter = m_aerodynamicCentersReverse[sectionIdx];

            auto relativeWind = calculateRelativeWind3(sectionIdx, aerodynamicCenter, windVector);
            if(m_propWash)
            {
                auto propwash = m_propWash->getPropWash(sectionIdx);
                relativeWind += propwash;
            }

            if(!aircraft->getEnablePowerUnit())
            {
                static auto strength = -10.0;
                auto w = Vector3<real_Num>(0, 0, strength);
                // m_windRotation.Y() += 1.0 * dt;

                static auto windY = 0.0;
                m_windRotation.Y() = windY;

                Quaternion<real_Num> q;
                q.fromDegrees(m_windRotation);

                relativeWind = q * w;
                // relativeWind.normalise();

                // relativeWind = aircraft->getBodyTransform()->inverseTransformVector(relativeWind);

                auto fromCOMToAerodynamicCenter = aerodynamicCenter - aircraft->getCG();
                auto angularVelocity = aircraftBody->getLocalAngularVelocity();

                //// hack
                // if (angularVelocity.Y() < 0.0)
                //{
                //	angularVelocity.Y() = angularVelocity.Y() * 0.985;
                // }

                auto localRelativeWind = fromCOMToAerodynamicCenter.crossProduct(angularVelocity);
                // localRelativeWind *= aircraft->getRollwiseDamping();
                localRelativeWind = localRelativeWind.normaliseCopy() * angularVelocity.length() *
                                    fromCOMToAerodynamicCenter.length();

                relativeWind += localRelativeWind;

                if(m_propWash)
                {
                    static auto propwashStrength = 10.0;
                    auto propwash = m_propWash->getPropWash(sectionIdx);
                    // auto propwash = Vector3<real_Num>::UNIT_Z * -propwashStrength;
                    relativeWind += propwash;
                }
            }

            // if (aircraft->getDisplayDebugData())
            //{
            //	aircraft->drawLocalPoint(0, getDebugId(1), relativeWind * 0.5f, 0xFF0000);
            // }

            // Find the angle of attack.
            m_relativeWindReversed[sectionIdx] = relativeWind;
        }
    }

    void CAircraftWing::calculateRelativeWind(double dt)
    {
        const auto aircraftBody = m_parent;
        const auto aircraft = m_parentAircraft;

        const auto aircraftTransform = aircraft->getBodyTransform();
        // SmartPtr<Transform3<real_Num>>& worldTransform = getWorldTransform();
        const auto localTransform = getLocalTransform();
        // SmartPtr<Transform3<real_Num>>& localBodyTransform = getLocalBodyTransform();
        auto wind = aircraft->getWind();

        auto aircraftWorldPosition = aircraftTransform.getPosition();
        auto windVector = wind->getWind(aircraftWorldPosition.Y(), 0.0);
        windVector = aircraftTransform.inverseTransformVector(windVector);

        auto attachedControlSurface = m_controlSurface;
        auto propwash = m_propWash;
        auto aerofoil = m_aerofoil;

        constexpr float debugLineScale = 5.0f; // 1.0f / 30.0f;

        s32 lineId = 0;

        auto airDensity = aircraft->getAirDensity();

        // auto localWingRootLeadingEdge = m_rootLeadingEdge;
        // auto localWingRootTrailingEdge = m_rootTrailingEdge;
        // auto localWingTipLeadingEdge = m_tipLeadingEdge;
        // auto localWingTipTrailingEdge = m_tipTrailingEdge;
        // auto localRootLiftPosition = m_rootLiftPosition;
        // auto localTipLiftPosition = m_tipLiftPosition;
        // auto liftLineChordPosition = m_liftLineChordPosition;
        auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

        for(s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx)
        {
            if(m_controlSurface)
            {
                if(isControlSurface())
                {
                    if(!m_controlSurface->isAffectedSection(sectionIdx))
                    {
                        continue;
                    }
                }
            }

            const auto aerodynamicCenter = m_aerodynamicCenters[sectionIdx];

            auto relativeWind = calculateRelativeWind3(sectionIdx, aerodynamicCenter, windVector);
            if(m_propWash)
            {
                auto propwash = m_propWash->getPropWash(sectionIdx);
                relativeWind += propwash;
            }

            if(!aircraft->getEnablePowerUnit())
            {
                static auto strength = static_cast<real_Num>(-10.0);
                auto w = Vector3<real_Num>(0, 0, strength);
                // m_windRotation.Y() += 1.0 * dt;

                static auto windY = 0.0;
                m_windRotation.Y() = windY;

                Quaternion<real_Num> q;
                q.fromDegrees(m_windRotation);

                // relativeWind = q * w;
                // relativeWind.normalise();

                // relativeWind = aircraft->getBodyTransform()->inverseTransformVector(relativeWind);

                auto fromCOMToAerodynamicCenter = aerodynamicCenter - aircraft->getCG();
                auto angularVelocity = aircraftBody->getLocalAngularVelocity();

                //// hack
                // if (angularVelocity.Y() < 0.0)
                //{
                //	angularVelocity.Y() = angularVelocity.Y() * 0.985;
                // }

                auto localRelativeWind = fromCOMToAerodynamicCenter.crossProduct(angularVelocity);
                // localRelativeWind *= aircraft->getRollwiseDamping();
                localRelativeWind = localRelativeWind.normaliseCopy() * angularVelocity.length() *
                                    fromCOMToAerodynamicCenter.length();

                relativeWind += localRelativeWind;

                if(m_propWash)
                {
                    static auto propwashStrength = 10.0;
                    auto propwash = m_propWash->getPropWash(sectionIdx);
                    // auto propwash = Vector3<real_Num>::UNIT_Z * -propwashStrength;
                    relativeWind += propwash;
                }
            }

            // if (aircraft->getDisplayDebugData())
            //{
            //	aircraft->drawLocalPoint(0, getDebugId(1), relativeWind * 0.5f, 0xFF0000);
            // }

            // Find the angle of attack.
            m_relativeWind[sectionIdx] = relativeWind;
        }
    }

    void CAircraftWing::calculateRelativeWindReverse(double dt)
    {
        // const auto aircraftBody = m_parent;
        const auto aircraft = m_parentAircraft;

        const auto aircraftTransform = aircraft->getBodyTransform();
        ////SmartPtr<Transform3<real_Num>>& worldTransform = getWorldTransform();
        // const auto localTransform = getLocalTransform();
        ////SmartPtr<Transform3<real_Num>>& localBodyTransform = getLocalBodyTransform();

        auto wind = aircraft->getWind();
        auto aircraftWorldPosition = aircraftTransform.getPosition();
        auto windVector = wind->getWind(aircraftWorldPosition.Y(), 0.0);
        windVector = aircraftTransform.inverseTransformVector(windVector);
        // windVector = Vector3<real_Num>::zero();

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
        auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

        for(s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx)
        {
            if(m_controlSurface)
            {
                if(isControlSurface())
                {
                    if(!m_controlSurface->isAffectedSection(sectionIdx))
                    {
                        continue;
                    }
                }
            }

            const auto aerodynamicCenter = m_aerodynamicCentersReverse[sectionIdx];

            auto relativeWind = calculateRelativeWind3(sectionIdx, aerodynamicCenter, windVector);
            if(m_propWash)
            {
                auto propwash = m_propWash->getPropWash(sectionIdx);
                relativeWind += propwash;
            }

            m_relativeWindReversed[sectionIdx] = relativeWind;
        }
    }

    Vector3<real_Num> CAircraftWing::calculateRelativeWind2(
        s32 sectionIndex, const Vector3<real_Num> &aerodynamicCenter, const Vector3<real_Num> &wind)
    {
        auto aircraftBody = m_parent;
        auto aircraft = m_parentAircraft;

        auto aircraftTransform = aircraft->getBodyTransform();
        auto worldTransform = getWorldTransform();
        auto localTransform = getLocalTransform();

        auto relativeWind = wind - aircraft->getLinearVelocity();

        auto worldAerodynamicCenter = aircraftTransform.transformPoint(aerodynamicCenter);
        auto fromCOMToAerodynamicCenter =
            worldAerodynamicCenter - aircraftBody->getWorldCenterOfMass();
        auto angularVelocity = aircraftBody->getAngularVelocity();

        auto localRelativeWind = fromCOMToAerodynamicCenter.crossProduct(angularVelocity);

        localRelativeWind *= aircraft->getRollwiseDamping();

        relativeWind += localRelativeWind;
        relativeWind = aircraftTransform.inverseTransformVector(relativeWind);

        if(m_propWash)
        {
            relativeWind += m_propWash->getPropWash(sectionIndex);
        }

        constexpr real_Num maxVelocity = 10000.0;
        relativeWind.X() = Math<real_Num>::clamp(relativeWind.X(), -maxVelocity, maxVelocity);
        relativeWind.Y() = Math<real_Num>::clamp(relativeWind.Y(), -maxVelocity, maxVelocity);
        relativeWind.Z() = Math<real_Num>::clamp(relativeWind.Z(), -maxVelocity, maxVelocity);

        return relativeWind;
    }

    Vector3<real_Num> CAircraftWing::calculateRelativeWind3(
        s32 sectionIndex, const Vector3<real_Num> &aerodynamicCenter, const Vector3<real_Num> &wind)
    {
        auto aircraftBody = m_parent;
        auto aircraft = m_parentAircraft;

        auto aircraftTransform = aircraft->getBodyTransform();
        auto worldTransform = getWorldTransform();
        auto localTransform = getLocalTransform();

        auto relativeWind = wind - aircraft->getLocalLinearVelocity();

        auto fromCOMToAerodynamicCenter = aerodynamicCenter - aircraft->getCG();
        auto angularVelocity = aircraftBody->getLocalAngularVelocity();

        auto localRelativeWind = fromCOMToAerodynamicCenter.crossProduct(angularVelocity);

        localRelativeWind *= aircraft->getRollwiseDamping();

        relativeWind += localRelativeWind;

        constexpr auto maxVelocity = static_cast<real_Num>(10000.0);
        relativeWind.X() = Math<real_Num>::clamp(relativeWind.X(), -maxVelocity, maxVelocity);
        relativeWind.Y() = Math<real_Num>::clamp(relativeWind.Y(), -maxVelocity, maxVelocity);
        relativeWind.Z() = Math<real_Num>::clamp(relativeWind.Z(), -maxVelocity, maxVelocity);

        return relativeWind;
    }

    void CAircraftWing::update(const double &t, const double &fDT)
    {
        updateGeometry();

        // if (isControlSurface())
        {
            calculateModifiedSections();
            calculateAeroProperties();
        }

        calculateRelativeWind(fDT);
        calculateRelativeWindReverse(fDT);

        calculateAlpha();
        calculateAlphaReversed();
        // calculateAlphaDebug(t, fDT);
        calculateStallControl();

        calculateForces(t, fDT);
        // calculateForcesSimple(fDT);

        updateForcesFast(t, fDT);

        auto sectionCount = m_sectionCount * m_parentAircraft->getSectionMultiplier();

        for(s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx)
        {
            if(m_parentAircraft->getDisplayDebugData())
            {
                auto a = m_modifiedSectionA[sectionIdx];
                auto b = m_modifiedSectionB[sectionIdx];
                auto c = m_modifiedSectionC[sectionIdx];
                auto d = m_modifiedSectionD[sectionIdx];

                DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId( sectionIdx, 4 ), a, b, 0x00FF00);
                DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId( sectionIdx, 5 ), b, c, 0xFF0000);
                DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId( sectionIdx, 6 ), c, d, 0x0000FF);
                DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId( sectionIdx, 7 ), d, a, 0x00FFFF);
            }
        }
    }

    void CAircraftWing::calculateStallControl()
    {
        auto aircraft = m_parentAircraft;
        auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();
        auto attachedControlSurface = m_controlSurface;

        for(s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx)
        {
            if(isControlSurface())
            {
                if(m_controlSurface)
                {
                    if(!m_controlSurface->isAffectedSection(sectionIdx))
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
            auto angleOfAttackFull = m_aoa[sectionIdx];

            auto fStallControlCL = static_cast<real_Num>(0.0);
            auto fStallControlCD = static_cast<real_Num>(0.0);
            auto fStallControlCM = static_cast<real_Num>(0.0);

#if defined _DEBUG
            if(!Math<real_Num>::equals(m_stallControlCL, 0.0))
            {
                int stop = 0;
                stop = 0;
            }
#endif

            auto chordLineDotRelWindFull = chordLine.dotProduct(-relativeWindNormalised);
            auto aoaNormalised = Math<real_Num>::clamp(
                angleOfAttackFull / static_cast<real_Num>(180.0), -1.0, 1.0);

            if(chordLineDotRelWindFull > std::numeric_limits<real_Num>::epsilon())
            {
                if(attachedControlSurface != nullptr)
                {
                    if(isControlSurface())
                    {
                        auto stallThreshold = m_controlSurface->getStallThreshold();
                        auto stallControlCL = m_controlSurface->getStallControlCL();
                        auto stallControlCD = m_controlSurface->getStallControlCD();
                        auto stallControlCM = m_controlSurface->getStallControlCM();

                        if(aoaNormalised > stallThreshold)
                        {
                            auto range = static_cast<real_Num>(1.0) - stallThreshold;
                            auto fUpDotRelWind = aoaNormalised - stallThreshold;

                            if(range > std::numeric_limits<real_Num>::epsilon())
                            {
                                fStallControlCL = (fUpDotRelWind / range) * stallControlCL;
                                fStallControlCD = (fUpDotRelWind / range) * stallControlCD;
                                fStallControlCM = (fUpDotRelWind / range) * stallControlCM;
                            }
                        }
                        else if(aoaNormalised < -stallThreshold)
                        {
                            auto range = static_cast<real_Num>(1.0) - stallThreshold;
                            auto fUpDotRelWind = aoaNormalised + stallThreshold;

                            if(range > std::numeric_limits<real_Num>::epsilon())
                            {
                                fStallControlCL = (fUpDotRelWind / range) * stallControlCL;
                                fStallControlCD = (fUpDotRelWind / range) * stallControlCD;
                                fStallControlCM = (fUpDotRelWind / range) * stallControlCM;
                            }
                        }
                    }
                    else
                    {
                        auto stallThreshold = m_stallThreshold;
                        auto stallControlCL = m_stallControlCL;
                        auto stallControlCD = m_stallControlCD;
                        auto stallControlCM = m_stallControlCM;

                        if(aoaNormalised > stallThreshold)
                        {
                            auto range = static_cast<real_Num>(1.0) - stallThreshold;
                            auto fUpDotRelWind = aoaNormalised - stallThreshold;

                            if(range > std::numeric_limits<real_Num>::epsilon())
                            {
                                fStallControlCL = (fUpDotRelWind / range) * stallControlCL;
                                fStallControlCD = (fUpDotRelWind / range) * stallControlCD;
                                fStallControlCM = (fUpDotRelWind / range) * stallControlCM;
                            }
                        }
                        else if(aoaNormalised < -stallThreshold)
                        {
                            auto range = static_cast<real_Num>(1.0) - stallThreshold;
                            auto fUpDotRelWind = aoaNormalised + stallThreshold;

                            if(range > std::numeric_limits<real_Num>::epsilon())
                            {
                                fStallControlCL = (fUpDotRelWind / range) * stallControlCL;
                                fStallControlCD = (fUpDotRelWind / range) * stallControlCD;
                                fStallControlCM = (fUpDotRelWind / range) * stallControlCM;
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

    void CAircraftWing::calculateForcesSimple(double fDT)
    {
        SmartPtr<IVehicleBody> aircraftBody = m_parent;
        auto aircraft = m_parentAircraft;

        auto attachedControlSurface = m_controlSurface;
        auto propwash = m_propWash;
        auto aerofoil = m_aerofoil;

        constexpr float debugLineScale = 5.0f; // 1.0f / 30.0f;

        s32 lineId = 0;

        auto airDensity = aircraft->getAirDensity();

        auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

        // Per section update.
        for(s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx)
        {
            if(attachedControlSurface)
            {
                if(isControlSurface())
                {
                    if(!attachedControlSurface->isAffectedSection(sectionIdx))
                    {
                        continue;
                    }
                }
            }

            const auto area = m_area[sectionIdx];
            const auto chordLine = m_chordLines[sectionIdx];
            const auto chordLength = m_chordLengths[sectionIdx];
            const auto relativeWind = m_relativeWind[sectionIdx];
            const auto alpha = m_aoa[sectionIdx];

            auto relativeWindLength = relativeWind.length();
            auto relativeWindNormalised = relativeWind.normaliseCopy();

            if(aircraft->getDisplayDebugData())
            {
                auto a = m_modifiedSectionA[sectionIdx];
                auto b = m_modifiedSectionB[sectionIdx];
                auto c = m_modifiedSectionC[sectionIdx];
                auto d = m_modifiedSectionD[sectionIdx];

                DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId( sectionIdx, 4 ), a, b, 0x00FF00);
                DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId( sectionIdx, 5 ), b, c, 0xFF0000);
                DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId( sectionIdx, 6 ), c, d, 0x0000FF);
                DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId( sectionIdx, 7 ), d, a, 0x00FFFF);
            }

            auto totalLift = static_cast<real_Num>(0.0);
            auto totalDrag = static_cast<real_Num>(0.0);
            auto totalPitch = static_cast<real_Num>(0.0);

            auto clGroundEffectMult = static_cast<real_Num>(1.0);
            auto cdGroundEffectMult = static_cast<real_Num>(1.0);

            auto fStallControlCL = m_fStallControlCL[sectionIdx];
            auto fStallControlCD = m_fStallControlCD[sectionIdx];
            auto fStallControlCM = m_fStallControlCM[sectionIdx];

            if(m_groundEffect)
            {
                auto a = m_modifiedSectionA[sectionIdx];
                auto b = m_modifiedSectionB[sectionIdx];
                auto c = m_modifiedSectionC[sectionIdx];
                auto d = m_modifiedSectionD[sectionIdx];

                m_groundEffect->getGroundEffectCoefficients(a, b, c, d, clGroundEffectMult,
                                                            cdGroundEffectMult);
            }

            {
                // WP_LOG_ERROR("No airfoil assigned.");

                // Fall back to basic l/d equations..
                // L = cl * a * 0.5f * r * v^2
                // Approximate Cl using the following formula - Cl = 2 * pi * angle (in radians)
                real_Num cL = static_cast<real_Num>(2.0) * Math<real_Num>::pi() *
                              Math<real_Num>::DegToRad(alpha);
                cL *= clGroundEffectMult;

                real_Num r = 1.29;
                real_Num v = relativeWind.length();
                totalLift = cL * area * static_cast<real_Num>(0.5) * r * (v * v);

                static real_Num maxLift = 100.0;
                totalLift = MathF::clamp(totalLift, -maxLift, maxLift);

                // D = 0.5f * cd * r * v2 * a;
                // Typical aerofoil drag co efficient is .045;
                real_Num cD = m_cdOverride; // Typical aerofoil drag co efficient
                cD *= cdGroundEffectMult;

                totalDrag = static_cast<real_Num>(0.5) * cD * r * (v * v) * area;

                static real_Num maxDrag = 100.0;
                totalDrag = Math<real_Num>::clamp(totalDrag, -maxDrag, maxDrag);

                totalPitch = static_cast<real_Num>(0.0);

                WP_ASSERT(cL < static_cast<real_Num>( 1000.0 ));
                WP_ASSERT(cD < static_cast<real_Num>( 1000.0 ));
            }

            m_totalLift[sectionIdx] = totalLift;
            m_totalDrag[sectionIdx] = totalDrag;
            m_totalPitch[sectionIdx] = totalPitch;
        }
    }

    void CAircraftWing::calculateForces(double t, double fDT)
    {
        SmartPtr<IVehicleBody> aircraftBody = m_parent;
        auto aircraft = m_parentAircraft;

        auto attachedControlSurface = m_controlSurface;
        auto propwash = m_propWash;
        auto aerofoil = m_aerofoil;

        constexpr float debugLineScale = 5.0f; // 1.0f / 30.0f;

        s32 lineId = 0;

        auto airDensity = aircraft->getAirDensity();

        auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

        // Per section update.
        for(s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx)
        {
            if(attachedControlSurface)
            {
                if(isControlSurface())
                {
                    if(!attachedControlSurface->isAffectedSection(sectionIdx))
                    {
                        continue;
                    }
                }
            }

            auto area = m_area[sectionIdx];
            auto chordLine = m_chordLines[sectionIdx];
            auto chordLength = m_chordLengths[sectionIdx];
            auto relativeWind = m_relativeWind[sectionIdx];
            auto alpha = m_aoa[sectionIdx];

            // if ((alpha > 70.0 && alpha < 110.0) || (alpha > -110.0 && alpha < -70.0))
            // if (relativeWind.Z() > 0.0)
            //{
            //	chordLine = m_chordLinesReverse[sectionIdx];
            //	chordLength = chordLine.length();
            //	relativeWind = m_relativeWindReversed[sectionIdx];
            //	alpha = m_aoaReverse[sectionIdx];
            // }

            auto relativeWindLength = relativeWind.length();
            auto relativeWindNormalised = relativeWind.normaliseCopy();

            auto totalLift = static_cast<real_Num>(0.0);
            auto totalDrag = static_cast<real_Num>(0.0);
            auto totalPitch = static_cast<real_Num>(0.0);

            auto clGroundEffectMult = static_cast<real_Num>(1.0);
            auto cdGroundEffectMult = static_cast<real_Num>(1.0);

            auto fStallControlCL = m_fStallControlCL[sectionIdx];
            auto fStallControlCD = m_fStallControlCD[sectionIdx];
            auto fStallControlCM = m_fStallControlCM[sectionIdx];

            if(m_groundEffect)
            {
                auto a = m_modifiedSectionA[sectionIdx];
                auto b = m_modifiedSectionB[sectionIdx];
                auto c = m_modifiedSectionC[sectionIdx];
                auto d = m_modifiedSectionD[sectionIdx];

                m_groundEffect->getGroundEffectCoefficients(a, b, c, d, clGroundEffectMult,
                                                            cdGroundEffectMult);
            }

            if(aerofoil)
            {
                auto CL = aerofoil->getCL();
                auto CD = aerofoil->getCD();
                auto CM = aerofoil->getCM();

                auto clZero = CL->interpolate(alpha);
                auto cdZero = CD->interpolate(alpha);
                auto cmZero = CM->interpolate(alpha);

                auto camber = static_cast<real_Num>(0.0);
                auto camberNormalised = static_cast<real_Num>(0.0);
                auto fCurrentDeflection = static_cast<real_Num>(0.0);

                if(m_controlSurface)
                {
                    fCurrentDeflection =
                        m_controlSurface->getCurrentDeflection() * static_cast<real_Num>(2.0);

                    if(m_controlSurface->isReversed())
                    {
                        camberNormalised = -fCurrentDeflection;
                        camber = -fCurrentDeflection;
                    }
                    else
                    {
                        camberNormalised = fCurrentDeflection;
                        camber = fCurrentDeflection;
                    }

                    camber = fCurrentDeflection > static_cast<real_Num>(0.0)
                                 ? Math<real_Num>::Abs(fCurrentDeflection) *
                                   m_controlSurface->getMaxDeflectionDegrees()
                                 : Math<real_Num>::Abs(fCurrentDeflection) *
                                   m_controlSurface->getMinDeflectionDegrees();
                }

                // Use aerofoil..
                // L = cl * a * 0.5f * r * v^2
                real_Num cL = 0.0;

                WP_ASSERT(CL);
                if(m_controlSurface && isControlSurface())
                {
                    auto clLookup = m_controlSurface->getClLookup();
                    auto clLookupMultiplier = clLookup->interpolate(camber);
                    auto clMultiplier = m_controlSurface->getClMultiplier();

                    cL = clZero * clMultiplier * clLookupMultiplier * static_cast<real_Num>(0.3);

                    if(fStallControlCL > std::numeric_limits<real_Num>::epsilon() ||
                       fStallControlCL < -std::numeric_limits<real_Num>::epsilon())
                    {
                        cL += ((clZero * static_cast<real_Num>(0.1)) *
                               Math<real_Num>::Abs(fStallControlCL) *
                               Math<real_Num>::Abs(fCurrentDeflection));
                    }
                }
                else if(m_controlSurface && !isControlSurface())
                {
                    if(fStallControlCL > std::numeric_limits<real_Num>::epsilon() ||
                       fStallControlCL < -std::numeric_limits<real_Num>::epsilon())
                    {
                        cL = clZero + ((clZero * static_cast<real_Num>(0.1)) *
                                       Math<real_Num>::Abs(fStallControlCL) *
                                       Math<real_Num>::Abs(fCurrentDeflection));
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

                totalLift = cL * area * static_cast<real_Num>(0.5) * airDensity *
                            Math<real_Num>::Sqr(relativeWindLength);

                // D = 0.5 * cd * r * (v * v) * a;
                WP_ASSERT(CD);
                real_Num cD = 0.0;
                if(m_controlSurface && isControlSurface())
                {
                    auto cdLookup = m_controlSurface->getCdLookup();
                    auto cdLookupValue = cdLookup->interpolate(camber);
                    auto cdMultiplier = m_controlSurface->getCdMultiplier();
                    cD = cdZero * cdMultiplier * cdLookupValue * static_cast<real_Num>(0.3);

                    if(fStallControlCD > std::numeric_limits<real_Num>::epsilon() ||
                       fStallControlCD < -std::numeric_limits<real_Num>::epsilon())
                    {
                        cD += ((cdZero * static_cast<real_Num>(0.1)) *
                               Math<real_Num>::Abs(fStallControlCD) *
                               Math<real_Num>::Abs(fCurrentDeflection));
                    }
                }
                else if(m_controlSurface && !isControlSurface())
                {
                    if(fStallControlCD > std::numeric_limits<real_Num>::epsilon() ||
                       fStallControlCD < -std::numeric_limits<real_Num>::epsilon())
                    {
                        cD = cdZero + ((cdZero * static_cast<real_Num>(0.1)) *
                                       Math<real_Num>::Abs(fStallControlCD) *
                                       Math<real_Num>::Abs(fCurrentDeflection));
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

                totalDrag = static_cast<real_Num>(0.5) * cD * airDensity *
                            (relativeWindLength * relativeWindLength) * area;

                auto cM = static_cast<real_Num>(0.0);
                WP_ASSERT(CM);
                if(m_controlSurface && isControlSurface())
                {
                    auto cmLookup = m_controlSurface->getCmLookup();
                    auto cmLookupValue = cmLookup->interpolate(camber);
                    auto cmMultiplier = m_controlSurface->getCmMultiplier();
                    cM = cmZero * cmMultiplier * cmLookupValue * static_cast<real_Num>(0.3);

                    if(fStallControlCM > std::numeric_limits<real_Num>::epsilon() ||
                       fStallControlCM < -std::numeric_limits<real_Num>::epsilon())
                    {
                        cM += ((cmZero * static_cast<real_Num>(0.1)) *
                               Math<real_Num>::Abs(fStallControlCM) *
                               Math<real_Num>::Abs(fCurrentDeflection));
                    }
                }
                else if(m_controlSurface && !isControlSurface())
                {
                    if(fStallControlCM > std::numeric_limits<real_Num>::epsilon() ||
                       fStallControlCM < -std::numeric_limits<real_Num>::epsilon())
                    {
                        cM = cmZero + ((cmZero * static_cast<real_Num>(0.1)) *
                                       Math<real_Num>::Abs(fStallControlCM) *
                                       Math<real_Num>::Abs(fCurrentDeflection));
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

                // Find wing pitching moment...
                //  0.5 RoAir*Sqr(Flowpeed)
                auto q = static_cast<real_Num>(0.5) * airDensity *
                         (relativeWindLength * relativeWindLength);
                totalPitch = cM * chordLength * q * area;

                WP_ASSERT(Math<real_Num>::Abs( totalLift ) < static_cast<real_Num>( 10000.0 ));
                WP_ASSERT(Math<real_Num>::Abs( totalDrag ) < static_cast<real_Num>( 10000.0 ));

                WP_ASSERT(Math<real_Num>::Abs( cL ) < static_cast<real_Num>( 1000.0 ));
                WP_ASSERT(Math<real_Num>::Abs( cD ) < static_cast<real_Num>( 1000.0 ));
                WP_ASSERT(Math<real_Num>::Abs( cM ) < static_cast<real_Num>( 1000.0 ));
            }
            else
            {
                // WP_LOG_ERROR("No airfoil assigned.");

                // Fall back to basic l/d equations..
                // L = cl * a * 0.5f * r * v^2
                // Approximate Cl using the following formula - Cl = 2 * pi * angle (in radians)
                real_Num cL = static_cast<real_Num>(0.75) * Math<real_Num>::pi() *
                              Math<real_Num>::DegToRad(alpha);
                cL *= clGroundEffectMult;
                cL *= m_clMultiplier;

                real_Num r = 1.29;
                real_Num v = relativeWind.length();
                totalLift = cL * area * static_cast<real_Num>(0.5) * r * (v * v);

                static real_Num maxLift = 100.0;
                totalLift = MathF::clamp(totalLift, -maxLift, maxLift);

                // D = 0.5f * cd * r * v2 * a;
                // Typical aerofoil drag co efficient is .045;
                real_Num cD = m_cdOverride; // Typical aerofoil drag co efficient
                cD *= cdGroundEffectMult;
                cD *= m_cdMultiplier;

                totalDrag = static_cast<real_Num>(0.25) * cD * r * (v * v) * area;

                static real_Num maxDrag = 100.0;
                totalDrag = Math<real_Num>::clamp(totalDrag, -maxDrag, maxDrag);

                totalPitch = static_cast<real_Num>(0.0);

                WP_ASSERT(cL < static_cast<real_Num>( 1000.0 ));
                WP_ASSERT(cD < static_cast<real_Num>( 1000.0 ));
            }

            m_totalLift[sectionIdx] = totalLift;
            m_totalDrag[sectionIdx] = totalDrag;
            m_totalPitch[sectionIdx] = totalPitch;
        }

        // if (m_nextLiftText < t)
        //{
        //	if (getName() == "FUSELAGE(Clone)")
        //	{
        //		WP_LOG("Total lift: " + StringUtil::toString(m_totalLift[0]));
        //		m_nextLiftText = t + 3.0;
        //	}
        // }
    }

    void CAircraftWing::updateForcesFast(double t, double fDT)
    {
        auto airDensity = m_parentAircraft->getAirDensity();
        auto totalLiftForce = Vector3<real_Num>::zero();
        auto totalDragForce = Vector3<real_Num>::zero();
        auto totalPitchAxis = Vector3<real_Num>::zero();
        auto totalAerodynamicCenter = Vector3<real_Num>::zero();

        for(s32 sectionIdx = 0; sectionIdx < m_sectionCount; ++sectionIdx)
        {
            if(isControlSurface())
            {
                if(m_controlSurface)
                {
                    if(!m_controlSurface->isAffectedSection(sectionIdx))
                    {
                        continue;
                    }
                }
            }

            auto alpha = m_aoa[sectionIdx];
            auto aerodynamicCenter = m_aerodynamicCenters[sectionIdx];
            auto relativeWind = m_relativeWind[sectionIdx];
            auto relativeWindLength = relativeWind.length();
            auto relativeWindNormalised = relativeWind.normaliseCopy();

            // if ((alpha > 70.0 && alpha < 110.0) || (alpha > -110.0 && alpha < -70.0))
            // if (relativeWind.Z() > 0.0)
            //{
            //	alpha = m_aoaReverse[sectionIdx];
            //	aerodynamicCenter = m_aerodynamicCentersReverse[sectionIdx];
            //	relativeWind = m_relativeWindReversed[sectionIdx];
            //	relativeWindLength = relativeWind.length();
            //	relativeWindNormalised = relativeWind.normaliseCopy();
            // }

            const auto totalLift = m_totalLift[sectionIdx];
            const auto totalDrag = m_totalDrag[sectionIdx];
            const auto totalPitch = m_totalPitch[sectionIdx];

            // Build Lift vector.
            auto liftVector = m_localWingRight.crossProduct(-relativeWindNormalised);
            liftVector.normalise();
            auto liftForce = liftVector * totalLift;

            // Drag vector.
            auto dragForce = relativeWindNormalised * totalDrag;

            // Pitch
            auto pitchAxis = m_localWingRight * -totalPitch;

            WP_ASSERT(Math<real_Num>::Abs( liftForce.Z() ) < static_cast<real_Num>( 2000.0 ));
            WP_ASSERT(Math<real_Num>::Abs( dragForce.Y() ) < static_cast<real_Num>( 2000.0 ));

            totalLiftForce += liftForce;
            totalDragForce += dragForce;
            totalPitchAxis += pitchAxis;
            totalAerodynamicCenter += aerodynamicCenter;

            // if (m_parentAircraft->getEnablePowerUnit())
            //{
            //	m_parent->addLocalForceAtLocalPosition(liftForce, aerodynamicCenter);
            //	m_parent->addLocalForceAtLocalPosition(dragForce, aerodynamicCenter);
            //	m_parent->addLocalTorque(pitchAxis);
            // }
        }

        totalAerodynamicCenter = totalAerodynamicCenter / static_cast<real_Num>(m_sectionCount);

        if(m_parentAircraft->getEnablePowerUnit())
        {
            m_parent->addLocalForceAtLocalPosition(totalLiftForce, totalAerodynamicCenter);
            m_parent->addLocalForceAtLocalPosition(totalDragForce, totalAerodynamicCenter);
            m_parent->addLocalTorque(totalPitchAxis);
        }

        if(m_parentAircraft->getDisplayDebugData())
        {
            DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId( 0 ), totalAerodynamicCenter,
                                        totalAerodynamicCenter + ( totalLiftForce * 0.3 ),
                                        0x00FF00);
            DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId( 1 ), totalAerodynamicCenter,
                                        totalAerodynamicCenter + ( totalDragForce * 0.3 ),
                                        0xFF0000);
        }
    }

    void CAircraftWing::calculateAlphaDebug(double t, double dt)
    {
        for(s32 sectionIdx = 0; sectionIdx < m_sectionCount; ++sectionIdx)
        {
            if(isControlSurface())
            {
                if(m_controlSurface)
                {
                    if(!m_controlSurface->isAffectedSection(sectionIdx))
                    {
                        continue;
                    }
                }
            }

            const auto chordLine = m_chordLines[sectionIdx];
            const auto relativeWind = m_relativeWind[sectionIdx];
            const auto up = m_up[sectionIdx];

            auto relativeWindNormalised = relativeWind.normaliseCopy();

            auto rightFlow = m_localWingRight.dotProduct(-relativeWindNormalised);
            auto chordFlow = chordLine.dotProduct(-relativeWindNormalised);
            auto normFlow = up.dotProduct(-relativeWindNormalised);

            auto angleOfAttackRadsFull = Math<real_Num>::ATan2(normFlow, chordFlow);

            auto AngleOfAttack = chordLine.dotProduct(-relativeWindNormalised);
            AngleOfAttack = Math<real_Num>::clamp(AngleOfAttack, -1.0f, 1.0f);
            AngleOfAttack = Math<real_Num>::ACos(AngleOfAttack);

            float yAxisDotRelativeWind = up.dotProduct(relativeWindNormalised);
            if(yAxisDotRelativeWind < 0.0f)
            {
                AngleOfAttack = -AngleOfAttack;
            }

            // if (yAxisDotRelativeWind < 0.0f)
            //{
            //	angleOfAttackRadsFull = -angleOfAttackRadsFull;
            // }

            auto alpha = static_cast<real_Num>(0.0);

            if(chordFlow > std::numeric_limits<real_Num>::epsilon() ||
               chordFlow < -std::numeric_limits<real_Num>::epsilon())
            {
                if(chordFlow > static_cast<real_Num>(0.0))
                {
                    alpha = Math<real_Num>::Atan(normFlow / chordFlow);
                }
                else
                {
                    if(normFlow > 0)
                    {
                        alpha = Math<real_Num>::pi() + Math<real_Num>::Atan(normFlow / chordFlow);
                    }
                    else
                    {
                        alpha = -Math<real_Num>::pi() + Math<real_Num>::Atan(normFlow / chordFlow);
                    }
                }
            }
            else
            {
                if(normFlow > 0)
                {
                    alpha = Math<real_Num>::pi();
                }
                else
                {
                    alpha = -Math<real_Num>::pi();
                }
            }

            if(chordFlow > 0.1)
            {
                // WP_ASSERT(Math<real_Num>::equals(alpha, -AngleOfAttack));
                // WP_ASSERT(Math<real_Num>::equals(angleOfAttackRadsFull, alpha));
            }

            angleOfAttackRadsFull = alpha;

            // if (getName() == "WING LOWER LEFT" ||
            //	getName() == "WING LOWER LEFT_ControlSurface" ||
            //	getName() == "WING LOWER RIGHT" ||
            //	getName() == "WING LOWER RIGHT_ControlSurface")
            //{
            AngleOfAttack = -AngleOfAttack;
            angleOfAttackRadsFull = AngleOfAttack;
            //}

            // if (getName() == "WING LOWER LEFT" ||
            //	getName() == "WING LOWER RIGHT")
            //{
            // AngleOfAttack = -AngleOfAttack;

            //	//if (Math<real_Num>::Abs(angleOfAttackRadsFull - AngleOfAttack) > 0.1)
            //	//{
            //	//	WP_LOG(StringUtil::toString(angleOfAttackRadsFull - AngleOfAttack));
            //	//}

            // angleOfAttackRadsFull = AngleOfAttack;
            // }

            // if (!(getName() == "FUSELAGE" || getName() == "FUSELAGE(Clone)" ||
            //	getName() == "RUDDER" ||
            //	getName() == "RUDDER_ControlSurface" ||
            //	getName() == "STAB LEFT" ||
            //	getName() == "STAB LEFT_ControlSurface" ||
            //	getName() == "STAB RIGHT" ||
            //	getName() == "STAB RIGHT_ControlSurface" ))
            //{
            //	AngleOfAttack = -AngleOfAttack;
            //	angleOfAttackRadsFull = AngleOfAttack;
            // }
            // else
            //{
            //	angleOfAttackRadsFull = alpha;
            // }

            // AngleOfAttack = -AngleOfAttack;
            // angleOfAttackRadsFull = AngleOfAttack;
            // angleOfAttackRadsFull = alpha;

            if(isControlSurface())
            {
                angleOfAttackRadsFull *= m_aoaMultiplier;
            }
            else
            {
                if(m_controlSurface)
                {
                    if(m_controlSurface->isAffectedSection(sectionIdx))
                    {
                        angleOfAttackRadsFull *= m_controlSurface->getAoAMultiplier();
                    }
                }
            }

            // if (getName().find("FUSELAGE(Clone") != std::string::npos)
            //{
            //	if (normFlow > 0.1)
            //	{
            //		int stop = 0;
            //		stop = 0;
            //		if (Math<real_Num>::Abs(angleOfAttackRadsFull - AngleOfAttack) > 0.05)
            //		{
            //			WP_LOG(StringUtil::toString(angleOfAttackRadsFull - AngleOfAttack));
            //		}
            //	}
            //	else if (normFlow < -0.1)
            //	{
            //		int stop = 0;
            //		stop = 0;
            //		if (Math<real_Num>::Abs(angleOfAttackRadsFull - AngleOfAttack) > 0.05)
            //		{
            //			WP_LOG(StringUtil::toString(angleOfAttackRadsFull - AngleOfAttack));
            //		}
            //	}
            // }

            auto angleOfAttackFull = Math<real_Num>::RadToDeg(angleOfAttackRadsFull);
            angleOfAttackFull = Math<real_Num>::wrap(
                angleOfAttackFull, -static_cast<real_Num>(180.0), 180.0);

            // if (getName().find("FUSELAGE(Clone)") != std::string::npos)
            //{
            //	int stop = 0;
            //	stop = 0;

            //	static auto maxAoa = real_Num(-1e10);
            //	static auto minAoa = real_Num(1e10);

            //	if (angleOfAttackFull < minAoa)
            //	{
            //		minAoa = angleOfAttackFull;
            //	}

            //	if (angleOfAttackFull > maxAoa)
            //	{
            //		maxAoa = angleOfAttackFull;
            //	}

            //	//if (m_parentAircraft->getDisplayDebugData())
            //	//{
            //	//	DEBUG_DRAW_LOCAL_LINE_BY_ID(getDebugId(0, 9), aerodynamicCenter,
            //	//		liftForce + (liftForce * 0.3), 0x00FF00);
            //	//}
            //}

            m_aoa[sectionIdx] = angleOfAttackFull;

            // if (getName() == "FUSELAGE(Clone)")
            //{
            //	int stop = 0;
            //	stop = 0;
            // }
        }

        if(m_nextLiftText < t)
        {
            if(getName() == "FUSELAGE(Clone)")
            {
                WP_LOG("Total lift: " + StringUtil::toString( m_aoa[0] ));
                m_nextLiftText = t + 3.0;
            }
        }
    }

    void CAircraftWing::calculateAlphaReversed()
    {
        auto bIsControlSurface = isControlSurface();

        for(s32 sectionIdx = 0; sectionIdx < m_sectionCount; ++sectionIdx)
        {
            if(isControlSurface())
            {
                if(m_controlSurface)
                {
                    if(!m_controlSurface->isAffectedSection(sectionIdx))
                    {
                        continue;
                    }
                }
            }

            auto chordLine = m_chordLinesReverse[sectionIdx];
            auto relativeWind = m_relativeWindReversed[sectionIdx];
            auto up = m_upReverse[sectionIdx];
            auto relativeWindNormalised = relativeWind.normaliseCopy();

            auto chordFlow = chordLine.dotProduct(-relativeWindNormalised);
            auto upFlow = up.dotProduct(-relativeWindNormalised);
            auto alpha = Math<real_Num>::ATan2(upFlow, chordFlow);

            if(bIsControlSurface)
            {
                alpha *= m_aoaMultiplier;
            }
            else
            {
                if(m_controlSurface)
                {
                    if(m_controlSurface->isAffectedSection(sectionIdx))
                    {
                        alpha *= m_controlSurface->getAoAMultiplier();
                    }
                }
            }

            auto alphaDegrees = Math<real_Num>::RadToDeg(alpha);
            m_aoaReverse[sectionIdx] = Math<real_Num>::wrap(
                alphaDegrees, -static_cast<real_Num>(180.0), 180.0);
        }
    }

    void CAircraftWing::calculateAlpha()
    {
        auto bIsControlSurface = isControlSurface();

        for(s32 sectionIdx = 0; sectionIdx < m_sectionCount; ++sectionIdx)
        {
            if(isControlSurface())
            {
                if(m_controlSurface)
                {
                    if(!m_controlSurface->isAffectedSection(sectionIdx))
                    {
                        continue;
                    }
                }
            }

            auto chordLine = m_chordLines[sectionIdx];
            auto relativeWind = m_relativeWind[sectionIdx];
            auto up = m_up[sectionIdx];
            auto relativeWindNormalised = relativeWind.normaliseCopy();

            auto chordFlow = chordLine.dotProduct(-relativeWindNormalised);
            auto upFlow = up.dotProduct(-relativeWindNormalised);
            auto alpha = Math<real_Num>::ATan2(upFlow, chordFlow);

            if(bIsControlSurface)
            {
                alpha *= m_aoaMultiplier;
            }
            else
            {
                if(m_controlSurface)
                {
                    if(m_controlSurface->isAffectedSection(sectionIdx))
                    {
                        alpha *= m_controlSurface->getAoAMultiplier();
                    }
                }
            }

            auto alphaDegrees = Math<real_Num>::RadToDeg(alpha);
            m_aoa[sectionIdx] = Math<real_Num>::wrap(alphaDegrees, -static_cast<real_Num>(180.0),
                                                     180.0);
        }
    }

    void CAircraftWing::calculateArea()
    {
        auto sectionCount = m_sectionCount;
        for(s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx)
        {
            const auto a = m_sectionA[sectionIdx];
            const auto b = m_sectionB[sectionIdx];
            const auto c = m_sectionC[sectionIdx];
            const auto d = m_sectionD[sectionIdx];

            m_area[sectionIdx] = calculateArea(a, b, c, d);
        }
    }

    void CAircraftWing::calculateAeroProperties()
    {
        auto aircraftBody = m_parent;
        auto aircraft = m_parentAircraft;

        auto localTransform = getLocalTransform();
        auto localScale = localTransform.getScale();

        auto attachedControlSurface = m_controlSurface;
        auto propwash = m_propWash;
        auto aerofoil = m_aerofoil;

        auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

        for(s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx)
        {
            const auto a = m_modifiedSectionA[sectionIdx];
            const auto b = m_modifiedSectionB[sectionIdx];
            const auto c = m_modifiedSectionC[sectionIdx];
            const auto d = m_modifiedSectionD[sectionIdx];

            auto chordLine = (a + ((b - a) * 0.5)) -
                             (d + ((c - d) * 0.5));
            auto chordLength = chordLine.length();
            chordLine.normalise();

#if 1
            auto alpha = m_aoa[sectionIdx];
            // static auto alpha = 70.0;

            auto liftLineChordPositionTransition = m_liftLineChordPosition;
            // Chordwise position of the lift line during transition period

            if(alpha >= m_liftLineAlphaStart && alpha <= m_liftLineAlphaEnd)
            {
                auto t =
                    (alpha - m_liftLineAlphaStart) / (m_liftLineAlphaEnd - m_liftLineAlphaStart);
                t = Math<real_Num>::Abs(t);
                t = Math<real_Num>::clamp01(t);

                if(m_liftLineChordPosition < m_liftLineChordPositionReversed)
                {
                    liftLineChordPositionTransition =
                        m_liftLineChordPosition -
                        (m_liftLineChordPositionReversed - m_liftLineChordPosition) * t;
                }
                else
                {
                    liftLineChordPositionTransition =
                        m_liftLineChordPosition -
                        (m_liftLineChordPosition - m_liftLineChordPositionReversed) * t;
                }
            }
            else if(alpha >= -m_liftLineAlphaEnd && alpha <= -m_liftLineAlphaStart)
            {
                auto t =
                    (alpha + m_liftLineAlphaStart) / (m_liftLineAlphaEnd - m_liftLineAlphaStart);
                t = Math<real_Num>::Abs(t);
                t = Math<real_Num>::clamp01(t);

                if(m_liftLineChordPosition < m_liftLineChordPositionReversed)
                {
                    liftLineChordPositionTransition =
                        m_liftLineChordPosition -
                        (m_liftLineChordPositionReversed - m_liftLineChordPosition) * t;
                }
                else
                {
                    liftLineChordPositionTransition =
                        m_liftLineChordPosition -
                        (m_liftLineChordPosition - m_liftLineChordPositionReversed) * t;
                }
            }
            else if((alpha >= m_liftLineAlphaEnd && alpha <= 180) ||
                    (alpha >= -180 && alpha <= -m_liftLineAlphaEnd))
            {
                liftLineChordPositionTransition = m_liftLineChordPositionReversed;
            }

            auto sectionRootLiftPositionFull = d + ((a - d) * liftLineChordPositionTransition);
            auto sectionTipLiftPositionFull = c + ((b - c) * liftLineChordPositionTransition);

#else
            auto sectionRootLiftPositionFull = d + ((a - d) * m_liftLineChordPosition);
            auto sectionTipLiftPositionFull = c + ((b - c) * m_liftLineChordPosition);

            auto relativeWind = m_relativeWind[sectionIdx];
            if(relativeWind.length() > 0.0)
            {
                auto relativeWindNormalised = relativeWind.normaliseCopy();
                auto threshold = 0.3;
                auto chordLineDotRelativeWind = chordLine.dotProduct(relativeWindNormalised);

                if(chordLineDotRelativeWind > -threshold && chordLineDotRelativeWind < threshold)
                {
                    auto liftLineChordPos =
                        m_liftLineChordPosition -
                        (m_liftLineChordPositionReversed - m_liftLineChordPosition) *
                        (chordLineDotRelativeWind / (threshold * 2.0));
                    sectionRootLiftPositionFull = d + ((a - d) * liftLineChordPos);
                    sectionTipLiftPositionFull = c + ((b - c) * liftLineChordPos);
                }
                else if(chordLineDotRelativeWind > 0.0)
                {
                    sectionRootLiftPositionFull =
                        d + ((a - d) * m_liftLineChordPositionReversed);
                    sectionTipLiftPositionFull = c + ((b - c) * m_liftLineChordPositionReversed);
                }
            }
            else
            {
                sectionRootLiftPositionFull = d + ((a - d) * m_liftLineChordPosition);
                sectionTipLiftPositionFull = c + ((b - c) * m_liftLineChordPosition);
            }
#endif

            auto vUp = sectionTipLiftPositionFull - sectionRootLiftPositionFull;
            auto up = chordLine.crossProduct(vUp.normaliseCopy());

            if(localScale.X() < static_cast<real_Num>(0.0))
            {
                up = -up;
            }

            up.normalise();

            auto aerodynamicCenter = sectionRootLiftPositionFull +
                                     ((sectionTipLiftPositionFull - sectionRootLiftPositionFull) *
                                      0.5);

            m_chordLines[sectionIdx] = chordLine;
            m_chordLengths[sectionIdx] = chordLength;
            m_up[sectionIdx] = up;
            m_aerodynamicCenters[sectionIdx] = aerodynamicCenter;
        }

        for(s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx)
        {
            const auto a = m_modifiedSectionA[sectionIdx];
            const auto b = m_modifiedSectionB[sectionIdx];
            const auto c = m_modifiedSectionC[sectionIdx];
            const auto d = m_modifiedSectionD[sectionIdx];

            auto sectionRootLiftPositionFull = d + ((a - d) * m_liftLineChordPositionReversed);
            auto sectionTipLiftPositionFull = c + ((b - c) * m_liftLineChordPositionReversed);

            auto chordLine = (a + ((b - a) * 0.5)) -
                             (d + ((c - d) * 0.5));
            auto chordLength = chordLine.length();
            chordLine.normalise();

            auto vUp = sectionTipLiftPositionFull - sectionRootLiftPositionFull;
            auto up = chordLine.crossProduct(vUp.normaliseCopy());

            if(localScale.X() < static_cast<real_Num>(0.0))
            {
                up = -up;
            }

            up.normalise();

            auto aerodynamicCenter = sectionRootLiftPositionFull +
                                     ((sectionTipLiftPositionFull - sectionRootLiftPositionFull) *
                                      0.5);

            m_chordLinesReverse[sectionIdx] = chordLine;
            m_upReverse[sectionIdx] = up;
            m_aerodynamicCentersReverse[sectionIdx] = aerodynamicCenter;
        }
    }

    void CAircraftWing::calculateModifiedSections()
    {
        auto aircraft = m_parentAircraft;
        auto attachedControlSurface = m_controlSurface;
        auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

        for(s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx)
        {
            auto a = m_sectionA[sectionIdx];
            auto b = m_sectionB[sectionIdx];
            auto c = m_sectionC[sectionIdx];
            auto d = m_sectionD[sectionIdx];

            if(attachedControlSurface)
            {
                if(isControlSurface())
                {
                    attachedControlSurface->modifyWingGeometry(sectionIdx, a, b, c, d);
                }
            }

            m_modifiedSectionA[sectionIdx] = a;
            m_modifiedSectionB[sectionIdx] = b;
            m_modifiedSectionC[sectionIdx] = c;
            m_modifiedSectionD[sectionIdx] = d;
        }
    }

    void CAircraftWing::updatePoints()
    {
        auto aircraft = m_parentAircraft;

        auto sectionCount = m_sectionCount * aircraft->getSectionMultiplier();

        auto localWingRootLeadingEdge = m_rootLeadingEdge;
        auto localWingRootTrailingEdge = m_rootTrailingEdge;
        auto localWingTipLeadingEdge = m_tipLeadingEdge;
        auto localWingTipTrailingEdge = m_tipTrailingEdge;
        // auto localRootLiftPosition = m_rootLiftPosition;
        // auto localTipLiftPosition = m_tipLiftPosition;
        auto liftLineChordPosition = m_liftLineChordPosition;

        for(s32 sectionIdx = 0; sectionIdx < sectionCount; ++sectionIdx)
        {
            // R A-----------------B (Leading edge)
            // O |                 |
            // O |                 |
            // T D-----------------C (Trailing edge
            auto rootDistance = static_cast<real_Num>(0.0);
            auto tipDistance = static_cast<real_Num>(0.0);

            auto attachedControlSurface = getAttachedControlSurface();
            if(attachedControlSurface)
            {
                if(attachedControlSurface->isAffectedSection(sectionIdx))
                {
                    if(!isControlSurface())
                    {
                        rootDistance =
                            attachedControlSurface->getRootHingeDistanceFromTrailingEdge();
                        tipDistance = attachedControlSurface->getTipHingeDistanceFromTrailingEdge();

                        localWingRootTrailingEdge =
                            m_rootTrailingEdge +
                            (m_rootLeadingEdge - m_rootTrailingEdge) * rootDistance;
                        localWingTipTrailingEdge =
                            m_tipTrailingEdge +
                            (m_tipLeadingEdge - m_tipTrailingEdge) * tipDistance;
                    }
                    else
                    {
                        auto pMainWing = attachedControlSurface->getMainWing();
                        auto mainWing = workphone::static_pointer_cast<CAircraftWing>(pMainWing);
                        if(mainWing)
                        {
                            localWingRootTrailingEdge = mainWing->m_rootTrailingEdge;
                            localWingTipTrailingEdge = mainWing->m_tipTrailingEdge;
                        }
                    }
                }
            }

            // Find points a,b,c & d for this chunk of wing.
            m_sectionA[sectionIdx] =
                localWingRootLeadingEdge + ((localWingTipLeadingEdge - localWingRootLeadingEdge) *
                                            static_cast<real_Num>(sectionIdx) / sectionCount);
            m_sectionB[sectionIdx] = localWingRootLeadingEdge +
                                     ((localWingTipLeadingEdge - localWingRootLeadingEdge) *
                                      static_cast<real_Num>(sectionIdx + 1) / sectionCount);
            m_sectionC[sectionIdx] = localWingRootTrailingEdge +
                                     ((localWingTipTrailingEdge - localWingRootTrailingEdge) *
                                      static_cast<real_Num>(sectionIdx + 1) / sectionCount);
            m_sectionD[sectionIdx] = localWingRootTrailingEdge +
                                     ((localWingTipTrailingEdge - localWingRootTrailingEdge) *
                                      static_cast<real_Num>(sectionIdx) / sectionCount);
        }
    }

    SmartPtr<IAircraftControlSurface> CAircraftWing::getAttachedControlSurface() const
    {
        return m_controlSurface;
    }

    void CAircraftWing::setAttachedControlSurface(
        SmartPtr<IAircraftControlSurface> attachedControlSurface)
    {
        m_controlSurface = attachedControlSurface;

        if(!isControlSurface())
        {
            auto pThis = getSharedFromThis<IAircraftWing>();
            m_controlSurface->setMainWing(pThis);
        }
    }

    void CAircraftWing::load(void *pData)
    {
        // setData( pData );

        // auto data = static_cast<data::aircraft_wing_data *>(pData);

        // m_sectionCount = data->sectionCount * m_parentAircraft->getSectionMultiplier();

        // m_wingTipWidthZeroToOne = data->wingTipWidthZeroToOne;
        // m_wingTipSweep = data->wingTipSweep;
        // m_wingTipAngle = data->wingTipAngle;
        // m_cdOverride = data->cdOverride;
        // m_liftLineChordPosition = data->liftLineChordPosition;
        // m_liftLineChordPositionReversed = data->liftLineChordPositionReversed;

        // m_liftLineAlphaStart = data->liftLineAlphaStart;
        // m_liftLineAlphaEnd = data->liftLineAlphaEnd;

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
        //     auto aerofoil = fb::make_ptr<CAerofoil>();
        //     aerofoil->setReverseValues( false );
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
        //         StringUtil::toString( id ) + data->name + "_" + StringUtil::toString(
        //             static_cast<s32>(i) ) );
        // }

        // updateGeometry();

        // m_windRotation.X() = 10.0;

        setLoadingState(LoadingState::Loaded);
    }

    void CAircraftWing::unload(SmartPtr<ISharedObject> data)
    {
        setLoadingState(LoadingState::Unloading);
        m_controlSurface = nullptr;
        setLoadingState(LoadingState::Unloaded);
    }

    void CAircraftWing::load(SmartPtr<ISharedObject> data)
    {
    }

    SmartPtr<IAircraftPropWash> CAircraftWing::getAttachedPropWash() const
    {
        return m_propWash;
    }

    void CAircraftWing::setAttachedPropWash(SmartPtr<IAircraftPropWash> attachedPropWash)
    {
        WP_ASSERT(m_propWash == nullptr);

        m_propWash = attachedPropWash;

        WP_ASSERT(getName().find( m_propWash->getName() ) != String::npos);
    }

    void CAircraftWing::setControlSurface(bool bIsControlSurface)
    {
        m_isControlSurface = bIsControlSurface;
    }

    bool CAircraftWing::useCombinedControlSurface() const
    {
        return m_useCombinedControlSurface;
    }

    void CAircraftWing::setUserCombinedControlSurface(bool bUseCombinedControlSurface)
    {
        m_useCombinedControlSurface = bUseCombinedControlSurface;
    }

    void CAircraftWing::reset()
    {
        updateGeometry();
        updatePoints();
        calculateArea();
        calculateModifiedSections();
        calculateAeroProperties();
    }
}
