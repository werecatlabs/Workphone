#include <EditorPCH.hpp>
#include <commands/ModifyPropertyCmd.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, ModifyPropertyCmd, Command );

    ModifyPropertyCmd::ModifyPropertyCmd()
    {
        setPrimary( true );
    }

    ModifyPropertyCmd::~ModifyPropertyCmd() = default;

    bool ModifyPropertyCmd::configure( SmartPtr<AssetDatabaseManager> catalog,
                                       const AssetDatabaseManager::EntrySnapshot &entry,
                                       const Array<SmartPtr<ISharedObject>> &targets,
                                       const String &property, hash_type expectedType )
    {
        if( !m_changes.empty() || targets.empty() || targets.size() > 256 )
        {
            m_error = "Reference command requires 1 to 256 targets and can only be configured once.";
            return false;
        }
        auto app = core::IApplicationManager::instancePtr();
        auto resources = app ? app->getResourceDatabase() : nullptr;
        auto project =
            resources ? dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() )
                      : nullptr;
        if( !project )
        {
            m_error = "No active project catalog.";
            return false;
        }
        if( !m_projectSession.capture( project ) )
        {
            m_error = "Cannot capture the active project catalog session.";
            return false;
        }
        m_referenceCatalog = catalog;
        if( activeCatalog() != catalog )
        {
            m_error = "The selected catalog is not mounted in the active project session.";
            return false;
        }
        AssetDatabaseManager::EntrySnapshot collision;
        if( catalog != project && project->tryGetEntry( entry.uuid, collision ) )
        {
            m_error = "The selected UUID conflicts with an asset in the project catalog.";
            return false;
        }
        Array<SmartPtr<ISharedObject>> unique;
        for( const auto &target : targets )
        {
            if( std::find( unique.begin(), unique.end(), target ) != unique.end() )
            {
                continue;
            }
            AssetReferenceChange change;
            if( !change.prepare( catalog, entry, target, property, expectedType ) )
            {
                m_error = change.error();
                m_changes.clear();
                return false;
            }
            unique.push_back( target );
            m_changes.push_back( change );
        }
        return !m_changes.empty();
    }

    SmartPtr<AssetDatabaseManager> ModifyPropertyCmd::activeCatalog() const
    {
        auto app = core::IApplicationManager::instancePtr();
        auto resources = app ? app->getResourceDatabase() : nullptr;
        auto project =
            resources ? dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() )
                      : nullptr;
        String root, error;
        if( !project || !m_projectSession.isCurrent( project ) || app->getProjectPath().empty() ||
            !canonicalAssetCatalogRoot( app->getProjectPath(), root, error ) ||
            root != project->getProjectRoot() )
        {
            return nullptr;
        }
        auto selected = m_referenceCatalog.lock();
        if( selected == project )
        {
            return project;
        }
        auto concrete = dynamic_pointer_cast<ResourceDatabase>( resources );
        return concrete && selected && concrete->getMediaCatalog() == selected ? selected : nullptr;
    }

    void ModifyPropertyCmd::markDirty()
    {
        for( const auto &change : m_changes )
        {
            auto target = change.target();
            if( !target )
            {
                continue;
            }
            if( auto actor = dynamic_pointer_cast<scene::IGameActor>( target ) )
            {
                actor->setDirty( true );
            }
            if( auto component = dynamic_pointer_cast<scene::IComponent>( target ) )
            {
                if( auto actor = component->getActor() )
                {
                    actor->setDirty( true );
                }
            }
            if( auto material = dynamic_pointer_cast<render::IMaterial>( target ) )
            {
                material->makeDirty();
            }
            if( auto node = dynamic_pointer_cast<render::IMaterialNode>( target ) )
            {
                if( auto material = node->getMaterial() )
                {
                    material->makeDirty();
                }
            }
        }
    }

    void ModifyPropertyCmd::execute()
    {
        auto catalog = activeCatalog();
        m_error.clear();
        if( m_changes.empty() || m_applied )
        {
            return;
        }
        for( auto &change : m_changes )
        {
            if( !change.canApply( catalog ) )
            {
                m_error = change.error();
                setState( State::Finished );
                return;
            }
        }
        size_t count = 0;
        for( auto &change : m_changes )
        {
            if( !change.apply( catalog ) )
            {
                m_error = change.error();
                while( count > 0 )
                {
                    if( !m_changes[--count].undo( catalog ) )
                    {
                        m_error += " Rollback failed: " + m_changes[count].error();
                    }
                }
                markDirty();
                setState( State::Finished );
                return;
            }
            ++count;
        }
        m_applied = true;
        markDirty();
        setState( State::Finished );
    }

    void ModifyPropertyCmd::undo()
    {
        auto catalog = activeCatalog();
        m_error.clear();
        if( !m_applied )
        {
            return;
        }
        for( auto &change : m_changes )
        {
            if( !change.canUndo( catalog ) )
            {
                m_error = change.error();
                return;
            }
        }
        size_t undone = 0;
        for( size_t i = m_changes.size(); i > 0; --i )
        {
            if( !m_changes[i - 1].undo( catalog ) )
            {
                m_error = m_changes[i - 1].error();
                for( size_t j = i; j < i + undone; ++j )
                {
                    if( !m_changes[j].apply( catalog ) )
                    {
                        m_error += " Rollback failed: " + m_changes[j].error();
                    }
                }
                markDirty();
                return;
            }
            ++undone;
        }
        m_applied = false;
        markDirty();
    }

    void ModifyPropertyCmd::redo()
    {
        execute();
    }

    const String &ModifyPropertyCmd::getError() const
    {
        return m_error;
    }

    bool ModifyPropertyCmd::wasApplied() const
    {
        return m_applied;
    }
}  // namespace workphone::editor
