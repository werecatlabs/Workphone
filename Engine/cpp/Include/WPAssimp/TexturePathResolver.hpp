#ifndef WP_ASSIMP_TEXTURE_PATH_RESOLVER_HPP
#define WP_ASSIMP_TEXTURE_PATH_RESOLVER_HPP

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace workphone::detail
{
    // One resolver per import: build each recursive filename index only when needed.
    class TexturePathResolver
    {
    public:
        TexturePathResolver( const std::filesystem::path &localFolder,
                             const std::filesystem::path &projectFolder ) :
            m_localFolder( localFolder ), m_projectFolder( projectFolder )
        {
        }

        std::filesystem::path resolve( std::string reference )
        {
            if( reference.empty() || reference.front() == '*' )
            {
                return {};
            }

            std::replace( reference.begin(), reference.end(), '\\', '/' );
            const auto path = std::filesystem::path( reference ).lexically_normal();
            if( path.is_absolute() )
            {
                if( isFile( path ) )
                {
                    return path;
                }
            }
            else
            {
                // Model-relative references take priority over project-relative ones.
                for( const auto &folder : { m_localFolder, m_projectFolder } )
                {
                    const auto candidate = ( folder / path ).lexically_normal();
                    if( isFile( candidate ) )
                    {
                        return candidate;
                    }
                }
            }

            // Exporters often retain an obsolete absolute directory.
            const auto besideModel = m_localFolder / path.filename();
            if( isFile( besideModel ) )
            {
                return besideModel;
            }

            buildIndex( m_localFolder, m_localIndex, m_localIndexed );
            const auto local = findBestMatch( path, m_localIndex );
            if( !local.empty() )
            {
                return local;
            }

            buildIndex( m_projectFolder, m_projectIndex, m_projectIndexed );
            return findBestMatch( path, m_projectIndex );
        }

    private:
        using FileIndex = std::map<std::string, std::vector<std::filesystem::path>>;

        static bool isFile( const std::filesystem::path &path )
        {
            std::error_code error;
            return std::filesystem::is_regular_file( path, error );
        }

        static std::string lower( std::string value )
        {
            std::transform( value.begin(), value.end(), value.begin(), []( unsigned char c ) {
                return static_cast<char>( std::tolower( c ) );
            } );
            return value;
        }

        static void buildIndex( const std::filesystem::path &folder, FileIndex &index, bool &indexed )
        {
            if( indexed )
            {
                return;
            }
            indexed = true;
            if( folder.empty() )
            {
                return;
            }

            std::error_code error;
            auto iterator = std::filesystem::recursive_directory_iterator(
                folder, std::filesystem::directory_options::skip_permission_denied, error );
            const auto end = std::filesystem::recursive_directory_iterator();
            while( !error && iterator != end )
            {
                if( iterator->is_regular_file( error ) )
                {
                    const auto path = iterator->path();
                    index[lower( path.filename().string() )].push_back( path );
                }
                error.clear();
                iterator.increment( error );
            }
        }

        static std::filesystem::path findBestMatch( const std::filesystem::path &reference,
                                                     const FileIndex &index )
        {
            const auto found = index.find( lower( reference.filename().string() ) );
            if( found == index.end() )
            {
                return {};
            }

            size_t bestScore = 0;
            bool ambiguous = false;
            std::filesystem::path best;
            for( const auto &candidate : found->second )
            {
                // Prefer matching trailing directories when filenames are duplicated.
                auto a = reference.end();
                auto b = candidate.end();
                size_t score = 0;
                while( a != reference.begin() && b != candidate.begin() )
                {
                    if( lower( ( --a )->string() ) != lower( ( --b )->string() ) )
                    {
                        break;
                    }
                    ++score;
                }
                if( score > bestScore )
                {
                    bestScore = score;
                    best = candidate;
                    ambiguous = false;
                }
                else if( score == bestScore )
                {
                    ambiguous = true;
                }
            }
            return ambiguous ? std::filesystem::path() : best;
        }

        std::filesystem::path m_localFolder;
        std::filesystem::path m_projectFolder;
        FileIndex m_localIndex;
        FileIndex m_projectIndex;
        bool m_localIndexed = false;
        bool m_projectIndexed = false;
    };
}

#endif
