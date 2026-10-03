#ifndef ControlSurface_h__
#define ControlSurface_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/LinearSpline1.hpp>
#include <Workphone/Interface/Vehicle/IAircraftControlSurface.hpp>
#include <WPVehiclePhysics/CAircraftAttachment.hpp>

namespace workphone
{
    namespace vehicle
    {
        class WPVehiclePhysics_API CAircraftControlSurface
            : public CAircraftAttachment<IAircraftControlSurface>
        {
        public:
            CAircraftControlSurface();
            ~CAircraftControlSurface() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void load( void *pData ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void update( const double &t, const double &dt ) override;

            void updateTransform() override;

            void modifyWingGeometry( s32 SectionIndex, Vector3<real_Num> &pointA,
                                     Vector3<real_Num> &pointB, Vector3<real_Num> &pointC,
                                     Vector3<real_Num> &pointD ) override;

            void modifyMainWingGeometry( s32 SectionIndex, Vector3<real_Num> &pointA,
                                         Vector3<real_Num> &pointB, Vector3<real_Num> &pointC,
                                         Vector3<real_Num> &pointD, real_Num t ) override;
            void modifyWingGeometry( s32 SectionIndex, Vector3<real_Num> &pointA,
                                     Vector3<real_Num> &pointB, Vector3<real_Num> &pointC,
                                     Vector3<real_Num> &pointD, real_Num t ) override;

            s32  getSurfaceId() const override;
            void setSurfaceId( s32 surfaceId ) override;

            SmartPtr<InputController> getInputController() const;
            void                      setInputController( SmartPtr<InputController> inputController );

            real_Num getCurrentDeflection() const override;
            void     setCurrentDeflection( real_Num deflection ) override;

            bool isReversed() const override;
            void setReversed( bool reversed ) override;

            real_Num getTipHingeDistanceFromTrailingEdge() const override;
            void     setTipHingeDistanceFromTrailingEdge( real_Num tipHingeDistance ) override;

            real_Num getRootHingeDistanceFromTrailingEdge() const override;
            void     setRootHingeDistanceFromTrailingEdge( real_Num rootHingeDistance ) override;

            Vector3<real_Num> getRotationAxis() const;
            void              setRotationAxis( Vector3<real_Num> rotationAxis );

            Array<bool>       &getAffectedSections() override;
            const Array<bool> &getAffectedSections() const override;
            void               setAffectedSections( const Array<bool> &affectedSections ) override;

            bool isAffectedSection( int index ) const override;

            SmartPtr<LinearSpline1<real_Num>> getClLookup() const override;
            void setClLookup( SmartPtr<LinearSpline1<real_Num>> linearSpline1 ) override;

            SmartPtr<LinearSpline1<real_Num>> getCdLookup() const override;
            void setCdLookup( SmartPtr<LinearSpline1<real_Num>> linearSpline1 ) override;

            SmartPtr<LinearSpline1<real_Num>> getCmLookup() const override;
            void setCmLookup( SmartPtr<LinearSpline1<real_Num>> linearSpline1 ) override;

            real_Num getClMultiplier() const override;
            void     setClMultiplier( real_Num clMultiplier ) override;

            real_Num getCdMultiplier() const override;
            void     setCdMultiplier( real_Num cdMultiplier ) override;

            real_Num getCmMultiplier() const override;
            void     setCmMultiplier( real_Num cmMultiplier ) override;

            real_Num getMinDeflectionDegrees() const override;
            void     setMinDeflectionDegrees( real_Num minDeflectionDegrees ) override;

            real_Num getMaxDeflectionDegrees() const override;
            void     setMaxDeflectionDegrees( real_Num maxDeflectionDegrees ) override;

            real_Num getAoAMultiplier() const override;

            void setAoAMultiplier( real_Num aoaMultiplier ) override;

            SmartPtr<IAircraftWing> getMainWing() const override;
            void                    setMainWing( SmartPtr<IAircraftWing> mainWing ) override;

            SmartPtr<IAircraftWing> getControlWing() const override;
            void                    setControlWing( SmartPtr<IAircraftWing> controlWing ) override;

            bool useControlWing() const;
            void setUseControlWing( bool useControlWing );

            void updateGeometry() override;

            real_Num getStallControlCL() const override;
            void     setStallControlCL( real_Num stallControlCL ) override;

            real_Num getStallControlCD() const override;
            void     setStallControlCD( real_Num stallControlCD ) override;

            real_Num getStallControlCM() const override;
            void     setStallControlCM( real_Num stallControlCM ) override;

            real_Num getStallThreshold() const override;
            void     setStallThreshold( real_Num stallThreshold ) override;

            void reset() override;

        protected:
            SmartPtr<IAircraftWing> m_mainWing;
            SmartPtr<IAircraftWing> m_controlWing;

            SmartPtr<LinearSpline1<real_Num>> m_clLookup = nullptr;
            SmartPtr<LinearSpline1<real_Num>> m_cdLookup = nullptr;
            SmartPtr<LinearSpline1<real_Num>> m_cmLookup = nullptr;

            SmartPtr<InputController> m_inputController = nullptr;

            Vector3<real_Num>    m_rotationAxis = Vector3<real_Num>::zero();
            Quaternion<real_Num> m_initialModelRotation = Quaternion<real_Num>::identity();

            real_Num m_aoaMultiplier = static_cast<real_Num>( 1.0 );

            real_Num m_clMultiplier = static_cast<real_Num>( 1.0 );
            real_Num m_cdMultiplier = static_cast<real_Num>( 1.0 );
            real_Num m_cmMultiplier = static_cast<real_Num>( 1.0 );

            real_Num m_stallControlCL = static_cast<real_Num>( 0.0 );
            real_Num m_stallControlCD = static_cast<real_Num>( 0.0 );
            real_Num m_stallControlCM = static_cast<real_Num>( 0.0 );
            real_Num m_stallThreshold = static_cast<real_Num>( 1.0 );

            real_Num m_minDeflectionDegrees = static_cast<real_Num>( -90.0 );
            real_Num m_maxDeflectionDegrees = static_cast<real_Num>( 90.0 );

            real_Num m_rootHingeDistanceFromTrailingEdge = static_cast<real_Num>( 0.0 );
            real_Num m_tipHingeDistanceFromTrailingEdge = static_cast<real_Num>( 0.0 );
            real_Num m_currentDeflection = static_cast<real_Num>( 0.0 );

            s32  m_surfaceId = -1;
            bool m_controllable = true;
            bool m_reverse = false;
            bool m_useControlWing = false;

            Array<bool> m_affectedSections;
        };
    } // namespace vehicle
} // namespace workphone

#endif // ControlSurface_h__
