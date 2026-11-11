#include <Workphone/Core/Properties.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/IO/MemoryFile.hpp>
#include <Workphone/IO/PackageArchive.hpp>
#include <Workphone/IO/PackagePaths.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/Mesh/MeshSkeleton.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/MeshSerializer.hpp>
#include <Workphone/Mesh/MeshUtil.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>
#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <Workphone/Interface/Mesh/IIndexBuffer.hpp>
#include <Workphone/Interface/Mesh/IGraphicsBone.hpp>
#include <Workphone/Mesh/XMLSkeletonSerializer.hpp>
#include <Workphone/Scene/SceneWorld.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/System/ProjectLocalization.hpp>
#include <WPAssimp/IOStream.hpp>
#include <WPAssimp/IOSystem.hpp>
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/scene.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace workphone;
namespace fs = std::filesystem;

namespace
{
    void require( bool value, const char *message )
    {
        if( !value ) throw std::runtime_error( message );
    }

    String pathString( const fs::path &path ) { return path.generic_u8string().c_str(); }

    struct Fixture
    {
        fs::path root = fs::temp_directory_path() /
                        ( "WorkphonePakConsumers-" + std::string( StringUtil::getUUID().c_str() ) );
        fs::path project = root / "game";
        SmartPtr<core::ApplicationManager> app = make_ptr<core::ApplicationManager>();
        SmartPtr<FileSystem> files = make_ptr<FileSystem>();
        SmartPtr<FactoryManager> factories = make_ptr<FactoryManager>();

        Fixture()
        {
            fs::create_directories( project );
            core::IApplicationManager::setInstance( app );
            app->setProjectPath( pathString( project ) );
            app->setPlayerSettings( make_ptr<Properties>() );
            app->setFileSystem( files );
            files->load( nullptr );
        }

        ~Fixture()
        {
            releaseProjectLocalization();
            setPackageContentOnly( *files, false );
            setPackagePathRemapping( *files, false );
            files->unload( nullptr );
            app->setFileSystem( nullptr );
            app->setPlayerSettings( nullptr );
            app->setFactoryManager( nullptr );
            factories->unload( nullptr );
            core::IApplicationManager::setInstance( nullptr );
            std::error_code error;
            fs::remove_all( root, error );
        }

        void write( const fs::path &path, const char *text )
        {
            fs::create_directories( path.parent_path() );
            std::ofstream output( path, std::ios::binary );
            output << text;
            require( bool( output ), "Could not write test fixture" );
        }
    };

    void seekContract()
    {
        char bytes[] = "0123456789";
        auto memory = make_ptr<MemoryFile>( bytes, 10, "seek fixture", false );
        IOStream stream( memory );
        require( stream.Seek( 3, aiOrigin_SET ) == AI_SUCCESS && stream.Tell() == 3,
                 "Absolute Assimp seek failed" );
        require( stream.Seek( 2, aiOrigin_CUR ) == AI_SUCCESS && stream.Tell() == 5,
                 "Relative Assimp seek failed" );
        require( stream.Seek( size_t( -2 ), aiOrigin_CUR ) == AI_SUCCESS && stream.Tell() == 3,
                 "Negative relative Assimp seek failed" );
        require( stream.Seek( size_t( -1 ), aiOrigin_END ) == AI_SUCCESS && stream.Tell() == 9,
                 "End-relative Assimp seek failed" );
        require( stream.Seek( 1, aiOrigin_END ) == AI_FAILURE && stream.Tell() == 9,
                 "Assimp seek escaped end of archive entry" );
        require( stream.Seek( size_t( -11 ), aiOrigin_END ) == AI_FAILURE && stream.Tell() == 9,
                 "Assimp seek escaped start of archive entry" );
        require( stream.Read( bytes, 0, 10 ) == 0 &&
                     stream.Read( bytes, 2, std::numeric_limits<size_t>::max() ) == 0,
                 "Assimp zero or overflowing read was accepted" );
    }

