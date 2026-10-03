#ifndef CAeroVehicleController_h__
#define CAeroVehicleController_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/Vehicle/IDriveTrain.hpp>
#include <Workphone/Interface/Vehicle/IVehicle.hpp>
#include <Workphone/Interface/Vehicle/IVehicleBody.hpp>
#include <Workphone/Interface/Vehicle/IVehicleCallback.hpp>
#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone
{
    namespace vehicle
    {
        /**
         * @file CAeroVehicleController.hpp
         * @brief Template base implementation for aerodynamic vehicle controllers.
         *
         * This template provides a common implementation for vehicle controllers
         * that manage aerodynamic forces, transforms and communication with a
         * physics/vehicle body via an @c IVehicleCallback and @c IVehicleBody.
         *
         * The template parameter T is expected to be a concrete controller interface
         * or an existing controller base class that defines the virtual contract.
         */

        /**
         * @class CAeroVehicleController
         * @brief Aerodynamics-capable vehicle controller implementation.
         *
         * CAeroVehicleController implements common behaviour required by vehicles
         * subject to aerodynamic forces. It collects forces and torques each update
         * and forwards them to the configured @c IVehicleCallback. It also keeps
         * and updates local/world transforms, centre of gravity and debug display
         * state.
         *
         * @tparam T The base controller type this template derives from.
         */
        template <class T>
        class CAerodynamicsVehicle : public T
        {
        public:
            /** @brief Default constructor. */
            CAerodynamicsVehicle();

            /** @brief Virtual destructor. */
            ~CAerodynamicsVehicle() override;

            /**
             * @brief Load controller state from a shared object.
             * @param data Generic shared data used to initialize the controller.
             *
             * Implementations should read configuration (transforms, mass, CG, etc.)
             * from @p data when available.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Load controller state from a string (e.g. serialized data or name).
             * @param data String data used to initialize the controller.
             */
            void load( const std::string &data );

            /**
             * @brief Reload or partially reinitialize the controller.
             * @param pData Optional raw pointer to data used for reinitialization.
             */
            void reload( void *pData );

            /** @brief Reset the controller to its default state. */
            void reset() override;

            /**
             * @brief Per-frame update called by the owning system.
             *
             * This function applies accumulated forces/torques to the configured
             * @c IVehicleCallback and then clears the local accumulators.
             */
            void update() override;

            /** @brief Update the internal transforms and sync centre of gravity with the body. */
            void updateTransform();

            /**
             * @brief Get a control channel value.
             * @param idx Channel index (0..N).
             * @return Channel value as a float.
             */
            f32 getChannel( s32 idx ) const override;

            /**
             * @brief Set a control channel value.
             * @param idx Channel index.
             * @param channel Value to set.
             */
            void setChannel( s32 idx, f32 channel ) override;

            /**
             * @brief Get the controller/world scale.
             * @return Scale vector (obtained from the callback if available).
             */
            Vector3<real_Num> getScale() const;

            /**
             * @brief Get world position (delegates to @c IVehicleCallback when set).
             * @return World position vector.
             */
            Vector3<real_Num> getPosition() const override;

            /**
             * @brief Set world position. Implementations may forward to the callback.
             * @param position New world position.
             */
            void setPosition( const Vector3<real_Num> &position ) override;

            /**
             * @brief Get world orientation (delegates to @c IVehicleCallback when set).
             * @return Orientation quaternion.
             */
            Quaternion<real_Num> getOrientation() const;

            /**
             * @brief Set world orientation. Implementations may forward to the callback.
             * @param orientation New orientation quaternion.
             */
            void setOrientation( const Quaternion<real_Num> &orientation );

            /**
             * @brief Check whether the vehicle is under user control.
             * @return True if user controlled, false otherwise.
             */
            bool isUserControlled() const override;

            /**
             * @brief Set whether the vehicle is user controlled.
             * @param userControlled True to mark as user controlled.
             */
            void setUserControlled( bool userControlled ) override;

            /**
             * @brief Get the vehicle mass (reads from @c IVehicleBody when available).
             * @return Mass in simulation units.
             */
            real_Num getMass() const override;

            /**
             * @brief Set the vehicle mass (applies to @c IVehicleBody if present).
             * @param mass New mass value.
             */
            void setMass( real_Num mass ) override;

            /**
             * @brief Get the physics body used by the controller.
             * @return Smart pointer to an @c IVehicleBody instance, or null.
             */
            SmartPtr<IVehicleBody> getBody() const override;

            /**
             * @brief Set the physics body the controller should operate on.
             * @param body Smart pointer to an @c IVehicleBody.
             */
            void setBody( SmartPtr<IVehicleBody> body ) override;

            /**
             * @brief Get the computed world transform used for drawing and physics.
             * @return World transform.
             */
            Transform3<real_Num> getWorldTransform() const override;

            /**
             * @brief Set the world transform.
             * @param worldTransform New world transform.
             */
            void setWorldTransform( const Transform3<real_Num> &worldTransform ) override;

            /**
             * @brief Get the local transform relative to the body.
             * @return Local transform.
             */
            Transform3<real_Num> getLocalTransform() const override;

            /**
             * @brief Set the local transform relative to the body.
             * @param localTransform New local transform.
             */
            void setLocalTransform( const Transform3<real_Num> &localTransform ) override;

            /**
             * @brief Draw a small cross at a world point (debug).
             * @param body Body index (for the callback).
             * @param id Base id for debug lines.
             * @param positon Point in local space (transformed to world).
             * @param color Colour to use for debug lines.
             */
            void drawPoint( s32 body, int id, const Vector3<real_Num> &positon, u32 color ) override;

            /**
             * @brief Draw a small cross at a local point (debug).
             * @param body Body index.
             * @param id Base id for debug lines.
             * @param positon Local-space point.
             * @param color Colour to use for debug lines.
             */
            void drawLocalPoint( s32 body, int id, const Vector3<real_Num> &positon,
                                 u32 color ) override;

            /**
             * @brief Request that a local-space vector be displayed (delegates to callback).
             * @param bodyId Body identifier for callback routing.
             * @param start Vector start in local space.
             * @param end Vector end in local space.
             * @param colour Colour for the displayed vector.
             */
            void displayLocalVector( s32 bodyId, const Vector3<real_Num> &start,
                                     const Vector3<real_Num> &end, u32 colour ) override;

            /**
             * @brief Request that a world-space vector be displayed (delegates to callback).
             * @param bodyId Body identifier.
             * @param id Unique id for this vector (used by debug drawing system).
             * @param start World-space start.
             * @param end World-space end.
             * @param colour Colour to use.
             */
            void displayVector( s32 bodyId, s32 id, const Vector3<real_Num> &start,
                                const Vector3<real_Num> &end, u32 colour ) override;

            /**
             * @brief Request that a local-space vector be displayed using an explicit id.
             * @param bodyId Body identifier.
             * @param id Unique id for this vector.
             * @param start Local-space start.
             * @param end Local-space end.
             * @param colour Colour to use.
             */
            void displayLocalVector( s32 bodyId, s32 id, const Vector3<real_Num> &start,
                                     const Vector3<real_Num> &end, u32 colour ) override;

            /**
             * @brief Check whether debug display data is enabled for this controller.
             * @return True when debug display is enabled.
             */
            bool getDisplayDebugData() const override;

            /**
             * @brief Enable or disable debug display for this controller.
             * @param debugDisplay True to enable debug display.
             */
            void setDisplayDebugData( bool displayDebugData ) override;

            /**
             * @brief Add a world-space force applied at a world-space location.
             * @param bodyIdx Body index (for callback routing).
             * @param Force Force vector in world space.
             * @param Loc World-space location where force is applied.
             *
             * This computes the torque produced by the force about the configured
             * centre of gravity and accumulates both force and torque for later
             * dispatch to the callback.
             */
            void addForce( s32 bodyIdx, const Vector3<real_Num> &Force,
                           const Vector3<real_Num> &Loc ) override;

            /**
             * @brief Add a world-space torque.
             * @param bodyIdx Body index (for callback routing).
             * @param Torque Torque vector in world space.
             */
            void addTorque( s32 bodyIdx, const Vector3<real_Num> &Torque ) override;

            /**
             * @brief Add a force expressed in local (body) space.
             * @param bodyIdx Body index.
             * @param Force Force vector in local space.
             * @param Loc Local-space application point.
             *
             * The function transforms the provided local-space force and point to
             * world space before calculating torque and accumulating the result.
             */
            void addLocalForce( s32 bodyIdx, const Vector3<real_Num> &Force,
                                const Vector3<real_Num> &loc ) override;

            /**
             * @brief Add a torque expressed in local (body) space.
             * @param bodyIdx Body index.
             * @param Torque Torque vector in local space.
             */
            void addLocalTorque( s32 bodyIdx, const Vector3<real_Num> &Torque ) override;

            /**
             * @brief Get the velocity of a world-space point on the body (delegates to callback).
             * @param p World-space point.
             * @return Point velocity in world space.
             */
            Vector3<real_Num> getPointVelocity( const Vector3<real_Num> &p ) override;

            /**
             * @brief Get the angular velocity of the body (world space).
             * @return Angular velocity vector.
             */
            Vector3<real_Num> getAngularVelocity() override;

            /**
             * @brief Get the linear velocity of the body (world space).
             * @return Linear velocity vector.
             */
            Vector3<real_Num> getLinearVelocity() override;

            /**
             * @brief Get the angular velocity expressed in local (body) space.
             * @return Local-space angular velocity.
             */
            Vector3<real_Num> getLocalAngularVelocity() override;

            /**
             * @brief Get the local linear velocity (delegates to callback).
             * @return Local linear velocity vector.
             */
            Vector3<real_Num> getLocalLinearVelocity() override;

            /**
             * @brief Retrieve the vehicle callback used for event/force dispatch.
             * @return Pointer to the vehicle callback.
             */
            IVehicleCallback *getVehicleCallbackPtr() const override;

            /**
             * @overload
             * @brief Const overload to retrieve the vehicle callback.
             */
            SmartPtr<IVehicleCallback> getVehicleCallback() const override;

            /**
             * @brief Set the vehicle callback used by the controller.
             * @param callback Smart pointer to an @c IVehicleCallback implementation.
             */
            void setVehicleCallback( SmartPtr<IVehicleCallback> callback ) override;

            /**
             * @brief Get the configured centre of gravity in local body space.
             * @return Centre of gravity vector.
             */
            Vector3<real_Num> getCG() const override;

            /** @copydoc IVehicleController::setState */
            virtual void setState( IVehicle::State state ) override;

            /** @copydoc IVehicleController::getState */
            virtual IVehicle::State getState() const override;

            /**
             * @brief Return a wheel controller component for the given index.
             * @param index Wheel index.
             * @return Smart pointer to an @c IWheelComponent or nullptr if unavailable.
             */
            SmartPtr<IWheelComponent> getWheelController( u32 index ) const;

            /**
             * @brief Get the drivetrain implementation used by this vehicle.
             * @return Smart pointer to IDriveTrain or nullptr if none set.
             */
            SmartPtr<IDriveTrain> getDriveTrain() const;

            /**
             * @brief Assign a drivetrain implementation to the vehicle.
             *
             * The drivetrain is responsible for distributing engine/torque to wheels.
             *
             * @param driveTrain SmartPtr to an IDriveTrain implementation.
             */
            void setDriveTrain( SmartPtr<IDriveTrain> driveTrain );

            /**
             * @brief Get configured drive type (FWD/RWD/AWD).
             * @return VehicleDriveType enum value describing the drive layout.
             */
            VehicleDriveType getDriveType() const;

            /**
             * @brief Set the vehicle drive type (FWD/RWD/AWD).
             * @param driveType Desired VehicleDriveType.
             */
            void setDriveType( VehicleDriveType driveType );

            WP_CLASS_REGISTER_TEMPLATE_DECL( CAerodynamicsVehicle, T );

        protected:
            /**
             * @brief Internal accumulator for world-space forces.
             * @param force Force vector to accumulate.
             *
             * This protected overload accumulates a force into the controller's
             * internal force accumulator. It is used by the public add* methods.
             */
            void addForce( const Vector3<real_Num> &force );

            /**
             * @brief Internal accumulator for world-space torques.
             * @param torque Torque vector to accumulate.
             */
            void addTorque( const Vector3<real_Num> &torque );

            /** @brief Clear accumulated forces and torques. */
            void clearForces();

            /** @brief Current vehicle state (atomic). */
            AtomicValue<IVehicle::State> m_vehicleState = IVehicle::State::AWAKE;

            /** @brief Transform representing the body (position/orientation/scale). */
            Transform3<real_Num> m_bodyTransform;
            /** @brief Computed world transform for drawing/physics. */
            Transform3<real_Num> m_worldTransform;
            /** @brief Local transform relative to parent or body. */
            Transform3<real_Num> m_localTransform;

            /** @brief Physics body used by the controller (atomic smart pointer). */
            AtomicSmartPtr<IVehicleBody> m_rigidbody;
            /** @brief Callback used to forward forces, queries and debug draws. */
            SmartPtr<IVehicleCallback> m_callback;

            /** @brief Accumulated world-space force for the current frame. */
            Vector3<real_Num> m_force = Vector3<real_Num>::zero();
            /** @brief Accumulated world-space torque for the current frame. */
            Vector3<real_Num> m_torque = Vector3<real_Num>::zero();

            /** @brief Aerodynamic drag vector (for future use). */
            Vector3<real_Num> m_drag = Vector3<real_Num>::zero();
            /** @brief Centre of gravity expressed in local body space. */
            Vector3<real_Num> m_cg = Vector3<real_Num>::zero();

            /** @brief Whether debug drawing is enabled for this controller. */
            atomic_bool m_displayDebugData = false;

            /** @brief Array of controller input channels (size fixed to 8). */
            FixedArray<f32, 8> m_channels;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, CAerodynamicsVehicle, T, T );

        template <class T>
        CAerodynamicsVehicle<T>::CAerodynamicsVehicle()
        {
        }

        template <class T>
        CAerodynamicsVehicle<T>::~CAerodynamicsVehicle()
        {
        }

        template <class T>
        void CAerodynamicsVehicle<T>::load( SmartPtr<ISharedObject> data )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto factoryManager = applicationManager->getFactoryManager();

            // auto worldTransform = factoryManager->make_ptr<Transform3<real_Num>>();
            // setWorldTransform(worldTransform);

            // auto localTransform = factoryManager->make_ptr<Transform3<real_Num>>();
            // setLocalTransform(localTransform);

            // auto bodyTransform = factoryManager->make_ptr<Transform3<real_Num>>();
            // m_bodyTransform = bodyTransform;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::load( const std::string &data )
        {
            auto worldTransform = Transform3<real_Num>();
            setWorldTransform( worldTransform );

            auto localTransform = Transform3<real_Num>();
            setLocalTransform( localTransform );

            auto bodyTransform = Transform3<real_Num>();
            // m_bodyTransform = bodyTransform;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::reload( void *pData )
        {
        }

        template <class T>
        void CAerodynamicsVehicle<T>::reset()
        {
        }

        template <class T>
        void CAerodynamicsVehicle<T>::update()
        {
            updateTransform();

            if( m_callback )
            {
                m_callback->addForce( 0, m_force, Vector3<real_Num>::zero() );
                m_callback->addTorque( 0, m_torque );

                m_force = Vector3<real_Num>::zero();
                m_torque = Vector3<real_Num>::zero();
            }
        }

        template <class T>
        f32 CAerodynamicsVehicle<T>::getChannel( s32 idx ) const
        {
            return m_channels[idx];
        }

        template <class T>
        void CAerodynamicsVehicle<T>::setChannel( s32 idx, f32 channel )
        {
            m_channels[idx] = channel;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::updateTransform()
        {
            auto p = getPosition();
            auto q = getOrientation();

            WP_ASSERT( MathUtil<real_Num>::isFinite( p ) );
            WP_ASSERT( MathUtil<real_Num>::isFinite( q ) );

            // p = Vector3<real_Num>::zero();
            // q = Quaternion<real_Num>::identity();

            // if (m_bodyTransform)
            {
                m_bodyTransform.setPosition( p );
                m_bodyTransform.setOrientation( q );
                m_bodyTransform.setScale( Vector3<real_Num>::unit() );

                // if (m_worldTransform)
                {
                    // if (m_localTransform)
                    {
                        m_worldTransform.transformFromParent( m_bodyTransform, m_localTransform );
                    }
                }
            }

            if( auto body = getBody() )
            {
                auto cg = m_bodyTransform.transformPoint( m_cg );
                body->setWorldCenterOfMass( cg );
            }
        }

        template <class T>
        Vector3<real_Num> CAerodynamicsVehicle<T>::getScale() const
        {
            if( m_callback )
            {
                return m_callback->getScale();
            }

            return Vector3<real_Num>::zero();
        }

        template <class T>
        Vector3<real_Num> CAerodynamicsVehicle<T>::getPosition() const
        {
            if( m_callback )
            {
                return m_callback->getPosition();
            }

            return Vector3<real_Num>::zero();
        }

        template <class T>
        void CAerodynamicsVehicle<T>::setPosition( const Vector3<real_Num> &position )
        {
        }

        template <class T>
        Quaternion<real_Num> CAerodynamicsVehicle<T>::getOrientation() const
        {
            if( m_callback )
            {
                return m_callback->getOrientation();
            }

            return Quaternion<real_Num>::identity();
        }

        template <class T>
        void CAerodynamicsVehicle<T>::setOrientation( const Quaternion<real_Num> &orientation )
        {
        }

        template <class T>
        bool CAerodynamicsVehicle<T>::isUserControlled() const
        {
            return false;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::setUserControlled( bool userControlled )
        {
        }

        template <class T>
        real_Num CAerodynamicsVehicle<T>::getMass() const
        {
            if( auto body = getBody() )
            {
                return body->getMass();
            }

            return static_cast<real_Num>( 1.0 );
        }

        template <class T>
        void CAerodynamicsVehicle<T>::setMass( real_Num mass )
        {
            if( auto body = getBody() )
            {
                body->setMass( mass );
            }
        }

        template <class T>
        SmartPtr<IVehicleBody> CAerodynamicsVehicle<T>::getBody() const
        {
            return m_rigidbody;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::setBody( SmartPtr<IVehicleBody> body )
        {
            m_rigidbody = body;
        }

        template <class T>
        Transform3<real_Num> CAerodynamicsVehicle<T>::getWorldTransform() const
        {
            return m_worldTransform;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::setWorldTransform( const Transform3<real_Num> &worldTransform )
        {
            m_worldTransform = worldTransform;
        }

        template <class T>
        Transform3<real_Num> CAerodynamicsVehicle<T>::getLocalTransform() const
        {
            return m_localTransform;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::setLocalTransform( const Transform3<real_Num> &localTransform )
        {
            m_localTransform = localTransform;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::drawPoint( s32 body, int id, const Vector3<real_Num> &positon,
                                                 u32 color )
        {
            auto size = static_cast<real_Num>( 0.1 );
            auto offset0 = Vector3<real_Num>::forward() * size;
            auto offset1 = Vector3<real_Num>::up() * size;
            auto offset2 = Vector3<real_Num>::right() * size;

            auto p = m_worldTransform.getOrientation() * positon;
            p += m_worldTransform.getPosition();

            displayVector( body, id, p + offset0, p - offset0, color );
            displayVector( body, id + 1, p + offset1, p - offset1, color );
            displayVector( body, id + 2, p + offset2, p - offset2, color );
        }

        template <class T>
        void CAerodynamicsVehicle<T>::drawLocalPoint( s32 body, int id, const Vector3<real_Num> &positon,
                                                      u32 color )
        {
            auto size = 0.1f;
            auto offset0 = Vector3<real_Num>::forward() * size;
            auto offset1 = Vector3<real_Num>::up() * size;
            auto offset2 = Vector3<real_Num>::right() * size;

            // if (m_worldTransform)
            {
                auto p = m_worldTransform.getOrientation() * positon;
                p += m_worldTransform.getPosition();

                displayVector( body, id, p + offset0, p - offset0, color );
                displayVector( body, id + 1000, p + offset1, p - offset1, color );
                displayVector( body, id + 2000, p + offset2, p - offset2, color );
            }
        }

        template <class T>
        void CAerodynamicsVehicle<T>::displayLocalVector( s32 bodyId, const Vector3<real_Num> &start,
                                                          const Vector3<real_Num> &end, u32 colour )
        {
            try
            {
                if( m_callback )
                {
                    m_callback->displayLocalVector(
                        bodyId, Vector3<real_Num>( start.X(), start.Y(), start.Z() ),
                        Vector3<real_Num>( end.X(), end.Y(), end.Z() ), colour );
                }
            }
            catch( std::exception &Err )
            {
                WP_LOG_EXCEPTION( Err );
            }
        }

        template <class T>
        void CAerodynamicsVehicle<T>::displayVector( s32 bodyId, s32 id, const Vector3<real_Num> &start,
                                                     const Vector3<real_Num> &end, u32 colour )
        {
            try
            {
                if( m_callback )
                {
                    m_callback->displayVector( bodyId, id, start, end, colour );
                }
            }
            catch( std::exception &Err )
            {
                WP_LOG_EXCEPTION( Err );
            }
        }

        template <class T>
        void CAerodynamicsVehicle<T>::displayLocalVector( s32 bodyId, s32 id,
                                                          const Vector3<real_Num> &start,
                                                          const Vector3<real_Num> &end, u32 colour )
        {
            try
            {
                if( m_callback )
                {
                    auto worldTransform = getWorldTransform();

                    WP_ASSERT( worldTransform.isValid() );

                    WP_ASSERT( start.isValid() );
                    WP_ASSERT( end.isValid() );

                    auto worldStart = worldTransform.transformPoint( start );
                    auto worldEnd = worldTransform.transformPoint( end );

                    WP_ASSERT( worldStart.isValid() );
                    WP_ASSERT( worldEnd.isValid() );

                    m_callback->displayVector( bodyId, id, start, end, colour );
                }
            }
            catch( std::exception &Err )
            {
                WP_LOG_EXCEPTION( Err );
            }
        }

        template <class T>
        bool CAerodynamicsVehicle<T>::getDisplayDebugData() const
        {
            return m_displayDebugData;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::setDisplayDebugData( bool displayDebugData )
        {
            m_displayDebugData = displayDebugData;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::addForce( s32 bodyIdx, const Vector3<real_Num> &Force,
                                                const Vector3<real_Num> &Loc )
        {
            WP_ASSERT( Force.length() < 1e5 );
            WP_ASSERT( Loc.length() < 1e5 );

            // if (m_callback)
            //{
            //	m_callback->addForce(Bdy, Force, Loc);
            // }

            const Vector3<real_Num> centerOfMass = m_worldTransform.transformPoint( m_cg );
            const Vector3<real_Num> torque = ( Loc - centerOfMass ).crossProduct( Force );

            addForce( Force );
            addTorque( torque );
        }

        template <class T>
        void CAerodynamicsVehicle<T>::addTorque( s32 bodyIdx, const Vector3<real_Num> &Torque )
        {
            WP_ASSERT( Torque.length() < 1e5 );

            // if (m_callback)
            //{
            //	m_callback->addTorque(Bdy, Torque);
            // }

            addTorque( Torque );
        }

        template <class T>
        void CAerodynamicsVehicle<T>::addLocalForce( s32 bodyIdx, const Vector3<real_Num> &Force,
                                                     const Vector3<real_Num> &Loc )
        {
            WP_ASSERT( Force.length() < 1e10 );
            WP_ASSERT( Loc.length() < 1e10 );

            // if (m_callback)
            //{
            //	m_callback->addLocalForce(Bdy, Force, Loc);
            // }

            const auto worldPoint = m_worldTransform.transformPoint( Loc );
            const auto force = m_worldTransform.transformVector( Force );

            if( auto body = getBody() )
            {
                const auto centerOfMass = body->getWorldCenterOfMass();
                const auto torque = ( worldPoint - centerOfMass ).crossProduct( force );

                addForce( force );
                addTorque( torque );
            }
        }

        template <class T>
        void CAerodynamicsVehicle<T>::addLocalTorque( s32 bodyIdx, const Vector3<real_Num> &Torque )
        {
            WP_ASSERT( Torque.length() < 1e5 );

            // if (m_callback)
            //{
            //	m_callback->addLocalTorque(Bdy, Torque);
            // }

            auto torque = m_worldTransform.getOrientation() * Torque;
            addTorque( torque );
        }

        template <class T>
        Vector3<real_Num> CAerodynamicsVehicle<T>::getPointVelocity( const Vector3<real_Num> &p )
        {
            if( m_callback )
            {
                return m_callback->getPointVelocity( p );
            }

            return Vector3<real_Num>::zero();
        }

        template <class T>
        Vector3<real_Num> CAerodynamicsVehicle<T>::getAngularVelocity()
        {
            if( m_callback )
            {
                return m_callback->getAngularVelocity();
            }

            return Vector3<real_Num>::zero();
        }

        template <class T>
        Vector3<real_Num> CAerodynamicsVehicle<T>::getLinearVelocity()
        {
            if( m_callback )
            {
                return m_callback->getLinearVelocity();
            }

            return Vector3<real_Num>::zero();
        }

        template <class T>
        Vector3<real_Num> CAerodynamicsVehicle<T>::getLocalAngularVelocity()
        {
            if( m_callback )
            {
                auto worldAngularVelocity = m_callback->getAngularVelocity();
                return m_worldTransform.inverseTransformVector( worldAngularVelocity );
            }

            return Vector3<real_Num>::zero();
        }

        template <class T>
        Vector3<real_Num> CAerodynamicsVehicle<T>::getLocalLinearVelocity()
        {
            if( m_callback )
            {
                return m_callback->getLocalLinearVelocity();
            }

            return Vector3<real_Num>::zero();
        }

        template <class T>
        IVehicleCallback *CAerodynamicsVehicle<T>::getVehicleCallbackPtr() const
        {
            return m_callback.get();
        }

        template <class T>
        SmartPtr<IVehicleCallback> CAerodynamicsVehicle<T>::getVehicleCallback() const
        {
            return m_callback;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::setVehicleCallback( SmartPtr<IVehicleCallback> callback )
        {
            m_callback = callback;
        }

        template <class T>
        Vector3<real_Num> CAerodynamicsVehicle<T>::getCG() const
        {
            return m_cg;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::setState( IVehicle::State state )
        {
            m_vehicleState = state;
        }

        template <class T>
        IVehicle::State CAerodynamicsVehicle<T>::getState() const
        {
            return m_vehicleState;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::addForce( const Vector3<real_Num> &force )
        {
            m_force += force;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::addTorque( const Vector3<real_Num> &torque )
        {
            m_torque += torque;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::clearForces()
        {
            m_force = Vector3<real_Num>::zero();
            m_torque = Vector3<real_Num>::zero();
        }

        template <class T>
        SmartPtr<IWheelComponent> CAerodynamicsVehicle<T>::getWheelController( u32 index ) const
        {
            return nullptr;
        }

        template <class T>
        SmartPtr<IDriveTrain> CAerodynamicsVehicle<T>::getDriveTrain() const
        {
            return nullptr;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::setDriveTrain( SmartPtr<IDriveTrain> driveTrain )
        {
        }

        template <class T>
        VehicleDriveType CAerodynamicsVehicle<T>::getDriveType() const
        {
            return VehicleDriveType::FrontWheelDrive;
        }

        template <class T>
        void CAerodynamicsVehicle<T>::setDriveType( VehicleDriveType driveType )
        {
        }
    } // namespace vehicle
} // namespace workphone

#endif // CVehicleController_h__
