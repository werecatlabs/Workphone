#ifndef GraphicsObjectOgreNextStateListener_h__
#define GraphicsObjectOgreNextStateListener_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace render
    {

        /**
             * @class GraphicsObjectListenerOgreNext
             * @brief Listener implementation for graphics objects in OgreNext.
             * 
             * This class implements IStateListener to handle state changes and messages 
             * associated with a specific IGraphicsObject.
             */
            class GraphicsObjectListenerOgreNext : public IStateListener
        {
        public:
            GraphicsObjectListenerOgreNext();
            ~GraphicsObjectListenerOgreNext() override;

            /**
             * @brief Unloads the specified shared data.
             * @param data The shared object data to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Handles a state message.
             * @param message The state message to process.
             * @return True if the message was handled, false otherwise.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Handles a change in state.
             * @param state The new state to process.
             * @return True if the state change was handled, false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            /**
             * @brief Gets the raw pointer to the owner graphics object.
             * @return Pointer to the owner IGraphicsObject.
             */
            IGraphicsObject *getOwnerPtr() const;

            /**
             * @brief Gets a smart pointer to the owner graphics object.
             * @return Smart pointer to the owner IGraphicsObject.
             */
            SmartPtr<IGraphicsObject> getOwner() const;

            /**
             * @brief Sets the owner of this listener.
             * @param owner The graphics object that owns this listener.
             */
            void setOwner( SmartPtr<IGraphicsObject> owner );

            WP_CLASS_REGISTER_DECL;

        protected:
            AtomicWeakPtr<IGraphicsObject> m_owner; ///< Weak pointer to the owning graphics object.
        };

        inline IGraphicsObject *GraphicsObjectListenerOgreNext::getOwnerPtr() const
        {
            return m_owner.get();
        }

    }  // namespace render
}  // namespace workphone

#endif  // GraphicsObjectOgreNextStateListener_h__
