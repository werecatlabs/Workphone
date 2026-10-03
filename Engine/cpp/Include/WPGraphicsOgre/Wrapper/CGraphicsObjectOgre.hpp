#ifndef CGraphicsObjectOgre_h__
#define CGraphicsObjectOgre_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Graphics/GraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/Handle.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/State/States/GraphicsObjectData.hpp>
#include <OgreMovableObject.h>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Lightweight wrapper that connects a GraphicsObject to an Ogre movable object.
         *
         * This template class connects engine-level graphics objects (GraphicsObject<T>)
         * to Ogre3D primitives via an Ogre::MovableObject pointer. It provides common
         * helper methods used by concrete Ogre-backed graphics objects (meshes, lights, overlays, etc.).
         *
         * @tparam T The concrete graphics resource type managed by the base GraphicsObject.
         */
        template <class T>
        class CGraphicsObjectOgre : public GraphicsObject<T>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Does not allocate Ogre resources. Use derived classes / factory methods to create and attach
             * an underlying Ogre::MovableObject.
             */
            CGraphicsObjectOgre();

            /**
             * @brief Destructor.
             *
             * Calls base class unload to ensure any managed resources are released.
             */
            ~CGraphicsObjectOgre() override;

            /**
             * @brief Unload the object and release runtime resources.
             *
             * Calls into the GraphicsObject<T>::unload implementation. Provided here for convenience
             * so callers can pass through to the templated base unload without resolving the base type.
             *
             * @param data Optional shared object data used for unloading (may be null).
             */
            void unload( SmartPtr<ISharedObject> data )
            {
                GraphicsObject<T>::unload( data );
            }

            /**
             * @brief Set the material name used by the underlying Ogre object.
             *
             * Derived classes should override to apply the material name to the specific Ogre resource.
             * The base implementation is a no-op.
             *
             * @param materialName Name of the material to apply.
             * @param index Optional sub-index (e.g. sub-entity or sub-material). Default is -1 (apply to all).
             */
            virtual void setMaterialName( const String &materialName, s32 index = -1 );

            /**
             * @brief Get the material name from the underlying Ogre object.
             *
             * Derived classes should override to return the material name used by the concrete Ogre resource.
             * The base implementation returns an empty string.
             *
             * @param index Optional sub-index (e.g. sub-entity or sub-material). Default is -1 (return for first/combined).
             * @return Material name or empty string if none.
             */
            virtual String getMaterialName( s32 index = -1 ) const;

            /**
             * @brief Clone the graphics object.
             *
             * Concrete implementations should perform a deep or shallow clone as required and
             * return a new IGraphicsObject instance representing the copy.
             *
             * @param name Optional name for the cloned object. Default is empty.
             * @return SmartPtr to the cloned object or nullptr if cloning is not supported.
             */
            virtual SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            /**
             * @brief Get the underlying raw object pointer.
             *
             * Writes the internal object pointer (Ogre pointer or equivalent) to the provided pointer.
             * The pointer written is specific to the derived implementation.
             *
             * @param ppObject Output pointer which receives the raw pointer. May be set to nullptr on failure.
             */
            virtual void _getObject( void **ppObject ) const override;

            /**
             * @brief Attach this graphics object to a scene node.
             *
             * Implementations should attach the internal Ogre::MovableObject to the supplied scene node
             * or perform equivalent parent-child linking.
             *
             * @param parent Scene node to attach to.
             */
            virtual void attachToParent( SmartPtr<IGraphicsSceneNode> parent ) override;

            /**
             * @brief Detach this graphics object from a scene node.
             *
             * Implementations should detach the internal Ogre::MovableObject from the supplied scene node.
             *
             * @param parent Scene node to detach from.
             */
            virtual void detachFromParent( SmartPtr<IGraphicsSceneNode> parent ) override;

            /**
             * @brief Retrieve a Properties container describing this object.
             *
             * The returned Properties should include high-level fields useful for tooling and inspection,
             * such as the owner name and visible state.
             *
             * @return SmartPtr to a Properties instance (never null).
             */
            virtual SmartPtr<Properties> getProperties() const;

            /**
             * @brief Apply properties from a Properties container to this object.
             *
             * This allows generic tooling or serialization code to set simple attributes (owner, visible, etc.).
             * Derived classes may read and apply additional properties.
             *
             * @param properties Properties object containing values to apply.
             */
            virtual void setProperties( SmartPtr<Properties> properties );

            /**
             * @brief Get the scene that created this graphics object.
             *
             * @return SmartPtr to the creator IGraphicsScene or null if none.
             */
            SmartPtr<IGraphicsScene> getCreator() const;

            /**
             * @brief Set the scene that created this graphics object.
             *
             * @param creator SmartPtr to the creating IGraphicsScene.
             */
            void setCreator( SmartPtr<IGraphicsScene> creator );

            /**
             * @brief Get the raw Ogre::MovableObject associated with this object.
             *
             * @return Pointer to the Ogre::MovableObject, or nullptr if none.
             */
            Ogre::MovableObject *getMovable() const;

            /**
             * @brief Set the raw Ogre::MovableObject associated with this object.
             *
             * Stores the pointer locally and also records this wrapper via Ogre's user data interface.
             * The caller remains owner of the Ogre::MovableObject lifetime unless otherwise agreed.
             *
             * @param movable Pointer to an existing Ogre::MovableObject. May be nullptr.
             */
            void setMovable( Ogre::MovableObject *movable );

            WP_CLASS_REGISTER_TEMPLATE_DECL( CGraphicsObjectOgre, T );

        protected:
            /// Raw pointer to the Ogre movable object. May be null.
            Ogre::MovableObject *m_movable = nullptr;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, CGraphicsObjectOgre, T, GraphicsObject<T> );

        /**
         * @brief State listener used to respond to engine state changes for the graphics object.
         *
         * This listener reacts to state changes (for example visibility or mask changes)
         * and applies them to the underlying Ogre::MovableObject when the owner is loaded.
         */
        class GraphicsObjectOgreStateListener : public IStateListener
        {
        public:
            GraphicsObjectOgreStateListener();
            ~GraphicsObjectOgreStateListener() override;

            /**
             * @brief Handle an incoming state message.
             *
             * Derived listeners typically use messages to trigger finer-grained updates. The default
             * implementation does nothing and returns false.
             *
             * @param message The state message to process.
             * @return True if the message was handled, false otherwise.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Handle a state change notification.
             *
             * This implementation reads GraphicsObjectData from the state and applies visibility and
             * visibility mask changes to the owner's Ogre::MovableObject (if present and loaded).
             *
             * @param state The state that has changed.
             * @return True if the change was handled, false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            /**
             * @brief Get the owner CGraphicsObjectOgre instance.
             *
             * Returns a strong reference to the owner if it still exists.
             *
             * @return SmartPtr to the owning CGraphicsObjectOgre or null.
             */
            virtual SmartPtr<IGraphicsObject> getOwner() const;

            /**
             * @brief Set the owner CGraphicsObjectOgre instance.
             *
             * The listener stores a weak (atomic) pointer to avoid reference cycles.
             *
             * @param owner SmartPtr to the owning CGraphicsObjectOgre.
             */
            virtual void setOwner( SmartPtr<IGraphicsObject> owner );

            /// Atomic smart pointer to the owning CGraphicsObjectOgre to avoid lifetime cycles.
            AtomicSmartPtr<IGraphicsObject> m_owner;
        };

        template <class T>
        CGraphicsObjectOgre<T>::CGraphicsObjectOgre()
        {
        }

        template <class T>
        CGraphicsObjectOgre<T>::~CGraphicsObjectOgre()
        {
            GraphicsObject<T>::unload( nullptr );
        }

        template <class T>
        void CGraphicsObjectOgre<T>::setMaterialName( [[maybe_unused]] const String &materialName,
                                                      [[maybe_unused]] s32 index )
        {
        }

        template <class T>
        String CGraphicsObjectOgre<T>::getMaterialName( [[maybe_unused]] s32 index ) const
        {
            return StringUtil::EmptyString;
        }

        template <class T>
        SmartPtr<IGraphicsObject> CGraphicsObjectOgre<T>::clone( const String &name ) const
        {
            return nullptr;
        }

        template <class T>
        void CGraphicsObjectOgre<T>::_getObject( void **ppObject ) const
        {
            *ppObject = m_movable;
        }

        template <class T>
        void CGraphicsObjectOgre<T>::attachToParent( SmartPtr<IGraphicsSceneNode> parent )
        {
        }

        template <class T>
        void CGraphicsObjectOgre<T>::detachFromParent( SmartPtr<IGraphicsSceneNode> parent )
        {
        }

        template <class T>
        Ogre::MovableObject *CGraphicsObjectOgre<T>::getMovable() const
        {
            return m_movable;
        }

        template <class T>
        void CGraphicsObjectOgre<T>::setMovable( Ogre::MovableObject *movable )
        {
            m_movable = movable;
            m_movable->setUserAny( this );
        }

        template <class T>
        SmartPtr<Properties> CGraphicsObjectOgre<T>::getProperties() const
        {
            auto properties = workphone::make_ptr<Properties>();

            auto name = String( "null" );

            if( auto owner = GraphicsObject<T>::getOwner() )
            {
                name = owner->getName();
            }

            properties->setProperty( "owner", name );

            auto visible = GraphicsObject<T>::isVisible();
            properties->setProperty( "visible", visible );
            return properties;
        }

        template <class T>
        void CGraphicsObjectOgre<T>::setProperties( [[maybe_unused]] SmartPtr<Properties> properties )
        {
            auto name = String();
            properties->getPropertyValue( "owner", name );

            auto visible = GraphicsObject<T>::isVisible();
            properties->getPropertyValue( "visible", visible );
        }

        template <class T>
        SmartPtr<IGraphicsScene> CGraphicsObjectOgre<T>::getCreator() const
        {
            auto p = GraphicsObject<T>::m_creator.load();
            return p.lock();
        }

        template <class T>
        void CGraphicsObjectOgre<T>::setCreator( SmartPtr<IGraphicsScene> creator )
        {
            GraphicsObject<T>::m_creator = creator;
        }

    }  // end namespace render
}  // namespace workphone

#endif  // CGraphicsObject_h__
