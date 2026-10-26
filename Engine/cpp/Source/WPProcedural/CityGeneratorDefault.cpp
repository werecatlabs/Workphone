#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/CityGeneratorDefault.hpp>
#include <Workphone/Workphone.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <limits>

namespace workphone
{
    namespace procedural
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CityGeneratorDefault,
                                   CProceduralGenerator<ICityGenerator> );

        namespace
        {
            SmartPtr<Properties> xmlProperties( const String &element,
                                                const boost::property_tree::ptree &tree )
            {
                auto result = workphone::make_ptr<Properties>();
                result->setProperty( "element", element );
                if( auto attributes = tree.get_child_optional( "<xmlattr>" ) )
                {
                    for( const auto &attribute : *attributes )
                    {
                        result->setProperty( String( attribute.first.c_str() ),
                                             String( attribute.second.data().c_str() ) );
                    }
                }
                if( !tree.data().empty() )
                {
                    result->setProperty( "value", String( tree.data().c_str() ) );
                }
                for( const auto &child : tree )
                {
                    if( child.first != "<xmlattr>" && child.first != "<xmlcomment>" )
                    {
                        result->addChild( xmlProperties( String( child.first.c_str() ), child.second ) );
                    }
                }
                return result;
            }
        }  // namespace

        CityGeneratorDefault::CityGeneratorDefault() = default;

        CityGeneratorDefault::~CityGeneratorDefault()
        {
            unload( nullptr );
        }

        void CityGeneratorDefault::requireIdle() const
        {
            if( m_running )
            {
                throw std::logic_error( "CityGeneratorDefault: generation is in progress" );
            }
        }

        Array<SmartPtr<IProceduralCity>> CityGeneratorDefault::collectCities() const
        {
            auto cities = m_cities;
            auto scenes = m_scenes;
            if( m_proceduralWorld )
            {
                for( auto scene : m_proceduralWorld->getScenes() )
                {
                    if( scene && std::find( scenes.begin(), scenes.end(), scene ) == scenes.end() )
                    {
                        scenes.push_back( scene );
                    }
                }
            }
            for( auto scene : scenes )
            {
                if( !scene )
                {
                    continue;
                }
                for( auto city : scene->getCities() )
                {
                    if( city && std::find( cities.begin(), cities.end(), city ) == cities.end() )
                    {
                        cities.push_back( city );
                    }
                }
            }
            return cities;
        }

        void CityGeneratorDefault::loadFromFile( const String &filePath )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            requireIdle();
            auto properties = osmDataToProperties( filePath );
            const auto cities = collectCities();
            // Parse successfully before changing the configured path or calling clients.
            for( auto city : cities )
            {
                city->load( properties );
            }
            for( auto city : cities )
            {
                if( m_roadGenerator )
                {
                    m_roadGenerator->setCity( city );
                    m_roadGenerator->load( properties );
                }
                if( m_blockGenerator )
                {
                    m_blockGenerator->setCity( city );
                    m_blockGenerator->load( properties );
                }
            }
            if( m_terrainGenerator )
            {
                m_terrainGenerator->load( properties );
            }
            if( m_meshGenerator )
            {
                m_meshGenerator->load( properties );
            }
            m_filePath = filePath;
            setLoadingState( LoadingState::Loaded );
        }

        void CityGeneratorDefault::load( SmartPtr<ISharedObject> data )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            requireIdle();
            if( !m_filePath.empty() )
            {
                loadFromFile( m_filePath );
                return;
            }
            const auto cities = collectCities();
            for( auto city : cities )
            {
                city->load( data );
            }
            if( m_terrainGenerator )
            {
                m_terrainGenerator->load( data );
            }
            if( m_meshGenerator )
            {
                m_meshGenerator->load( data );
            }
            for( auto city : cities )
            {
                if( m_roadGenerator )
                {
                    m_roadGenerator->setCity( city );
                    m_roadGenerator->load( data );
                }
                if( m_blockGenerator )
                {
                    m_blockGenerator->setCity( city );
                    m_blockGenerator->load( data );
                }
            }
            setLoadingState( LoadingState::Loaded );
        }

        void CityGeneratorDefault::unload( SmartPtr<ISharedObject> data )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            // Detach all owned inventories before releasing any callback owner.
            auto steps = std::move( m_steps );
            auto meshes = std::move( m_generatedMeshes );
            auto scenes = std::move( m_scenes );
            auto cities = std::move( m_cities );
            auto active = std::exchange( m_active, nullptr );
            auto terrain = std::exchange( m_terrainGenerator, nullptr );
            auto road = std::exchange( m_roadGenerator, nullptr );
            auto block = std::exchange( m_blockGenerator, nullptr );
            auto mesh = std::exchange( m_meshGenerator, nullptr );
            auto world = std::exchange( m_proceduralWorld, nullptr );
            auto input = std::exchange( m_input, nullptr );
            auto output = std::exchange( m_output, nullptr );
            auto parent = std::exchange( m_parent, nullptr );
            m_running = false;
            m_finished = true;
            m_nextStep = 0;
            ++m_generation;
            setLoadingState( LoadingState::Unloaded );
        }

        void CityGeneratorDefault::generate()
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            requireIdle();
            // Prepare the complete run before replacing the previous result or
            // invoking a producer. A denied plan leaves the last run intact.
            auto steps = makeSourceBudgetSnapshot<Step>();
            auto scenes = m_scenes;
            auto cities = m_cities;
            auto appendUnique = []( auto &items, const auto &item ) {
                if( item && std::find( items.begin(), items.end(), item ) == items.end() )
                    items.push_back( item );
            };
            if( m_proceduralWorld )
                for( auto scene : m_proceduralWorld->getScenes() ) appendUnique( scenes, scene );
            size_t outputSlots = 0;
            for( auto scene : scenes )
            {
                for( auto city : scene->getCities() ) appendUnique( cities, city );
                if( auto terrain = m_terrainGenerator )
                {
                    steps.push_back( { Step::Kind::Terrain, terrain, scene, nullptr, nullptr } );
                    if( auto mesh = m_meshGenerator )
                    {
                        steps.push_back( { Step::Kind::TerrainMesh, mesh, nullptr, nullptr, terrain } );
                        ++outputSlots;
                    }
                }
            }
            for( auto city : cities )
            {
                if( auto road = m_roadGenerator )
                    steps.push_back( { Step::Kind::Road, road, nullptr, city, nullptr } );
                if( auto block = m_blockGenerator )
                    steps.push_back( { Step::Kind::Block, block, nullptr, city, nullptr } );
                if( auto mesh = m_meshGenerator )
                {
                    steps.push_back( { Step::Kind::CityMesh, mesh, nullptr, city, nullptr } );
                    if( outputSlots > std::numeric_limits<size_t>::max() - 2 )
                        throw std::length_error( "City mesh inventory overflow" );
                    outputSlots += 2;
                }
            }
            auto meshes = makeSourceBudgetSnapshot<SmartPtr<IMesh>>();
            meshes.reserve( outputSlots );
            m_steps.swap( steps );
            m_generatedMeshes.swap( meshes );
            m_nextStep = 0;
            m_finished = false;
            m_running = true;
            ++m_generation;
            advance();
        }

        void CityGeneratorDefault::advance()
        {
            if( m_advancing )
            {
                return;
            }
            m_advancing = true;

            struct Guard
            {
                bool &flag;

                ~Guard()
                {
                    flag = false;
                }
            } guard{ m_advancing };

            auto generation = m_generation;
            try
            {
                while( m_running )
                {
                    generation = m_generation;
                    auto active = m_active;
                    if( active && !active->isFinished() )
                    {
                        return;
                    }
                    if( generation != m_generation ) continue;
                    m_active = nullptr;
                    if( m_nextStep == m_steps.size() )
                    {
                        m_running = false;
                        m_finished = true;
                        auto retired = std::move( m_steps );
                        return;
                    }
                    // Copy the action so reentrant queries cannot invalidate its storage.
                    auto step = m_steps[m_nextStep++];
                    m_active = step.generator;
                    // Typed stack values retain the owners through reentrant callbacks;
                    // there are no erased heap captures or allocating action copies.
                    switch( step.kind )
                    {
                    case Step::Kind::Terrain:
                    {
                        auto terrain = static_pointer_cast<ITerrainGenerator>( step.generator );
                        terrain->setProceduralScene( step.scene );
                        terrain->generate();
                        break;
                    }
                    case Step::Kind::TerrainMesh:
                    {
                        auto mesh = static_pointer_cast<IMeshGenerator>( step.generator );
                        if( auto data = step.terrain->getTerrain() )
                        {
                            auto result = mesh->generateFromTerrain( data );
                            if( generation != m_generation || !m_running ) break;
                            if( !result ) throw std::runtime_error( "CityGeneratorDefault: terrain mesh generation failed" );
                            m_generatedMeshes.push_back( result );
                        }
                        break;
                    }
                    case Step::Kind::Road:
                    {
                        auto road = static_pointer_cast<IRoadGenerator>( step.generator );
                        road->setCity( step.city );
                        road->generate();
                        break;
                    }
                    case Step::Kind::Block:
                    {
                        auto block = static_pointer_cast<IBlockGenerator>( step.generator );
                        block->setCity( step.city );
                        block->generate();
                        break;
                    }
                    case Step::Kind::CityMesh:
                    {
                        auto mesh = static_pointer_cast<IMeshGenerator>( step.generator );
                        if( auto network = step.city->getRoadNetwork() )
                        {
                            if( !network->getRoads().empty() )
                            {
                                auto result = mesh->generate( network );
                                if( generation != m_generation || !m_running ) break;
                                if( !result ) throw std::runtime_error( "CityGeneratorDefault: road mesh generation failed" );
                                m_generatedMeshes.push_back( result );
                            }
                        }
                        auto objects = makeSourceBudgetSnapshot<SmartPtr<IProceduralObject>>();
                        for( auto block : step.city->getBlocks() )
                            if( block ) objects.push_back( block );
                        if( !objects.empty() )
                        {
                            auto result = mesh->generateFromObjects( objects );
                            if( generation != m_generation || !m_running ) break;
                            if( !result ) throw std::runtime_error( "CityGeneratorDefault: block mesh generation failed" );
                            m_generatedMeshes.push_back( result );
                        }
                        break;
                    }
                    }
                }
            }
            catch( ... )
            {
                if( generation == m_generation )
                {
                    auto retired = std::move( m_steps );
                    auto active = std::exchange( m_active, nullptr );
                    m_running = false;
                }
                throw;
            }
        }

        bool CityGeneratorDefault::isFinished() const
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            if( m_running )
            {
                const_cast<CityGeneratorDefault *>( this )->advance();
            }
            return m_finished;
        }

        SmartPtr<Properties> CityGeneratorDefault::osmDataToProperties( const String &filePath ) const
        {
            if( filePath.empty() )
            {
                throw std::invalid_argument( "CityGeneratorDefault: empty OSM path" );
            }
            auto app = core::IApplicationManager::instancePtr();
            if( !app || !app->getFileSystem() )
            {
                throw std::runtime_error( "CityGeneratorDefault: file system unavailable" );
            }
            auto text = app->getFileSystem()->readAllText( filePath );
            if( text.empty() )
            {
                throw std::runtime_error( "CityGeneratorDefault: empty or missing OSM file" );
            }
            std::istringstream stream( text.c_str() );
            boost::property_tree::ptree document;
            boost::property_tree::read_xml( stream, document,
                                            boost::property_tree::xml_parser::trim_whitespace );
            auto osm = document.get_child_optional( "osm" );
            if( !osm )
            {
                throw std::invalid_argument( "CityGeneratorDefault: expected an osm XML root" );
            }
            return xmlProperties( "osm", *osm );
        }

        const Array<SmartPtr<IProceduralCity>> CityGeneratorDefault::getCities() const
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            return m_cities;
        }

        void CityGeneratorDefault::addCity( SmartPtr<IProceduralCity> city )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            requireIdle();
            if( city && std::find( m_cities.begin(), m_cities.end(), city ) == m_cities.end() )
            {
                m_cities.push_back( city );
            }
        }

        void CityGeneratorDefault::removeCity( SmartPtr<IProceduralCity> city )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            requireIdle();
            auto it = std::find( m_cities.begin(), m_cities.end(), city );
            if( it != m_cities.end() )
            {
                m_cities.erase( it );
            }
        }

        String CityGeneratorDefault::getFilePath() const
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            return m_filePath;
        }

        void CityGeneratorDefault::setFilePath( const String &path )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            requireIdle();
            m_filePath = path;
        }

        Array<SmartPtr<IMesh>> CityGeneratorDefault::getGeneratedMeshes() const
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            return m_generatedMeshes;
        }

        SmartPtr<ITerrainGenerator> CityGeneratorDefault::getTerrainGenerator() const
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            return m_terrainGenerator;
        }

        void CityGeneratorDefault::setTerrainGenerator( SmartPtr<ITerrainGenerator> value )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            requireIdle();
            m_terrainGenerator = value;
        }

        SmartPtr<IRoadGenerator> CityGeneratorDefault::getRoadGenerator() const
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            return m_roadGenerator;
        }

        void CityGeneratorDefault::setRoadGenerator( SmartPtr<IRoadGenerator> value )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            requireIdle();
            m_roadGenerator = value;
        }

        SmartPtr<IBlockGenerator> CityGeneratorDefault::getBlockGenerator() const
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            return m_blockGenerator;
        }

        void CityGeneratorDefault::setBlockGenerator( SmartPtr<IBlockGenerator> value )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            requireIdle();
            m_blockGenerator = value;
        }

        SmartPtr<IMeshGenerator> CityGeneratorDefault::getMeshGenerator() const
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            return m_meshGenerator;
        }

        void CityGeneratorDefault::setMeshGenerator( SmartPtr<IMeshGenerator> value )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            requireIdle();
            m_meshGenerator = value;
        }

        SmartPtr<IProceduralWorld> CityGeneratorDefault::getProceduralWorld() const
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            return m_proceduralWorld;
        }

        void CityGeneratorDefault::setProceduralWorld( SmartPtr<IProceduralWorld> value )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            requireIdle();
            m_proceduralWorld = value;
        }
    }  // namespace procedural
}  // namespace workphone
