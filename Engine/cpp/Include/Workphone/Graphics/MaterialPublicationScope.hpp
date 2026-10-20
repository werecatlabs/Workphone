#ifndef WP_MaterialPublicationScope_hpp
#define WP_MaterialPublicationScope_hpp

#include <Workphone/WorkphoneTypes.hpp>
#include <cstddef>

namespace workphone
{
    class MaterialPassStateData;

    /** Owning-thread update boundary for immutable material snapshots.
     * Writes retain their usual short locks; readers keep the previous snapshot
     * until the outer scope commits. End update scopes before render capture.
     * Other threads and edits outside a scope retain immediate publication.
     * An enclosing state write/batch retains its final publication boundary.
     * Scopes belong to their constructing thread and must be nested normally.
     */
    class WPCore_API MaterialPublicationScope
    {
    public:
        MaterialPublicationScope();
        ~MaterialPublicationScope() noexcept;
        MaterialPublicationScope(const MaterialPublicationScope &) = delete;
        MaterialPublicationScope &operator=(const MaterialPublicationScope &) = delete;
        void commit();
        static bool isActive() noexcept;
        static size_t pendingEntryCount() noexcept;
        static size_t retainedStorageBytes() noexcept;
    private:
        friend class MaterialPassStateData;
        static bool defer(MaterialPassStateData &state) noexcept;
        static void forget(MaterialPassStateData &state) noexcept;
        bool m_active = true;
    };
}
#endif
