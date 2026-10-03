#ifndef Sky_h__
#define Sky_h__

#include <Workphone/Interface/Graphics/ISky.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/State/States/SkyStateData.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Template base for engine sky objects (e.g. skybox, skydome).
         *
         * The Sky class wraps common sky functionality and integrates with the
         * engine's shared graphics object/state system. It exposes methods to
         * manage the sky's material, textures, owning scene, visibility and
         * distance from the camera. State changes are synchronized via the
         * engine's state context using a SkyStateData instance keyed by the
         * object's id.
         *
         * @tparam T Concrete derived type (CRTP style used by SharedGraphicsObject).
         */
        template <class T>
        class Sky : public SharedGraphicsObject<T>
        {
        public:
            /// Construct a new Sky instance and assign a unique id extension.
            Sky();

            /// Destructor, releases any state associations and resources.
            ~Sky() override;

            /**
             * @brief Assign a texture to a specific texture layer.
             *
             * If the object has an associated state context, this will invalidate
             * the stored state data and update the texture for the given layer.
             *
             * @param texture Smart pointer to the texture to set.
             * @param layerIdx Layer index to update (default = 0).
             */
            virtual void setTexture( SmartPtr<ITexture> texture, u32 layerIdx = 0 );

            /**
             * @brief Load and assign a texture from a resource file to a layer.
             *
             * Loads the texture from the application's resource database and,
             * if successful, updates the state's texture for the specified layer.
             *
             * @param fileName Resource path or name of the texture to load.
             * @param layerIdx Layer index to update (default = 0).
             */
            virtual void setTexture( const String &fileName, u32 layerIdx = 0 );

            /**
             * @brief Retrieve all texture layers associated with this sky object.
             *
             * @return A copy of the array of texture smart pointers. If no state
             *         context is available an empty array is returned.
             */
            virtual Array<SmartPtr<ITexture>> getTextures() const;

            /**
             * @brief Replace the texture layers with the provided array.
             *
             * Only the first N layers are copied where N is the minimum of the
             * provided array size and the internal layer count. Remaining layers
             * are cleared.
             *
             * @param textures Array of textures to set.
             */
            virtual void setTextures( const Array<SmartPtr<ITexture>> &textures );

            /**
             * @brief Get the name of the texture currently assigned to a layer.
             *
             * @param layerIdx Index of the layer to query (default = 0).
             * @return Texture resource name or an empty string if none assigned.
             */
            virtual String getTextureName( u32 layerIdx = 0 ) const;

            /**
             * @brief Get the texture assigned to a particular layer.
             *
             * @param layerIdx Index of the texture layer (default = 0).
             * @return Smart pointer to the texture or nullptr if not set.
             */
            virtual SmartPtr<ITexture> getTexture( u32 layerIdx = 0 ) const;

            /**
             * @brief Set the material used by this sky object.
             *
             * The material is stored in the object's state data and will be
             * applied by the rendering implementation that consumes the state.
             *
             * @param material Material smart pointer to set.
             */
            virtual void setMaterial( SmartPtr<IMaterial> material );

            /**
             * @brief Get the material currently associated with the sky.
             *
             * @return Smart pointer to the material or nullptr if not set.
             */
            virtual SmartPtr<IMaterial> getMaterial() const;

            /**
             * @brief Get the scene this sky is attached to.
             *
             * @return Smart pointer to the graphics scene or nullptr if none.
             */
            virtual SmartPtr<IGraphicsScene> getScene() const;

            /**
             * @brief Attach this sky to a scene.
             *
             * Stores the scene in the object's state so renderers can find and
             * use the correct scene context when drawing the sky.
             *
             * @param scene Scene to attach to.
             */
            virtual void setScene( SmartPtr<IGraphicsScene> scene );

            /**
             * @brief Query whether the sky is currently visible.
             *
             * @return true when visible, false otherwise.
             */
            virtual bool isVisible() const;

            /**
             * @brief Set the sky's visibility flag.
             *
             * Changing visibility updates the state's visible flag; renderers
             * should respect this flag when deciding whether to draw the sky.
             *
             * @param visible true to show the sky, false to hide it.
             */
            virtual void setVisible( bool visible );

            /**
             * @brief Get the sky's render distance.
             *
             * Distance can be used by renderers to place the sky at an
             * appropriate radius from the camera (e.g. skybox distance).
             *
             * @return The configured distance value (units are engine-dependent).
             */
            virtual f32 getDistance() const;

            /**
             * @brief Set the sky's render distance.
             *
             * @param distance Distance used by renderers when placing the sky.
             */
            virtual void setDistance( f32 distance );

            /**
             * @brief Handle an incoming state message targeted at this object.
             *
             * Implementations can inspect and respond to messages dispatched
             * by the engine's state system. By default this base class does
             * nothing and returns false.
             *
             * @param message Message to handle.
             * @return true if the message was handled, false otherwise.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Callback invoked after state data for this object changed.
             *
             * Receives the new state object so derived classes can react to
             * external modifications. The base class returns false.
             *
             * @param state The state object that changed.
             * @return true if the change was handled, false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state );

            WP_CLASS_REGISTER_TEMPLATE_DECL( Sky, T );

        protected:
            /// Per-type id extension used to generate stable unique ids.
            static u32 m_idExt;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::render, Sky, T, SharedGraphicsObject<T> );

        template <class T>
        u32 Sky<T>::m_idExt = 0;

        template <class T>
        Sky<T>::Sky()
        {
            auto typeManager = TypeManager::instance();
            WP_ASSERT( typeManager );

            auto typeinfo = Sky<T>::typeInfo();
            auto name = typeManager->getName( typeinfo );
            auto id = StringUtil::getHash( name ) + m_idExt++;
            this->setId( id );
        }

        template <class T>
        Sky<T>::~Sky() = default;

        template <class T>
        void Sky<T>::setMaterial( SmartPtr<IMaterial> material )
        {
            if( auto stateContext = this->getStateContext() )
            {
                if( auto state =
                        stateContext->template invalidateStateDataById<SkyStateData>( this->getId() ) )
                {
                    state->material = material;
                }
            }
        }

        template <class T>
        SmartPtr<IMaterial> Sky<T>::getMaterial() const
        {
            if( auto stateContext = this->getStateContext() )
            {
                if( auto state = stateContext->template getStateDataById<SkyStateData>( this->getId() ) )
                {
                    return state->material;
                }
            }

            return nullptr;
        }

        template <class T>
        void Sky<T>::setTexture( SmartPtr<ITexture> texture, u32 layerIdx )
        {
            if( auto stateContext = this->getStateContext() )
            {
                if( auto state =
                        stateContext->template invalidateStateDataById<SkyStateData>( this->getId() ) )
                {
                    state->textures[layerIdx] = texture;
                }
            }
        }

        template <class T>
        void Sky<T>::setTexture( const String &fileName, u32 layerIdx )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto resourceDatabase = applicationManager->getResourceDatabasePtr();
            WP_ASSERT( resourceDatabase );

            auto texture = resourceDatabase->loadResourceByType<ITexture>( fileName );
            if( texture )
            {
                if( auto stateContext = this->getStateContext() )
                {
                    if( auto state = stateContext->template invalidateStateDataById<SkyStateData>(
                            this->getId() ) )
                    {
                        state->textures[layerIdx] = texture;
                    }
                }
            }
        }

        template <class T>
        Array<SmartPtr<ITexture>> Sky<T>::getTextures() const
        {
            if( auto stateContext = this->getStateContext() )
            {
                if( auto state = stateContext->template getStateDataById<SkyStateData>( this->getId() ) )
                {
                    return { state->textures.begin(), state->textures.end() };
                }
            }

            return {};
        }

        template <class T>
        void Sky<T>::setTextures( const Array<SmartPtr<ITexture>> &textures )
        {
            if( auto stateContext = this->getStateContext() )
            {
                if( auto state =
                        stateContext->template invalidateStateDataById<SkyStateData>( this->getId() ) )
                {
                    state->textures.fill( nullptr );

                    const size_t srcSize = textures.size();
                    const size_t dstSize = state->textures.size();
                    const size_t count = srcSize < dstSize ? srcSize : dstSize;

                    for( size_t i = 0; i < count; ++i )
                    {
                        state->textures[i] = textures[i];
                    }
                }
            }
        }

        template <class T>
        String Sky<T>::getTextureName( u32 layerIdx ) const
        {
            if( auto stateContext = this->getStateContext() )
            {
                if( auto state = stateContext->template getStateDataById<SkyStateData>( this->getId() ) )
                {
                    auto texture = state->textures[layerIdx];
                    if( texture )
                    {
                        return texture->getName();
                    }
                }
            }

            return {};
        }

        template <class T>
        SmartPtr<ITexture> Sky<T>::getTexture( u32 layerIdx ) const
        {
            if( auto stateContext = this->getStateContext() )
            {
                if( auto state = stateContext->template getStateDataById<SkyStateData>( this->getId() ) )
                {
                    return state->textures[layerIdx];
                }
            }

            return nullptr;
        }

        template <class T>
        SmartPtr<IGraphicsScene> Sky<T>::getScene() const
        {
            if( auto stateContext = this->getStateContext() )
            {
                if( auto state = stateContext->template getStateDataById<SkyStateData>( this->getId() ) )
                {
                    return state->scene;
                }
            }

            return nullptr;
        }

        template <class T>
        void Sky<T>::setScene( SmartPtr<IGraphicsScene> scene )
        {
            if( auto stateContext = this->getStateContext() )
            {
                if( auto state =
                        stateContext->template invalidateStateDataById<SkyStateData>( this->getId() ) )
                {
                    state->scene = scene;
                }
            }
        }

        template <class T>
        bool Sky<T>::isVisible() const
        {
            if( auto stateContext = this->getStateContext() )
            {
                if( auto state = stateContext->template getStateDataById<SkyStateData>( this->getId() ) )
                {
                    return state->visible;
                }
            }

            return false;
        }

        template <class T>
        void Sky<T>::setVisible( bool visible )
        {
            if( auto stateContext = this->getStateContext() )
            {
                if( auto state =
                        stateContext->template invalidateStateDataById<SkyStateData>( this->getId() ) )
                {
                    state->visible = visible;
                }
            }
        }

        template <class T>
        f32 Sky<T>::getDistance() const
        {
            if( auto stateContext = this->getStateContext() )
            {
                if( auto state = stateContext->template getStateDataById<SkyStateData>( this->getId() ) )
                {
                    return state->distance;
                }
            }

            return 0.0f;
        }

        template <class T>
        void Sky<T>::setDistance( f32 distance )
        {
            if( auto stateContext = this->getStateContext() )
            {
                if( auto state =
                        stateContext->template invalidateStateDataById<SkyStateData>( this->getId() ) )
                {
                    state->distance = distance;
                }
            }
        }

        template <class T>
        bool Sky<T>::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        template <class T>
        bool Sky<T>::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }

    }  // namespace render
}  // namespace workphone

#endif  // Sky_h__
