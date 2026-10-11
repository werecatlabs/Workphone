#ifndef WP_ASSET_CATALOG_BROWSER_HPP
#define WP_ASSET_CATALOG_BROWSER_HPP
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Memory/WeakPtr.hpp>

namespace workphone
{
    /** UI-independent catalog navigation. Call only from its owning UI thread.
     * Does not construct assets or retain an old project/catalog owner. */
    class AssetCatalogBrowser
    {
    public:
        using Query = AssetDatabaseManager::EntryQuery;
        using Page = AssetDatabaseManager::EntryPage;
        using Entry = AssetDatabaseManager::EntrySnapshot;

        void clear()
        {
            m_catalog = nullptr;
            m_page = {};
            m_cursors.clear();
        }
        bool refresh( SmartPtr<AssetDatabaseManager> catalog, const Query &query )
        {
            clear();
            m_catalog = catalog;
            m_query = query;
            return read( catalog );
        }
        bool next( SmartPtr<AssetDatabaseManager> activeCatalog )
        {
            if( !sameCatalog( activeCatalog ) )
                return changed();
            if( !m_page.succeeded || !m_page.hasMore )
                return false;
            m_cursors.push_back( m_page.next );
            return read( activeCatalog );
        }
        bool previous( SmartPtr<AssetDatabaseManager> activeCatalog )
        {
            if( !sameCatalog( activeCatalog ) )
                return changed();
            if( m_cursors.empty() )
                return false;
            // Validate the displayed generation even when returning to page one.
            if( m_page.entries.empty() || !activeCatalog->isEntryCurrent( m_page.entries.front() ) )
                return changed();
            m_cursors.pop_back();
            return read( activeCatalog );
        }
        bool select( size_t index, SmartPtr<AssetDatabaseManager> activeCatalog, Entry &entry )
        {
            entry = {};
            if( !sameCatalog( activeCatalog ) )
                return changed();
            if( !m_page.succeeded || index >= m_page.entries.size() )
                return false;
            if( !activeCatalog->isEntryCurrent( m_page.entries[index] ) )
                return changed();
            entry = m_page.entries[index];
            return true;
        }
        const Page &page() const { return m_page; }
        size_t pageNumber() const { return m_cursors.size() + 1; }
        bool hasPrevious() const { return m_page.succeeded && !m_cursors.empty(); }
    private:
        bool sameCatalog( SmartPtr<AssetDatabaseManager> catalog ) const
        {
            auto bound = m_catalog.lock();
            return catalog && catalog == bound;
        }
        bool changed()
        {
            clear();
            m_page.error = "Catalog changed; refresh the first page.";
            return false;
        }
        bool read( SmartPtr<AssetDatabaseManager> catalog )
        {
            if( !catalog )
            {
                clear();
                m_page.error = "No active asset catalog. Open a project and refresh.";
                return false;
            }
            m_page = catalog->queryEntries( m_query, m_cursors.empty() ? nullptr : &m_cursors.back() );
            if( !m_page.succeeded )
                m_cursors.clear();
            return m_page.succeeded;
        }
        WeakPtr<AssetDatabaseManager> m_catalog;
        Query m_query;
        Page m_page;
        Array<AssetDatabaseManager::EntryCursor> m_cursors;
    };
}
#endif
