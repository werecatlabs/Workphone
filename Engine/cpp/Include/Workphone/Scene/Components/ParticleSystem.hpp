#ifndef __ParticleSystemComponent_h__
#define __ParticleSystemComponent_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @brief Component that manages a particle system.
         *
         * This component wraps renderer objects (graphics object, scene node and
         * particle system) and exposes particle-related properties (lifetime,
         * emission rate, size, shape, etc.). It handles creation and destruction
         * of the underlying graphics particle system and provides property
         * accessors used by the scene system.
         *
         * The class stores both runtime configuration values and handles to
         * renderer objects. Properties are represented by static string keys
         * defined below and are used when serializing/deserializing component
         * state via Properties objects.
         */
        class WPCore_API ParticleSystem : public Component
        {
        public:
            /** @name Property key constants
             *  String keys used for property lookup and serialization.
             */
            //@{
            static const String lifetimeStr;            /**< Property key: particle lifetime */
            static const String durationStr;            /**< Property key: emitter duration */
            static const String loopingStr;             /**< Property key: looping enabled */
            static const String playStr;                /**< Property key: play action */
            static const String stopStr;                /**< Property key: stop action */
            static const String templateNameStr;        /**< Property key: particle system template */
            static const String techniqueNameStr;       /**< Property key: default technique name */
            static const String emitterNameStr;         /**< Property key: default emitter name */
            static const String playOnLoadStr;          /**< Property key: start particles on load */
            static const String fastForwardTimeStr;     /**< Property key: fast-forward time */
            static const String fastForwardIntervalStr; /**< Property key: fast-forward interval */
            static const String startLifetimeStr;       /**< Property key: start lifetime range */
            static const String startSizeStr;           /**< Property key: start size range */
            static const String scaleStr;               /**< Property key: particle scale */
            static const String emissionStr;            /**< Property key: emission group */
            static const String rateStr;                /**< Property key: emission rate */
            static const String rateVarianceStr;        /**< Property key: emission rate variance */
            static const String angleStr;               /**< Property key: emission angle */
            static const String angleVarianceStr;       /**< Property key: emission angle variance */
            static const String shapeStr;               /**< Property key: emitter shape group */
            static const String typeStr;                /**< Property key: particle type */
            static const String sizeStr;                /**< Property key: particle size */
            static const String sizeVarianceStr;        /**< Property key: particle size variance */
            static const String shapeTypeStr;           /**< Property key: emitter shape type */
            static const String shapeSizeStr;           /**< Property key: emitter shape size */
            static const String shapeSizeVarianceStr;   /**< Property key: emitter shape size variance */
            //@}

            /**
             * @brief Constructs a new ParticleSystem component.
             *
             * Initializes default property values; graphics objects are not
             * created until load() or createGraphicsParticleSystem() is invoked.
             */
            ParticleSystem();

            /**
             * @brief Virtual destructor.
             *
             * Ensures derived cleanup occurs and releases any renderer handles
             * if still present.
             */
            ~ParticleSystem() override;

            /**
             * @copydoc Component::load
             *
             * Loads component state from @p data (typically a Properties
             * object). This will create the graphics particle system if
             * necessary and apply property values.
             *
             * @param data Data used to initialize the component (can be null).
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::unload
             *
             * Unloads the component and releases renderer resources.
             *
             * @param data Optional unload context.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::getChildObjects
             *
             * Returns any child shared objects owned by the component (for
             * example nested components or editor-only resources).
             *
             * @return Array of child ISharedObject pointers (may be empty).
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @copydoc Component::getProperties
             *
             * Serializes the component state into a Properties object using the
             * defined property keys.
             *
             * @return Properties object describing this component.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc Component::setProperties
             *
             * Applies values from a Properties object to this component.
             *
             * @param properties Properties to apply (must not be null).
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Synchronizes the renderer node with the owning actor.
             */
            void updateTransform() override;

            /**
             * @brief Applies a precomputed world transform to the renderer node.
 *

             * * @param transform World transform to apply.
             */
            void updateTransform( const Transform3<real_Num> &transform ) override;

            /**
             * @brief Synchronizes renderer visibility with the component and actor.
             */
            void updateVisibility() override;

            /**
             * @brief Returns the renderer graphics object associated with this particle system.
             *
             * The graphics object is the renderer-level container for GPU/visual
             * resources required by the particle system. Can be null if not created.
             *
             * @return SmartPtr to the renderer's IGraphicsObject (or null).
             */
            SmartPtr<render::IGraphicsObject> getGraphicsObject() const;

            /**
             * @brief Sets the renderer graphics object for this component.
             *
             * Ownership is held by SmartPtr; passing null will clear the current
             * reference without destroying external resources.
             *
             * @param graphicsObject SmartPtr to the graphics object.
             */
            void setGraphicsObject( SmartPtr<render::IGraphicsObject> graphicsObject );

            /**
             * @brief Returns the scene node that hosts the particle system.
             *
             * A scene node represents the transform and hierarchical placement
             * for the particle system within the scene graph. May be null.
             *
             * @return SmartPtr to the renderer's IGraphicsSceneNode (or null).
             */
            SmartPtr<render::IGraphicsSceneNode> getGraphicsNode() const;

            /**
             * @brief Sets the scene node used by the particle system.
             *
             * @param graphicsNode SmartPtr to the scene node.
             */
            void setGraphicsNode( SmartPtr<render::IGraphicsSceneNode> graphicsNode );

            /**
             * @brief Returns the underlying renderer particle system object.
             *
             * This is the renderer-specific particle-system interface instance.
             * May be null if the graphics particle system has not been created.
             *
             * @return SmartPtr to the renderer's IParticleSystem (or null).
             */
            SmartPtr<render::IParticleSystem> getParticleSystem() const;

            /**
             * @brief Sets the renderer particle system object.
             *
             * Replaces the internal handle to the renderer particle system.
             *
             * @param particleSystem SmartPtr to the renderer particle system.
             */
            void setParticleSystem( SmartPtr<render::IParticleSystem> particleSystem );

            const String &getTemplateName() const;
            void setTemplateName( const String &templateName );

            const String &getTechniqueName() const;
            void setTechniqueName( const String &techniqueName );

            const String &getEmitterName() const;
            void setEmitterName( const String &emitterName );

            bool getPlayOnLoad() const;
            void setPlayOnLoad( bool playOnLoad );

            /** @name Lifetime and duration */
            //@{
            f32 getLifetime() const;
            void setLifetime( f32 lifetime );

            f32 getDuration() const;
            void setDuration( f32 duration );

            bool isLooping() const;
            void setLooping( bool looping );
            //@}

            /** @name Fast-forward */
            //@{
            f32 getFastForwardTime() const;
            void setFastForwardTime( f32 time );

            f32 getFastForwardInterval() const;
            void setFastForwardInterval( f32 interval );
            //@}

            /** @name Start ranges */
            //@{
            Vector2<real_Num> getStartLifetime() const;
            void setStartLifetime( const Vector2<real_Num> &startLifetime );

            Vector2<real_Num> getStartSize() const;
            void setStartSize( const Vector2<real_Num> &startSize );

            Vector3<real_Num> getScale() const;
            void setScale( const Vector3<real_Num> &scale );
            //@}

            /** @name Emission */
            //@{
            f32 getRate() const;
            void setRate( f32 rate );

            f32 getRateVariance() const;
            void setRateVariance( f32 rateVariance );

            f32 getAngle() const;
            void setAngle( f32 angle );

            f32 getAngleVariance() const;
            void setAngleVariance( f32 angleVariance );
            //@}

            /** @name Emitter shape */
            //@{
            f32 getShapeType() const;
            void setShapeType( f32 shapeType );

            f32 getShapeSize() const;
            void setShapeSize( f32 shapeSize );

            f32 getShapeSizeVariance() const;
            void setShapeSizeVariance( f32 shapeSizeVariance );
            //@}

            void play();
            void stop();
            void pause();
            void resume();
            void rebuild();
            bool isPlaying() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Creates the renderer-specific particle system and related objects.
             *
             * This method is responsible for allocating/initializing renderer
             * objects (graphics object, scene node and particle system) using
             * current property values. It is safe to call multiple times; any
             * existing graphics particle system will be destroyed first.
             */
            void createGraphicsParticleSystem();

            /**
             * @brief Destroys the renderer-specific particle system and related objects.
             *
             * Releases references to renderer objects and performs any needed
             * cleanup so that the component no longer holds renderer resources.
             */
            void destroyGraphicsParticleSystem();

            void applyParticleSystemProperties();

            void applyRendererProperties();

            /** Renderer particle system handle (may be null). */
            SmartPtr<render::IParticleSystem> m_particleSystem;

            /** Renderer graphics object (container for GPU resources). */
            SmartPtr<render::IGraphicsObject> m_graphicsObject;

            /** Scene node that positions the particle system in the scene graph. */
            SmartPtr<render::IGraphicsSceneNode> m_graphicsNode;

            /** Range for start lifetime (min, max). */
            Vector2<real_Num> m_startLifetime = Vector2<real_Num>( 1.0f, 5.0f );

            /** Range for start size (min, max). */
            Vector2<real_Num> m_startSize = Vector2<real_Num>( 1.0f, 1.0f );

            /** Local scale applied to emitted particles. */
            Vector3<real_Num> m_scale = Vector3<real_Num>( 1.0f, 1.0f, 1.0f );

            /** Optional renderer particle template name. */
            String m_templateName;

            /** Stable technique name used by this component. */
            String m_techniqueName = "default";

            /** Stable emitter name used by this component. */
            String m_emitterName = "default";

            /** Default particle lifetime in seconds. */
            f32 m_lifetime = 5.0f;

            /** Duration of the emitter (seconds). */
            f32 m_duration = 5.0f;

            /** Time to fast-forward the simulation on start (seconds). */
            f32 m_fastForwardTime = 0.0f;

            /** Interval used when fast-forwarding the simulation. */
            f32 m_fastForwardInterval = 0.0f;

            /** Emission rate (particles per second). */
            f32 m_rate = 5.0f;

            /** Variance applied to the emission rate. */
            f32 m_rateVariance = 0.0f;

            /** Base emission angle (degrees or radians depending on renderer). */
            f32 m_angle = 0.0f;

            /** Variance applied to the emission angle. */
            f32 m_angleVariance = 0.0f;

            /** Numeric identifier for shape type (renderer-specific). */
            f32 m_shapeType = 0;

            /** Size parameter for the emitter shape. */
            f32 m_shapeSize = 0.0f;

            /** Variance applied to the emitter shape size. */
            f32 m_shapeSizeVariance = 0.0f;

            /** Whether the particle system loops after finishing. */
            bool m_looping = true;

            /** Whether the particle system starts emitting after load. */
            bool m_playOnLoad = true;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // ParticleSystemComponent_h__
