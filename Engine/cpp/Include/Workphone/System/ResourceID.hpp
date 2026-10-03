#ifndef WPResourceID_h__
#define WPResourceID_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneConfig.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone::resource
{
    /** Lowercase, one-to-eight character resource type identifier. */
    class WPCore_API ResourceTypeID final
    {
    public:
        ResourceTypeID() = default;
        explicit ResourceTypeID( const String &value );

        static bool isValidString( const String &value );
        static ResourceTypeID fromPath( const String &path );

        bool isValid() const;
        const String &str() const;
        u64 value() const;

        bool operator==( const ResourceTypeID &other ) const;
        bool operator!=( const ResourceTypeID &other ) const;
        bool operator<( const ResourceTypeID &other ) const;

    private:
        String m_value;
    };

    /**
     * Stable path-based resource identifier.
     *
     * IDs use the form data://folder/file.type and may identify a sub-resource
     * with data://folder/parent.type:child.type. Absolute paths and traversal
     * segments are rejected at construction time.
     */
    class WPCore_API ResourceID final
    {
    public:
        ResourceID() = default;
        explicit ResourceID( const String &value );

        static bool isValidString( const String &value, String *error = nullptr );

        bool set( const String &value, String *error = nullptr );
        void clear();
        bool isValid() const;

        const String &str() const;
        const ResourceTypeID &type() const;
        u64 pathHash() const;

        bool isSubResource() const;
        String subResourceName() const;
        ResourceID parent() const;

        /** Relative source descriptor path (the parent path for sub-resources). */
        String sourceRelativePath() const;

        /** Relative compiled path, with sub-resources flattened to parent_child.type. */
        String compiledRelativePath() const;

        bool operator==( const ResourceID &other ) const;
        bool operator!=( const ResourceID &other ) const;
        bool operator<( const ResourceID &other ) const;

    private:
        String m_value;
        ResourceTypeID m_type;
        u64 m_pathHash = 0;
    };

    /** Stable FNV-1a hash used by resource IDs and incremental build records. */
    WPCore_API u64 hashBytes( const void *data, size_t size, u64 seed = 14695981039346656037ull );
    WPCore_API u64 hashString( const String &value, u64 seed = 14695981039346656037ull );
    WPCore_API u64 combineHash( u64 current, u64 value );
}  // namespace workphone::resource

#endif  // WPResourceID_h__
