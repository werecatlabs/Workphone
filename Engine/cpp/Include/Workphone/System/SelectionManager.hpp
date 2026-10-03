#ifndef __CSELECTIONMANAGER_H_INCLUDED__
#define __CSELECTIONMANAGER_H_INCLUDED__

#include <Workphone/Interface/System/ISelectionManager.hpp>

namespace workphone
{

    /** Selection manager class.
     *  This class is used to manage the selection of objects.
     *  It is used by the editor to manage the selection of objects.
     *  @see ISelectionManager
     */
    class WPCore_API SelectionManager : public ISelectionManager
    {
    public:
        /** Constructor. */
        SelectionManager();

        /** Destructor. */
        ~SelectionManager() override;

        /** @copydoc ISelectionManager::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISelectionManager::addSelectedObject */
        void addSelectedObject( SmartPtr<ISharedObject> object ) override;

        /** @copydoc ISelectionManager::removeSelectedObject */
        void removeSelectedObject( SmartPtr<ISharedObject> object ) override;

        /** @copydoc ISelectionManager::clearSelection */
        void clearSelection() override;

        /** @copydoc ISelectionManager::getSelection */
        Array<SmartPtr<ISharedObject>> getSelection() const override;

        WP_CLASS_REGISTER_DECL;

    private:
        // The selected objects.
        Array<SmartPtr<ISharedObject>> m_selection;
    };

}  // namespace workphone

#endif
