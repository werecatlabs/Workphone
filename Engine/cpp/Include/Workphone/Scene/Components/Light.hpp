#ifndef LightComponent_h__
#define LightComponent_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/Graphics/IGraphicsLight.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @file Light.hpp
         * @brief Component that encapsulates a scene light (diffuse/specular, type, range, attenuation,
         * intensity).
         *
         * This header declares the `Light` scene component which owns a renderer-specific
         * light (`render::IGraphicsLight`) and an optional scene node (`render::IGraphicsSceneNode`)
         * used to position/orient the light in the scene graph. The component exposes common light
         * properties (diffuse/specular colours, light type, range, attenuation and intensity) and
         * synchronises them with the renderer object when available.
         */

        /**
         * @class Light
         * @brief Scene component that owns and manages a renderable light.
         *
         * The `Light` component is responsible for:
         *  - Storing serializable light properties (diffuse/specular colours, type, intensity,
         *    range and attenuation).
         *  - Creating, owning and releasing the renderer light object (`render::IGraphicsLight`)
         *    and an associated `render::IGraphicsSceneNode` used for transform updates.
         *  - Responding to component lifecycle events (load/unload), handling flag/state
         *    changes and performing debug drawing if enabled.
         *
         * By default the light is directional and both diffuse/specular colours default
         * to white. Changing the `Light` type may recreate or reconfigure the underlying
         * renderer object.
         *
         * @note The class only stores parameters and coordinates updates to the renderer;
         *       actual shading behaviour depends on the active renderer implementation.
         *
         * @see render::IGraphicsLight, render::IGraphicsSceneNode
         */
        class WPCore_API Light : public Component
        {
        public:
            /** Hash used to identify light components (unique per component type). */
            static const hash_type lightHash;

            /** Property name: diffuse colour (key used in `Properties`). */
            static const String diffuseColourStr;

            /** Property name: specular colour (key used in `Properties`). */
            static const String specularColourStr;

            /** Property name: range (used for point/spot lights; radius in engine units). */
            static const String rangeStr;

            /** Property name: constant attenuation term (A0). */
            static const String constantStr;

            /** Property name: linear attenuation term (A1). */
            static const String linearStr;

            /** Property name: quadratic attenuation term (A2). */
            static const String quadraticStr;

            /** Property name: intensity multiplier applied to colours. */
            static const String intensityStr;

            /** Property name: light type (directional/point/spot). */
            static const String lightTypeStr;

            /** Property name: debug line colour used in the editor visualisation. */
            static const String debugColourStr;

            /**
             * @brief String enumeration used when serializing/deserializing the light type.
             *
             * Contains string names that map to `render::IGraphicsLight::LightTypes` values and is
             * used by `setProperties`/`getProperties` for human-readable representation.
             */
            static const Array<String> lightTypeEnum;

            /**
             * @brief Default constructor.
             *
             * Initializes member defaults:
             *  - `m_diffuseColour` and `m_specularColour` to white
             *  - `m_lightType` to `LT_DIRECTIONAL`
             *  - attenuation and intensity to sensible defaults
             *
             * The renderer light and scene node are not created until `load` or the
             * renderer is available.
             */
            Light();

            /**
             * @brief Virtual destructor.
             *
             * Ensures renderer resources are released if still held. Prefer calling
             * `unload` explicitly from the owning system to ensure deterministic cleanup.
             */
            ~Light() override;

            /**
             * @copydoc Component::load
             *
             * When loading, the component will create or acquire the renderer `ILight`
             * object and a scene node if required, and apply stored properties from
             * `data` (if present). `data` is expected to be a `Properties`-like object
             * containing serialised values for the property keys declared above.
             *
             * @param data Optional shared object containing serialized properties.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::unload
             *
             * Releases ownership of the renderer `ILight` and `IGraphicsSceneNode` (if present),
             * and clears internal references so the component can be safely destroyed or
             * re-loaded later.
             *
             * @param data Optional shared object provided by the framework for unload-time context.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::updateFlags
             *
             * Called whenever component flags change (e.g. visibility, debug). The light
             * component updates renderer visibility and debug drawing state accordingly.
             *
             * @param flags New flags bitset.
             * @param oldFlags Previous flags bitset.
             */
            void updateFlags( u32 flags, u32 oldFlags ) override;

            /**
             * @copydoc Component::getChildObjects
             *
             * Returns any child shared objects owned by this component (for example the
             * renderer scene node or light object when they are represented as shared objects).
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @copydoc Component::getProperties
             *
             * Produces a `Properties` object that contains serializable state for this
             * component: diffuse/specular colours, light type, range, attenuation terms
             * and intensity.
             *
             * @return Smart pointer to a `Properties` instance containing current values.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc Component::setProperties
             *
             * Applies known keys from `properties` to this component. Unknown keys are
             * ignored to preserve forward compatibility. Values are validated where
             * appropriate and applied to the renderer object if it exists.
             *
             * @param properties Properties object containing values to apply.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @copydoc Component::updateTransform
             *
             * Synchronises the internal `IGraphicsSceneNode` transform with the component's
             * transform so the renderer light follows the scene component.
             */
            void updateTransform() override;

            /**
             * @copydoc Component::updateTransform
             *
             * Applies the supplied transform to the internal scene node instead of the
             * component's stored transform.
             *
             * @param transform Transform to apply to the light's scene node.
             */
            void updateTransform( const Transform3<real_Num> &transform ) override;

            /**
             * @brief Get the current light type.
             * @return Current light type (see `render::IGraphicsLight::LightTypes`).
             */
            LightTypes getLightType() const;

            /**
             * @brief Set the light type.
             * @param lightType New light type (directional, point, spot, ...).
             *
             * Changing the light type may require recreating or reconfiguring the
             * underlying renderer light. This call updates the component's stored type
             * and attempts to apply the change immediately if a renderer object exists.
             */
            void setLightType( LightTypes lightType );

            /**
             * @brief Set attenuation in one call.
             * @param range Effective radius for point/spot lights (engine units).
             * @param constant Constant attenuation term (A0).
             * @param linear Linear attenuation term (A1).
             * @param quadratic Quadratic attenuation term (A2).
             */
            void setAttenuation( f32 range, f32 constant, f32 linear, f32 quadratic );

            /**
             * @brief Get the diffuse colour of the light.
             * @return RGBA diffuse colour in floating point components.
             */
            ColourF getDiffuseColour() const;

            /**
             * @brief Set the diffuse colour of the light.
             * @param diffuseColour RGBA diffuse colour in floating point components.
             */
            void setDiffuseColour( const ColourF &diffuseColour );

            /**
             * @brief Get the specular colour of the light (used for highlights).
             * @return RGBA specular colour in floating point components.
             */
            ColourF getSpecularColour() const;

            /**
             * @brief Set the specular colour of the light.
             * @param specularColour RGBA specular colour in floating point components.
             */
            void setSpecularColour( const ColourF &specularColour );

            /**
             * @brief Retrieve the renderer light object owned by this component.
             * @return Smart pointer to `render::IGraphicsLight` or null if not created.
             *
             * Ownership remains with the component; callers should not destroy the
             * returned object.
             */
            SmartPtr<render::IGraphicsLight> getLight() const;

            /**
             * @brief Replace the renderer light used by this component.
             * @param light Smart pointer to a renderer `ILight`. Ownership is transferred.
             *
             * Use with care: replacing the renderer object bypasses normal property
             * synchronization flows and may require additional manual configuration.
             */
            void setLight( SmartPtr<render::IGraphicsLight> light );

            /**
             * @brief Access the scene node used to position the light.
             * @return Non-const reference to the smart pointer holding the `IGraphicsSceneNode`.
             *
             * The returned reference can be used to attach/detach children or replace
             * the node. The component will use this node when applying transforms.
             */
            SmartPtr<render::IGraphicsSceneNode> &getSceneNode();

            /**
             * @brief Const access to the scene node used by this light.
             * @return Const reference to the smart pointer holding the `IGraphicsSceneNode`.
             */
            const SmartPtr<render::IGraphicsSceneNode> &getSceneNode() const;

            /**
             * @brief Set the scene node used by this component.
             * @param sceneNode Smart pointer to the new `IGraphicsSceneNode`.
             *
             * The component will use the supplied node to position and orient the
             * renderer light. Passing `nullptr` detaches the component from any node.
             */
            void setSceneNode( SmartPtr<render::IGraphicsSceneNode> sceneNode );

            /** Getters for individual attenuation parameters. */
            real_Num getAttenuationRange() const;
            real_Num getAttenuationConstant() const;
            real_Num getAttenuationLinear() const;
            real_Num getAttenuationQuadratic() const;

            /** Setters for individual attenuation parameters. */
            void setAttenuationRange( real_Num range );
            void setAttenuationConstant( real_Num constant );
            void setAttenuationLinear( real_Num linear );
            void setAttenuationQuadratic( real_Num quadratic );

            /** Get and set the global intensity multiplier for this light. */
            f32 getIntensity() const;
            void setIntensity( f32 intensity );

            /** Get and set the ARGB colour used when drawing the debug line for this light. */
            u32 getDebugColour() const;
            void setDebugColour( u32 colour );

            /** Set the light's visible state in the renderer. */
            void setVisible( bool visible );

            /** Query whether the light is currently visible to the renderer. */
            bool isVisible() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Handles finite-state-machine events specific to the component.
             *
             * Responds to state transitions (enable/disable, attach/detach from scene)
             * and performs any necessary renderer updates.
             *
             * @param state Current FSM state.
             * @param eventType Event identifier.
             * @return FSMReturnType indicating the outcome of event handling.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @brief Handle framework events targeted at this component.
             *
             * Used for receiving arbitrary engine events that the component may react to.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /**
             * @brief Update debug visualisation for the light.
             *
             * Depending on the light type and active debug settings this method may draw
             * an icon at the light position, a sphere indicating light range, or an arrow
             * showing the direction of a directional/spot light.
             */
            void updateDebugDraw();

            /** Diffuse colour used when shading surfaces lit by this light. Defaults to white. */
            ColourF m_diffuseColour = ColourF::White;

            /** Specular colour used when computing highlights. Defaults to white. */
            ColourF m_specularColour = ColourF::White;

            /** Current light type (directional / point / spot). Defaults to directional. */
            LightTypes m_lightType = LightTypes::LT_DIRECTIONAL;

            /** Renderer light object owned by the component. May be null if not created. */
            SmartPtr<render::IGraphicsLight> m_light;

            /** Scene node that positions the renderer light. May be null. */
            SmartPtr<render::IGraphicsSceneNode> m_sceneNode;

            /** Range (radius) for point / spot lights. Units are engine units. */
            f32 m_range = 1000.0f;

            /** Constant attenuation factor (A0 in attenuation equation). */
            f32 m_constant = 1.0f;

            /** Linear attenuation factor (A1 in attenuation equation). */
            f32 m_linear = 1.0f;

            /** Quadratic attenuation factor (A2 in attenuation equation). */
            f32 m_quadratic = 1.0f;

            /** Intensity multiplier applied to both diffuse and specular components. */
            f32 m_intensity = 1.0f;

            /** ARGB colour used when drawing the debug visualisation line for this light. */
            u32 m_debugColour = 0xFFFFFF;

            /** Internal counter used to generate unique names/IDs for light instances. */
            static u32 m_nameExt;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // LightComponent_h__