    void archiveConsumersContract()
    {
        Fixture fixture;
        const auto source = fixture.root / "source";
        fixture.write( source / "Localization/en.json", R"({"packaged":"Read from package"})" );
        fixture.write( source / "Cache/rig.skeleton.xml",
                       "<skeleton><bones><bone id=\"0\" name=\"root\">"
                       "<position x=\"0\" y=\"0\" z=\"0\"/>"
                       "<rotation angle=\"0\"><axis x=\"0\" y=\"1\" z=\"0\"/>"
                       "</rotation></bone></bones></skeleton>" );
        fixture.write( source / "Assets/A/triangle.obj",
                       "mtllib shared.mtl\nv 0 0 0\nv 1 0 0\nv 0 1 0\nusemtl selected\nf 1 2 3\n" );
        fixture.write( source / "Assets/A/shared.mtl", "newmtl selected\nKd 1 0 0\n" );
        fixture.write( source / "Assets/B/shared.mtl", "newmtl selected\nKd 0 1 0\n" );
        fixture.write( source / "Cache/lines.txt", "first\nsecond\n" );
        fixture.factories->load( nullptr );
        fixture.app->setFactoryManager( fixture.factories );
        const Array<Vector3F> positions{ { 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 } };
        const Array<Vector3F> normals{ { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 } };
        const Array<Vector2F> uvs{ { 0, 0 }, { 1, 0 }, { 0, 1 } };
        auto cooked = dynamic_pointer_cast<Mesh>( MeshUtil::createMesh( positions, normals, uvs,
                                                                       Array<u32>{ 0, 1, 2 } ) );
        require( cooked != nullptr, "Could not create cooked mesh fixture" );
        cooked->updateAABB( true );
        MeshSerializer{}.exportMesh( cooked.get(), pathString( source / "Cache/triangle.fbmeshbin" ) );
        fixture.app->setFactoryManager( nullptr );

        auto scene = make_ptr<Properties>();
        auto actor = make_ptr<Properties>();
        actor->setName( "actors" );
        actor->setProperty( "uuid", "package-world-actor" );
        actor->setProperty( "label", "Archive world actor" );
        scene->addChild( actor );
        String manifestPath, error;
        require( scene::SceneWorld::cook( *scene, pathString( source / "Assets/World" ), {},
                                           manifestPath, error ), error.c_str() );
        const auto manifestName = fs::u8path( manifestPath.c_str() ).lexically_relative( source );
        Array<StringW> entries;
        for( const auto &entry : fs::recursive_directory_iterator( source ) )
            if( entry.is_regular_file() ) entries.emplace_back( entry.path().wstring().c_str() );
        const auto package = fixture.project / "Content.fbpak";
        require( writeGamePackageArchive( StringW( package.wstring().c_str() ), entries,
                                           StringW( source.wstring().c_str() ), {} ) == 0,
                 "Could not write consumer test package" );
        fs::remove_all( source );
        require( fixture.files->addFileArchive( pathString( package ), true, false,
                                                IFileSystem::ArchiveType::ObfuscatedZip ),
                 "Could not mount consumer test package" );
        setPackagePathRemapping( *fixture.files, true );
        require( !fs::exists( fixture.project / "Assets" ) && !fs::exists( fixture.project / "Cache" ) &&
                     !fs::exists( fixture.project / "Localization" ), "Fixture leaked loose content" );

        auto lines = fixture.files->open( "Cache/lines.txt", true, true, false );
        require( lines && lines->getLine( false ) == "first" && lines->tell() == 6 &&
                     lines->getLine( false ) == "second" && lines->tell() == 13,
                 "Archive line reads did not rewind their lookahead" );
        lines->skip( size_Num( -7 ) );
        require( lines->tell() == 6, "Archive negative skip failed" );
        lines->skip( size_Num( -7 ) );
        require( lines->tell() == 6, "Archive negative skip escaped entry start" );
        lines->skip( 8 );
        require( lines->tell() == 6, "Archive forward skip escaped entry end" );

        const auto currentMaterial = fixture.project / "Assets/A/shared.mtl";
        const auto legacyMaterial = fixture.root / "authoring/Assets/A/shared.mtl";
        fixture.write( currentMaterial, "stale current loose file" );
        fixture.write( legacyMaterial, "stale authoring loose file" );
        const String expectedMaterial = "newmtl selected\nKd 1 0 0\n";
        require( fixture.files->readAllText( pathString( currentMaterial ) ) == expectedMaterial &&
                     fixture.files->readAllText( pathString( legacyMaterial ) ) == expectedMaterial,
                 "Packaged text reads bypassed the archive for a loose or legacy path" );
        const auto materialBytes = fixture.files->readAllBytes( pathString( legacyMaterial ) );
        require( std::string( materialBytes.begin(), materialBytes.end() ) == expectedMaterial.c_str(),
                 "Packaged byte reads bypassed the archive for a legacy path" );
        fs::remove_all( fixture.project / "Assets" );

        require( localizeProjectString( "packaged", "missing" ) == "Read from package",
                 "Localization did not read the packaged dictionary" );
        fixture.app->setFactoryManager( fixture.factories );
        auto meshInput = fixture.files->open( "Cache/triangle.fbmeshbin", true, true, false );
        auto restored = make_ptr<Mesh>();
        MeshSerializer{}.importMesh( meshInput, restored.get() );
        auto submeshes = restored->getSubMeshes();
        require( submeshes.size() == 1 && submeshes[0]->getVertexBuffer() &&
                     submeshes[0]->getVertexBuffer()->getNumVertices() == 3 &&
                     submeshes[0]->getIndexBuffer() &&
                     submeshes[0]->getIndexBuffer()->getNumIndices() == 3,
                 "Archive cooked mesh lost vertices or triangles after chunk lookahead" );
        setPackageContentOnly( *fixture.files, true );
        const auto missingCurrent = fixture.project / "Assets/A/not-in-package.obj";
        const auto missingLegacy = fixture.root / "authoring/Assets/A/not-in-package.obj";
        fixture.write( missingCurrent, "unpacked current content" );
        fixture.write( missingLegacy, "unpacked authoring content" );
        require( !fixture.files->open( pathString( missingCurrent ), true, true, false ),
                 "Content-only open fell back to current loose source content" );
        require( !fixture.files->open( pathString( missingLegacy ), true, true, false ),
                 "Content-only open fell back to legacy loose source content" );
        const auto acceptedLooseSource = fixture.files->isExistingFile( pathString( missingCurrent ) );
        require( fixture.files->readAllBytes( pathString( missingLegacy ) ).empty(),
                 "Content-only byte read fell back to legacy loose source content" );
        fs::remove_all( fixture.project / "Assets" );
        auto skeleton = make_ptr<MeshSkeleton>();
        XMLSkeletonSerializer{}.importSkeleton( pathString( fixture.project / "Cache/rig.skeleton.xml" ),
                                                skeleton.get() );
        require( skeleton->getBone( "root" ) != nullptr, "Skeleton XML did not load from archive" );

        scene::WorldManifest manifest;
        const auto runtimeManifest = pathString( fixture.project / manifestName );
        require( scene::SceneWorld::readManifest( runtimeManifest, manifest, error ), error.c_str() );
        require( manifest.cells.size() == 1, "Cooked archive world lost its persistent cell" );
        const auto cell = scene::SceneWorld::readCell( runtimeManifest, manifest.cells.front(), error );
        require( cell != nullptr && cell->getChildrenByName( "actors" ).size() == 1,
                 "World cell did not load from archive" );

        Assimp::Importer importer;
        importer.SetIOHandler( new IOSystem() );
        const auto imported = importer.ReadFile( pathString( fixture.project / "Assets/A/triangle.obj" )
                                                    .c_str(), 0 );
        require( imported && imported->mNumMeshes == 1, importer.GetErrorString() );
        aiColor3D colour;
        const auto material = imported->mMaterials[imported->mMeshes[0]->mMaterialIndex];
        require( material->Get( AI_MATKEY_COLOR_DIFFUSE, colour ) == AI_SUCCESS &&
                     colour.r == 1 && colour.g == 0,
                 "Assimp loaded the wrong companion file with a duplicate basename" );
        require( !acceptedLooseSource,
                 "Content-only existence check accepted current loose source content" );
    }
}

int main()
{
    TypeManager types;
    types.load();
    TypeManager::setInstance( &types );
    int result = 0;
    try
    {
        seekContract();
        archiveConsumersContract();
        std::cout << "Package consumer contracts passed (archive line/mesh rewind, Assimp seeks/materials, world cells, skeleton, localization).\n";
    }
    catch( const std::exception &error )
    {
        std::cerr << "Package consumer contract failed: " << error.what() << '\n';
        result = 1;
    }
    types.unload();
    TypeManager::setInstance( nullptr );
    return result;
}
