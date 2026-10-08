#ifndef WP_ASSET_CATALOG_PATH_CONTRACTS_HPP
#define WP_ASSET_CATALOG_PATH_CONTRACTS_HPP

#include <Workphone/Database/AssetCatalogPath.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#    include <windows.h>
#endif

inline void runAssetCatalogPathContracts( const std::filesystem::path &folder )
{
    using namespace workphone;
    const auto requirePath = []( bool value, const char *message ) {
        if( !value )
        {
            throw std::runtime_error( message );
        }
    };
    const auto utf8 = []( const std::filesystem::path &path ) {
        return String( path.generic_u8string().c_str() );
    };
    const auto project = folder / "path-project";
    const auto relocated = folder / "path-relocated";
    const auto outside = folder / "path-project-sibling";
    std::filesystem::create_directories( project / "textures" );
    std::filesystem::create_directories( relocated / "textures" );
    std::filesystem::create_directories( outside );
    String root;
    String relocatedRoot;
    String error;
    requirePath( canonicalAssetCatalogRoot( utf8( project / "." ), root, error ),
                 "Project root must resolve to an absolute UTF-8 path" );
    requirePath( std::filesystem::u8path( root.begin(), root.end() ).is_absolute(),
                 "Captured root must be absolute" );
    requirePath( canonicalAssetCatalogRoot( utf8( relocated ), relocatedRoot, error ),
                 "Relocated project root must resolve" );

    AssetCatalogPath first;
    AssetCatalogPath equivalent;
    const String expected = u8"textures/caf\u00e9's;\u7eb9\u7406.tex";
    requirePath( canonicalAssetCatalogPath(
                     root, u8"textures\\.\\missing\\..\\caf\u00e9's;\u7eb9\u7406.tex", first, error ),
                 "Missing resources with Unicode, quotes and separators must normalize" );
    requirePath( first.path == expected, "Canonical relative path must preserve Unicode spelling" );
    requirePath( canonicalAssetCatalogPath(
                     root, utf8( project / std::filesystem::u8path( expected.begin(), expected.end() ) ),
                     equivalent, error ) &&
                     equivalent.path == first.path && equivalent.key == first.key,
                 "Absolute and relative paths must produce the same identity" );
#ifdef _WIN32
    const auto nativeProject = std::filesystem::u8path( root.begin(), root.end() );
    const auto shortCapacity = GetShortPathNameW( nativeProject.c_str(), nullptr, 0 );
    std::wstring shortName( shortCapacity, L'\0' );
    const auto shortLength =
        shortCapacity ? GetShortPathNameW( nativeProject.c_str(), shortName.data(), shortCapacity ) : 0;
    if( shortLength && shortLength < shortCapacity && shortName.find( L'~' ) != std::wstring::npos )
    {
        shortName.resize( shortLength );
        const std::filesystem::path shortProject( shortName );
        requirePath(
            canonicalAssetCatalogPath(
                root, utf8( shortProject / std::filesystem::u8path( expected.begin(), expected.end() ) ),
                equivalent, error ) &&
                equivalent.path == first.path && equivalent.key == first.key,
            "Windows short root aliases must resolve the same missing resource identity" );
        requirePath( !canonicalAssetCatalogPath(
                         root, utf8( shortProject / ".." / shortProject.filename() / "return.tex" ),
                         equivalent, error ),
                     "Resolving a short root alias must not hide escape-and-return traversal" );
        std::cout << "PASS: Windows short-root aliases and retained traversal rejection\n";
    }
    else
    {
        std::cout << "UNAVAILABLE: Windows short-name alias fixture on this volume\n";
    }
#endif
    requirePath( canonicalAssetCatalogPath( relocatedRoot, expected, equivalent, error ) &&
                     equivalent.path == first.path && equivalent.key == first.key,
                 "Project relocation must preserve relative asset identity" );
    requirePath( canonicalAssetCatalogPath( root, "Textures/Mixed.TEX", first, error ) &&
                     canonicalAssetCatalogPath( root, "textures/mixed.tex", equivalent, error ),
                 "ASCII mixed-case paths must normalize" );
#ifdef _WIN32
    requirePath( first.key == equivalent.key, "Windows catalog keys must fold ASCII case" );
#else
    requirePath( first.key != equivalent.key, "Non-Windows catalog keys must preserve case" );
#endif
    requirePath( canonicalAssetCatalogPath( root, u8"textures/caf\u00e9.tex", first, error ) &&
                     canonicalAssetCatalogPath( root, u8"textures/CAF\u00c9.TEX", equivalent, error ) &&
                     first.key != equivalent.key,
                 "Non-ASCII case folding must not be implied by ASCII-only identity policy" );

    const auto rejects = [&]( const String &input ) {
        first.path = "stale";
        first.key = "stale";
        requirePath( !canonicalAssetCatalogPath( root, input, first, error ) && first.path.empty() &&
                         first.key.empty() && !error.empty(),
                     "Rejected path must fail with a reason and clear stale output" );
    };
    rejects( "" );
    rejects( "." );
    rejects( "textures/.." );
    rejects( root );
    rejects( "../escape.tex" );
    rejects( "textures/../../escape.tex" );
    rejects( "../path-project/return.tex" );
    rejects( utf8( project / ".." / "path-project" / "return.tex" ) );
    rejects( utf8( outside / "escape.tex" ) );
    rejects( "C:drive-relative.tex" );
    rejects( "textures/file.tex:stream" );
    rejects( "\\\\?\\C:\\textures\\file.tex" );
    rejects( "\\\\.\\C:\\textures\\file.tex" );
    rejects( String( 1025, 'x' ) );
    rejects( String( "textures/a\0b.tex", 16 ) );
    rejects( String( "textures/\xc0\xaf.tex" ) );
#ifdef _WIN32
    rejects( "\\root-relative.tex" );
    rejects( "/root-relative.tex" );
    rejects( "textures/NUL.tex" );
    rejects( "textures/COM1.tex" );
    rejects( u8"textures/COM\u00b9.tex" );
    rejects( "textures/trailing.tex." );
    rejects( "textures/trailing.tex " );
    rejects( "textures/wildcard*.tex" );
#endif
    requirePath( !canonicalAssetCatalogPath( "relative-root", "file.tex", first, error ),
                 "File identity must reject uncaptured relative project roots" );
    requirePath( !canonicalAssetCatalogRoot( "", relocatedRoot, error ) && relocatedRoot.empty(),
                 "Invalid roots must clear stale output" );
    {
        std::ofstream file( project / "root-is-file" );
        file << "fixture";
    }
    requirePath( !canonicalAssetCatalogRoot( utf8( project / "root-is-file" ), relocatedRoot, error ),
                 "An existing file cannot serve as project root" );

    std::error_code linkError;
    std::filesystem::create_directory_symlink( outside, project / "outside-link", linkError );
    if( linkError )
    {
        std::cout << "UNAVAILABLE: catalog symbolic-link fixture: " << linkError.message() << '\n';
    }
    else
    {
        rejects( "outside-link/missing.tex" );
        rejects( "outside-link/../path-project/return.tex" );
        std::filesystem::create_directory_symlink( project / "textures", project / "inside-link",
                                                   linkError );
        requirePath( !linkError, "Once supported, in-root symlink fixture must be created" );
        requirePath( canonicalAssetCatalogPath( root, "inside-link/missing.tex", first, error ) &&
                         first.path == "textures/missing.tex",
                     "In-root symlinks must share target identity" );
        std::filesystem::create_directory_symlink( outside / "missing", project / "dangling-link",
                                                   linkError );
        requirePath( !linkError, "Once supported, dangling symlink fixture must be created" );
        rejects( "dangling-link/missing.tex" );
        std::cout << "PASS: catalog symlink containment and target identity\n";
    }
    std::cout << "PASS: catalog canonical paths, relocation, platform case policy and malformed paths\n";
}

#endif
