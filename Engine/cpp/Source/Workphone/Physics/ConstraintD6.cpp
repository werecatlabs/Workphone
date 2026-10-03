#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/ConstraintD6.hpp>
#include <Workphone/Interface/Physics/IConstraintDrive.hpp>
#include <Workphone/Interface/Physics/IConstraintLinearLimit.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone
{
    namespace physics
    {
        WP_CLASS_REGISTER_DERIVED( workphone::physics, ConstraintD6, PhysicsConstraint3<IConstraintD6> );

        ConstraintD6::ConstraintD6() :
            m_drivePosition( Transform3<real_Num>::identity() ),
            m_linearLimit( nullptr )
        {
            // Initialize drives array
            for( int i = 0; i < static_cast<int>( D6DriveEnum::eCOUNT ); ++i )
            {
                m_drives[i] = nullptr;
            }

            // Initialize motion types - default to locked for all axes
            for( int i = 0; i < static_cast<int>( D6AxisEnum::eCOUNT ); ++i )
            {
                m_motionTypes[i] = D6MotionEnum::eLOCKED;
            }
        }

        ConstraintD6::~ConstraintD6() = default;

        void ConstraintD6::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                PhysicsConstraint3<IConstraintD6>::load( data );
            }
            catch( Exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void ConstraintD6::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                // Clean up drives
                for( int i = 0; i < static_cast<int>( D6DriveEnum::eCOUNT ); ++i )
                {
                    if( m_drives[i] )
                    {
                        m_drives[i]->unload( nullptr );
                        m_drives[i] = nullptr;
                    }
                }

                // Clean up linear limit
                if( m_linearLimit )
                {
                    m_linearLimit->unload( nullptr );
                    m_linearLimit = nullptr;
                }

                PhysicsConstraint3<IConstraintD6>::unload( data );
            }
            catch( Exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        Array<SmartPtr<ISharedObject>> ConstraintD6::getChildObjects() const
        {
            auto objects = PhysicsConstraint3<IConstraintD6>::getChildObjects();

            // Add drives to child objects
            for( int i = 0; i < static_cast<int>( D6DriveEnum::eCOUNT ); ++i )
            {
                if( m_drives[i] )
                {
                    objects.push_back( m_drives[i] );
                }
            }

            // Add linear limit to child objects
            if( m_linearLimit )
            {
                objects.push_back( m_linearLimit );
            }

            return objects;
        }

        SmartPtr<Properties> ConstraintD6::getProperties() const
        {
            auto properties = PhysicsConstraint3<IConstraintD6>::getProperties();

            if( properties )
            {
                // Add D6-specific properties
                auto drivePos = getDrivePosition();
                properties->setProperty( "drivePosition.position.x",
                                         StringUtil::toString( drivePos.getPosition().X() ) );
                properties->setProperty( "drivePosition.position.y",
                                         StringUtil::toString( drivePos.getPosition().Y() ) );
                properties->setProperty( "drivePosition.position.z",
                                         StringUtil::toString( drivePos.getPosition().Z() ) );

                auto driveRot = drivePos.getOrientation();
                properties->setProperty( "drivePosition.rotation.x",
                                         StringUtil::toString( driveRot.X() ) );
                properties->setProperty( "drivePosition.rotation.y",
                                         StringUtil::toString( driveRot.Y() ) );
                properties->setProperty( "drivePosition.rotation.z",
                                         StringUtil::toString( driveRot.Z() ) );
                properties->setProperty( "drivePosition.rotation.w",
                                         StringUtil::toString( driveRot.W() ) );

                // Add motion type properties
                for( int i = 0; i < static_cast<int>( D6AxisEnum::eCOUNT ); ++i )
                {
                    String axisName = getAxisName( static_cast<D6AxisEnum>( i ) );
                    String motionTypeName = getMotionTypeName( m_motionTypes[i] );
                    properties->setProperty( "motion." + axisName, motionTypeName );
                }
            }

            return properties;
        }

        void ConstraintD6::setProperties( SmartPtr<Properties> properties )
        {
            if( properties )
            {
                PhysicsConstraint3<IConstraintD6>::setProperties( properties );

                // Set drive position from properties
                Transform3<real_Num> drivePos;
                Vector3<real_Num> position;
                Quaternion<real_Num> rotation;

                /*
                if( properties->hasPropertyValue( "drivePosition.position.x" ) )
                    position.X() = StringUtil::parseReal(
                        properties->GetPropertyValue( "drivePosition.position.x" ) );
                if( properties->HasPropertyValue( "drivePosition.position.y" ) )
                    position.Y() = StringUtil::parseReal(
                        properties->GetPropertyValue( "drivePosition.position.y" ) );
                if( properties->HasPropertyValue( "drivePosition.position.z" ) )
                    position.Z() = StringUtil::parseReal(
                        properties->GetPropertyValue( "drivePosition.position.z" ) );

                if( properties->HasPropertyValue( "drivePosition.rotation.x" ) )
                    rotation.X() = StringUtil::parseReal(
                        properties->GetPropertyValue( "drivePosition.rotation.x" ) );
                if( properties->HasPropertyValue( "drivePosition.rotation.y" ) )
                    rotation.Y() = StringUtil::parseReal(
                        properties->GetPropertyValue( "drivePosition.rotation.y" ) );
                if( properties->HasPropertyValue( "drivePosition.rotation.z" ) )
                    rotation.Z() = StringUtil::parseReal(
                        properties->GetPropertyValue( "drivePosition.rotation.z" ) );
                if( properties->HasPropertyValue( "drivePosition.rotation.w" ) )
                    rotation.W() = StringUtil::parseReal(
                        properties->GetPropertyValue( "drivePosition.rotation.w" ) );
                */

                drivePos.setPosition( position );
                drivePos.setOrientation( rotation );
                setDrivePosition( drivePos );

                // Set motion types from properties
                for( int i = 0; i < static_cast<int>( D6AxisEnum::eCOUNT ); ++i )
                {
                    String axisName = getAxisName( static_cast<D6AxisEnum>( i ) );
                    String propertyName = "motion." + axisName;

                    if( properties->hasProperty( propertyName ) )
                    {
                        String motionTypeName;
                        properties->getPropertyValue( propertyName, motionTypeName );

                        D6MotionEnum motionType = parseMotionType( motionTypeName );
                        setMotion( static_cast<D6AxisEnum>( i ), motionType );
                    }
                }
            }
        }

        void ConstraintD6::setDrivePosition( const Transform3<real_Num> &pose )
        {
            m_drivePosition = pose;
        }

        Transform3<real_Num> ConstraintD6::getDrivePosition() const
        {
            return m_drivePosition;
        }

        void ConstraintD6::setDrive( D6DriveEnum index, SmartPtr<IConstraintDrive> drive )
        {
            int driveIndex = static_cast<int>( index );
            if( driveIndex >= 0 && driveIndex < static_cast<int>( D6DriveEnum::eCOUNT ) )
            {
                // Unload previous drive
                if( m_drives[driveIndex] )
                {
                    m_drives[driveIndex]->unload( nullptr );
                }

                m_drives[driveIndex] = drive;

                // Load the new drive
                if( m_drives[driveIndex] )
                {
                    m_drives[driveIndex]->load( nullptr );
                }
            }
        }

        SmartPtr<IConstraintDrive> ConstraintD6::getDrive( D6DriveEnum index ) const
        {
            int driveIndex = static_cast<int>( index );
            if( driveIndex >= 0 && driveIndex < static_cast<int>( D6DriveEnum::eCOUNT ) )
            {
                return m_drives[driveIndex];
            }
            return nullptr;
        }

        void ConstraintD6::setLinearLimit( SmartPtr<IConstraintLinearLimit> limit )
        {
            // Unload previous limit
            if( m_linearLimit )
            {
                m_linearLimit->unload( nullptr );
            }

            m_linearLimit = limit;

            // Load the new limit
            if( m_linearLimit )
            {
                m_linearLimit->load( nullptr );
            }
        }

        SmartPtr<IConstraintLinearLimit> ConstraintD6::getLinearLimit() const
        {
            return m_linearLimit;
        }

        void ConstraintD6::setMotion( D6AxisEnum axis, D6MotionEnum type )
        {
            int axisIndex = static_cast<int>( axis );
            if( axisIndex >= 0 && axisIndex < static_cast<int>( D6AxisEnum::eCOUNT ) )
            {
                m_motionTypes[axisIndex] = type;
            }
        }

        D6MotionEnum ConstraintD6::getMotion( D6AxisEnum axis ) const
        {
            int axisIndex = static_cast<int>( axis );
            if( axisIndex >= 0 && axisIndex < static_cast<int>( D6AxisEnum::eCOUNT ) )
            {
                return m_motionTypes[axisIndex];
            }

            return D6MotionEnum::eLOCKED;
        }

        String ConstraintD6::getAxisName( D6AxisEnum axis ) const
        {
            switch( axis )
            {
            case D6AxisEnum::eX:
                return "x";
            case D6AxisEnum::eY:
                return "y";
            case D6AxisEnum::eZ:
                return "z";
            case D6AxisEnum::eTWIST:
                return "twist";
            case D6AxisEnum::eSWING1:
                return "swing1";
            case D6AxisEnum::eSWING2:
                return "swing2";
            default:
                return "unknown";
            }
        }

        String ConstraintD6::getMotionTypeName( D6MotionEnum motionType ) const
        {
            switch( motionType )
            {
            case D6MotionEnum::eLOCKED:
                return "locked";
            case D6MotionEnum::eLIMITED:
                return "limited";
            case D6MotionEnum::eFREE:
                return "free";
            default:
                return "locked";
            }
        }

        D6MotionEnum ConstraintD6::parseMotionType( const String &motionTypeName ) const
        {
            String lowerName = StringUtil::make_lower( motionTypeName );

            if( lowerName == "locked" )
                return D6MotionEnum::eLOCKED;
            else if( lowerName == "limited" )
                return D6MotionEnum::eLIMITED;
            else if( lowerName == "free" )
                return D6MotionEnum::eFREE;
            else
                return D6MotionEnum::eLOCKED;  // Default fallback
        }

    }  // namespace physics
}  // namespace workphone
