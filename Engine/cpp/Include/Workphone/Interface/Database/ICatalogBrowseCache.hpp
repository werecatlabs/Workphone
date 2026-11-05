#pragma once

namespace workphone
{
    // Optional backend tuning for repeated scans of the catalog browse index.
    class ICatalogBrowseCache
    {
    public:
        virtual ~ICatalogBrowseCache() = default;
        virtual void prepareCatalogBrowseCache() noexcept = 0;
    };
}
