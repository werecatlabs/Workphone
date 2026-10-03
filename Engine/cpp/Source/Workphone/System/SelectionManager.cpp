#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/SelectionManager.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, SelectionManager, ISelectionManager );

    SelectionManager::SelectionManager() = default;

    SelectionManager::~SelectionManager()
    {
        unload( nullptr );
    }

    void SelectionManager::unload( SmartPtr<ISharedObject> data )
    {
        m_selection.clear();
    }

    void SelectionManager::addSelectedObject( SmartPtr<ISharedObject> object )
    {
        m_selection.push_back( object );

        auto objectListeners = getObjectListeners();
        for( auto listener : objectListeners )
        {
            if( listener )
            {
                listener->handleEvent( EventType::Scene, IEvent::addSelectedObject, {}, nullptr, object,
                                       nullptr );
            }
        }
    }

    void SelectionManager::removeSelectedObject( SmartPtr<ISharedObject> object )
    {
        auto it = std::find( m_selection.begin(), m_selection.end(), object );
        if( it != m_selection.end() )
        {
            m_selection.erase( it );
        }

        auto objectListeners = getObjectListeners();
        for( auto listener : objectListeners )
        {
            if( listener )
            {
                listener->handleEvent( EventType::Scene, IEvent::deselectObjects, {}, nullptr, object,
                                       nullptr );
            }
        }
    }

    void SelectionManager::clearSelection()
    {
        m_selection.clear();

        auto objectListeners = getObjectListeners();
        for( auto listener : objectListeners )
        {
            if( listener )
            {
                listener->handleEvent( EventType::Scene, IEvent::deselectAll, {}, nullptr, nullptr,
                                       nullptr );
            }
        }
    }

    auto SelectionManager::getSelection() const -> Array<SmartPtr<ISharedObject>>
    {
        return m_selection;
    }

}  // namespace workphone
