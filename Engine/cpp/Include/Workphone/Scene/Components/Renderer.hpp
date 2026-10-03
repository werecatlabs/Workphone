#ifndef __Renderer_h__
#define __Renderer_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class Renderer
         * @brief Component responsible for rendering objects in the scene.
         *
         * The Renderer class manages the rendering of scene objects, including material assignment,
         * shadow casting and receiving, reflections, occlusion, and bounding box calculations.
         * It provides interfaces for updating materials, handling events, and managing graphics
         * objects/nodes.
         */
        class WPCore_API Renderer : public Component
        {
        public:
            /** @brief String identifier for shadow casting property. */
            static const String castShadowsStr;

            /** @brief String identifier for shadow receiving property. */
            static const String recieveShadowsStr;

            /** @brief String identifier for reflections property. */
            static const String reflectionsStr;

            /** @brief String identifier for occlusion property. */
            static const String occulsionStr;

            /** @brief String identifier for material update property. */
            static const String materialUpdateStr;

            /** @brief String identifier for bounds update property. */
            static const String updateBoundsStr;

            /** @brief String identifier for bounding box property. */
            static const String boundingBoxStr;

            /** @brief String identifier for Z-order / render queue property. */
            static const String zOrderStr;

            /** @brief String identifier for visibility flags property. */
            static const String visibilityFlagsStr;

            /** @brief String identifier for material name property. */
            static const String materialNameStr;

            /** @brief State names for shadow casting. */
            static const Array<String> castShadowStateNames;

            /** @brief State names for shadow receiving. */
            static const Array<String> recieveShadowsNames;

            /** @brief State names for reflections. */
            static const Array<String> reflectionsStateNames;

            /** @brief State names for occlusion. */
            static const Array<String> occulsionStateNames;

            /**
             * @enum CastShadows
             * @brief Shadow casting modes for the renderer.
             */
            enum class CastShadows : u8
            {
                Off,         /**< Do not cast shadows. */
                On,          /**< Cast shadows normally. */
                DoubleSided, /**< Cast double-sided shadows. */
                ShadowsOnly  /**< Only render shadows, not the object itself. */
            };

            /**
             * @enum RecieveShadows
             * @brief Shadow receiving modes for the renderer.
             */
            enum class RecieveShadows : u8
            {
                Off, /**< Do not receive shadows. */
                On   /**< Receive shadows. */
            };

            /**
             * @enum Reflections
             * @brief Reflection modes for the renderer.
             */
            enum class Reflections : u8
            {
                Off,     /**< No reflections. */
                Default, /**< Default reflection mode. */
                Blend,   /**< Blended reflections. */
                Simple   /**< Simple reflections. */
            };

            /**
             * @enum Occulsion
             * @brief Occlusion modes for the renderer.
             */
            enum class Occulsion : u8
            {
                Off,     /**< No occlusion. */
                On,      /**< Occlusion enabled. */
                Dynamic, /**< Dynamic occlusion. */
                Static   /**< Static occlusion. */
            };

            /**
             * @brief Default constructor.
             */
            Renderer();

            /**
             * @brief Destructor.
             */
            ~Renderer() override;

            /**
             * @copydoc Component::load
             * @param data Shared object data for loading.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::unload
             * @param data Shared object data for unloading.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::updateFlags
             * @param flags New flags value.
             * @param oldFlags Previous flags value.
             */
            void updateFlags( u32 flags, u32 oldFlags ) override;

            /**
             * @copydoc Component::getChildObjects
             * @return Array of child shared objects.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @copydoc Component::getProperties
             * @return Properties of the renderer.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc Component::setProperties
             * @param properties Properties to set.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Gets the shadow casting mode.
             * @return Current CastShadows enum value.
             */
            CastShadows getCastShadows() const;

            /**
             * @brief Sets the shadow casting mode and propagates to the graphics object.
             * @param castShadows New CastShadows value.
             */
            void setCastShadows( CastShadows castShadows );

            /**
             * @brief Gets the shadow receiving mode.
             * @return Current RecieveShadows enum value.
             */
            RecieveShadows getRecieveShadows() const;

            /**
             * @brief Sets the shadow receiving mode and propagates to the graphics object.
             * @param recieveShadows New RecieveShadows value.
             */
            void setRecieveShadows( RecieveShadows recieveShadows );

            /**
             * @brief Gets the reflection mode.
             * @return Current Reflections enum value.
             */
            Reflections getReflections() const;

            /**
             * @brief Sets the reflection mode.
             * @param reflections New Reflections value.
             */
            void setReflections( Reflections reflections );

            /**
             * @brief Gets the occlusion mode.
             * @return Current Occulsion enum value.
             */
            Occulsion getOcculsion() const;

            /**
             * @brief Sets the occlusion mode.
             * @param occulsion New Occulsion value.
             */
            void setOcculsion( Occulsion occulsion );

            /**
             * @brief Gets the Z-order / render queue group.
             * @return Current Z-order value.
             */
            u32 getZOrder() const;

            /**
             * @brief Sets the Z-order / render queue group and propagates to the graphics object.
             * @param zOrder New Z-order value.
             */
            void setZOrder( u32 zOrder );

            /**
             * @brief Gets the visibility flags bitmask.
             * @return Current visibility flags.
             */
            u32 getVisibilityFlags() const;

            /**
             * @brief Sets the visibility flags bitmask and propagates to the graphics object.
             * @param flags New visibility flags.
             */
            void setVisibilityFlags( u32 flags );

            /**
             * @brief Gets the shared material used by the renderer.
             * @return Shared material smart pointer.
             */
            SmartPtr<render::IMaterial> getSharedMaterial() const;

            /**
             * @brief Sets the shared material for the renderer.
             * @param sharedMaterial Shared material smart pointer.
             */
            void setSharedMaterial( SmartPtr<render::IMaterial> sharedMaterial );

            /**
             * @brief Gets the material name used by the renderer.
             * @return Material name string.
             */
            String getMaterialName() const;

            /**
             * @brief Sets the material name for the renderer.
             * @param materialName Name of the material.
             */
            void setMaterialName( const String &materialName );

            /**
             * @brief Updates the materials used by the renderer.
             */
            void updateMaterials() override;

            /**
             * @copydoc Component::updateVisibility
             */
            void updateVisibility() override;

            /**
             * @brief Sets the visibility contribution controlled by LODGroup.
             *
             * This is independent of the component and actor enabled flags, so selecting an LOD
             * never mutates authored component state.
             */
            void setLODVisible( bool visible );

            /** @return True when this renderer is enabled by its owning LODGroup. */
            bool isLODVisible() const;

            /**
             * @brief Handles an event for the renderer.
             * @param eventType Type of the event.
             * @param eventValue Value associated with the event.
             * @param arguments Arguments for the event.
             * @param sender Sender of the event.
             * @param object Object associated with the event.
             * @param event Event smart pointer.
             * @return Parameter result of the event handling.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /**
             * @copydoc IComponent::updateTransform
             */
            void updateTransform() override;

            /**
             * @copydoc IComponent::updateTransform
             * @param transform Transform to apply.
             */
            void updateTransform( const Transform3<real_Num> &transform ) override;

            /**
             * @brief Gets the graphics object used by the renderer.
             * @return Graphics object smart pointer.
             */
            SmartPtr<render::IGraphicsObject> getGraphicsObject() const;

            /**
             * @brief Sets the graphics object for the renderer.
             * @param graphicsObject Graphics object smart pointer.
             */
            void setGraphicsObject( SmartPtr<render::IGraphicsObject> graphicsObject );

            /**
             * @brief Gets the graphics node used by the renderer.
             * @return Reference to graphics node smart pointer.
             */
            SmartPtr<render::IGraphicsSceneNode> getGraphicsNode() const;

            /**
             * @brief Sets the graphics node for the renderer.
             * @param graphicshNode Graphics node smart pointer.
             */
            void setGraphicsNode( SmartPtr<render::IGraphicsSceneNode> graphicshNode );

            /**
             * @copydoc Component::getBoundingBox
             * @return Axis-aligned bounding box of the renderer.
             */
            AABB3<real_Num> getBoundingBox() const override;

            /**
             * @brief Gets the graphics object used by the renderer.
             * @return Graphics object smart pointer.
             */
            template <class T>
            SmartPtr<T> getGraphicsObjectByType() const;

            /**
             * @brief Gets the graphics object used by the renderer.
             * @return Graphics object smart pointer.
             */
            template <class T>
            T *getGraphicsObjectByTypePtr() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @copydoc Component::handleComponentEvent
             * @param state Current state.
             * @param eventType Event type.
             * @return FSM return type.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @brief Applies all stored rendering state (shadows, z-order, visibility flags)
             *        to the current graphics object.
             */
            void applyGraphicsState();

            /**
             * @brief Updates the mesh associated with the renderer.
             */
            virtual void updateMesh();

            /**
             * @brief Sets up the transform update for static objects.
             */
            void updateStatic() override;

            /**
             * @brief Calculates the axis-aligned bounding box for the renderer.
             * @return Calculated bounding box.
             */
            virtual AABB3<real_Num> calculateBoundingBox();

            /** @brief Axis-aligned bounding box for the renderer. */
            AtomicObject<AABB3<real_Num>> m_boundingBox;

            /** @brief Graphics object associated with the renderer. */
            AtomicSmartPtr<render::IGraphicsObject> m_graphicsObject;

            /** @brief Graphics node associated with the renderer. */
            AtomicSmartPtr<render::IGraphicsSceneNode> m_graphicsNode;

            /** @brief Shared material used by the renderer. */
            AtomicSmartPtr<render::IMaterial> m_sharedMaterial;

            /** @brief Shadow casting mode. */
            AtomicValue<CastShadows> m_castShadows = CastShadows::On;

            /** @brief Shadow receiving mode. */
            AtomicValue<RecieveShadows> m_recieveShadows = RecieveShadows::On;

            /** @brief Reflection mode. */
            AtomicValue<Reflections> m_reflections = Reflections::Default;

            /** @brief Occlusion mode. */
            AtomicValue<Occulsion> m_occulsion = Occulsion::On;

            /** @brief Z-order / render queue group. */
            AtomicValue<u32> m_zOrder = 0u;

            /** @brief Visibility flags bitmask. */
            AtomicValue<u32> m_visibilityFlags = 0xFFFFFFFFu;

            /** @brief Visibility contribution supplied by the LOD system. */
            atomic_bool m_lodVisible = true;

            /** @brief Name of the material used by the renderer. */
            AtomicObject<String> m_materialName;

            /** @brief List of materials used by the renderer. */
            ConcurrentArray<SmartPtr<render::IMaterial>> m_materials;

            /** @brief List of shared materials used by the renderer. */
            ConcurrentArray<SmartPtr<render::IMaterial>> m_sharedMaterials;

            /** @brief Static ID extension for the renderer. */
            static u32 m_idExt;
        };

        /**
         * @brief Gets the graphics node used by the renderer.
         * @return Reference to graphics node smart pointer.
         */
        inline SmartPtr<render::IGraphicsSceneNode> Renderer::getGraphicsNode() const
        {
            return m_graphicsNode;
        }

        template <class T>
        SmartPtr<T> Renderer::getGraphicsObjectByType() const
        {
            return m_graphicsObject.load();
        }

        template <class T>
        T *Renderer::getGraphicsObjectByTypePtr() const
        {
            return (T *)m_graphicsObject.get();
        }

    }  // namespace scene
}  // namespace workphone

#endif  // __Renderer_h__
