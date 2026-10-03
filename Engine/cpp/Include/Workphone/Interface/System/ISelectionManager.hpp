#ifndef __ISELECTIONMANAGER_H_INCLUDED__
#define __ISELECTIONMANAGER_H_INCLUDED__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /**
     * @class ISelectionManager
     * @brief Interface for managing a selection of shared objects.
     *
     * This interface provides methods for adding, removing, and clearing selected objects,
     * as well as retrieving the current selection. It is intended to be implemented by classes
     * that need to manage a collection of selected objects, such as in an editor or selection tool.
     *
     * @ingroup System
     */
    class WPCore_API ISelectionManager : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor for safe polymorphic destruction.
         */
        ~ISelectionManager() override;

        /**
         * @brief Adds a shared object to the current selection.
         *
         * If the object is already selected, this method may have no effect.
         *
         * @param object The shared pointer to the object to add to the selection. Must not be null.
         * @pre object must be a valid shared pointer.
         * @post The object will be part of the selection if not already present.
         */
        virtual void addSelectedObject( SmartPtr<ISharedObject> object ) = 0;

        /**
         * @brief Removes a shared object from the current selection.
         *
         * If the object is not currently selected, this method may have no effect.
         *
         * @param object The shared pointer to the object to remove from the selection. Must not be null.
         * @pre object must be a valid shared pointer.
         * @post The object will no longer be part of the selection.
         */
        virtual void removeSelectedObject( SmartPtr<ISharedObject> object ) = 0;

        /**
         * @brief Clears all objects from the current selection.
         *
         * After calling this method, the selection will be empty.
         * @post The selection is empty.
         */
        virtual void clearSelection() = 0;

        /**
         * @brief Retrieves the current selection as an array of shared objects.
         *
         * @return Array of shared pointers to the currently selected objects. If no objects are
         * selected, returns an empty array.
         * @note The returned array is a copy of the current selection.
         */
        virtual Array<SmartPtr<ISharedObject>> getSelection() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif
