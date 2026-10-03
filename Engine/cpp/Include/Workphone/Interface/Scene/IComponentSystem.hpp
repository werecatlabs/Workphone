#ifndef IComponentSystem_h__
#define IComponentSystem_h__

#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Abstract interface for the entity component system.
         *
         * This class provides the core functionality for managing components within the entity component
         * system. It handles component lifecycle, storage, and state management. The system maintains a
         * collection of components and provides methods to add, remove, and manage them efficiently.
         */
        class WPCore_API IComponentSystem : public IEventListener
        {
        public:
            /**
             * @brief Virtual destructor.
             * Ensures proper cleanup of derived classes.
             */
            ~IComponentSystem() override;

            /**
             * @brief Adds a component to the system.
             * @param component The component to add to the system.
             * @return The unique identifier assigned to the component.
             */
            virtual u32 addComponent( SmartPtr<IComponent> component ) = 0;

            /**
             * @brief Removes a component from the system.
             * @param component The component to remove from the system.
             */
            virtual void removeComponent( SmartPtr<IComponent> component ) = 0;

            /**
             * @brief Removes a component from the system by its ID.
             * @param id The unique identifier of the component to remove.
             */
            virtual void removeComponent( u32 id ) = 0;

            /**
             * @brief Reserves space for a specified number of components.
             * @param size The number of components to reserve space for.
             */
            virtual void reserve( size_t size ) = 0;

            /**
             * @brief Gets the current number of components in the system.
             * @return The total number of components currently managed by the system.
             */
            virtual size_t getSize() const = 0;

            /**
             * @brief Sets the number of components in the system.
             * @param size The desired number of components.
             */
            virtual void setSize( size_t size ) = 0;

            /**
             * @brief Gets the grow size of the system.
             * @return The number of components by which the system grows when it needs to expand.
             */
            virtual u32 getGrowSize() const = 0;

            /**
             * @brief Sets the grow size of the system.
             * @param growSize The number of components by which the system should grow when expanding.
             */
            virtual void setGrowSize( u32 growSize ) = 0;

            /**
             * @brief Checks if the system is in a dirty state.
             * @return True if the system has pending changes, false otherwise.
             */
            virtual bool isDirty() const = 0;

            /**
             * @brief Sets the dirty state of the system.
             * @param dirty The new dirty state to set.
             */
            virtual void setDirty( bool dirty ) = 0;

            /**
             * @brief Marks the system as dirty, indicating it has pending changes.
             */
            virtual void makeDirty() = 0;

            /**
             * @brief Adds a component to the dirty components list.
             * @param component The component to mark as dirty.
             */
            virtual void addDirtyComponent( SmartPtr<IComponent> component ) = 0;

            /**
             * @brief Removes a component from the dirty components list.
             * @param component The component to remove from the dirty list.
             */
            virtual void removeDirtyComponent( SmartPtr<IComponent> component ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // IComponentSystem_h__
