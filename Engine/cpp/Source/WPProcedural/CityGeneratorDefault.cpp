#include "WPProcedural/WPProceduralPCH.hpp"
#include "WPProcedural/CityGeneratorDefault.hpp"
#include "WPProcedural/CRoadGenerator.hpp"
#include "WPProcedural/CProceduralCity.hpp"
#include "WPProcedural/CRoadNetwork.hpp"
#include "WPProcedural/MeshGeneratorDefault.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace procedural
    {
        WP_CLASS_REGISTER_DERIVED( workphone, CityGeneratorDefault,
                                   CProceduralGenerator<ICityGenerator> );

        CityGeneratorDefault::CityGeneratorDefault()
        {
        }

        CityGeneratorDefault::~CityGeneratorDefault()
        {
            unload( nullptr );
        }

        void CityGeneratorDefault::loadFromFile( const String &filePath )
        {
            //try
            //{
            //    auto applicationManager = core::IApplicationManager::instance();
            //    WP_ASSERT( applicationManager );

            //    auto fileSystem = applicationManager->getFileSystem();
            //    WP_ASSERT( fileSystem );

            //    auto text = fileSystem->readAllText( filePath );
            //    if( !StringUtil::isNullOrEmpty( text ) )
            //    {
            //        text =
            //            StringUtil::replaceAll( text, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>", "" );

            //        auto pData = fb::make_ptr<Data<data::osm>>();
            //        auto data = pData->getDataAsType<data::osm>();
            //        DataUtil::parseXML( text, data );

            //        auto jsonData = DataUtil::toString( data );
            //        auto fileName = Path::getFileNameWithoutExtension( filePath );
            //        fileSystem->writeAllText( fileName + ".osmjson", jsonData );

            //        for( auto city : m_cities )
            //        {
            //            city->load( pData );
            //        }

            //        auto roadGenerator = getRoadGenerator();
            //        if( roadGenerator )
            //        {
            //            for( auto city : m_cities )
            //            {
            //                roadGenerator->setCity( city );
            //                roadGenerator->load( pData );
            //            }
            //        }

            //        auto blockGenerator = getBlockGenerator();
            //        if( blockGenerator )
            //        {
            //            for( auto city : m_cities )
            //            {
            //                blockGenerator->setCity( city );
            //                // blockGenerator->load(data);
            //            }
            //        }
            //    }
            //}
            //catch( std::exception &e )
            //{
            //    WP_LOG_EXCEPTION( e );
            //}
        }

        void CityGeneratorDefault::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                if( m_terrainGenerator )
                {
                    m_terrainGenerator->unload( nullptr );
                    m_terrainGenerator = nullptr;
                }

                if( m_proceduralWorld )
                {
                    m_proceduralWorld->unload( nullptr );
                    m_proceduralWorld = nullptr;
                }

                if( m_blockGenerator )
                {
                    m_blockGenerator->unload( nullptr );
                    m_blockGenerator = nullptr;
                }

                if( m_meshGenerator )
                {
                    m_meshGenerator->unload( nullptr );
                    m_meshGenerator = nullptr;
                }

                if( m_roadGenerator )
                {
                    m_roadGenerator->unload( nullptr );
                    m_roadGenerator = nullptr;
                }

                for( auto scene : m_scenes )
                {
                    scene->unload( nullptr );
                }

                m_scenes.clear();

                for( auto city : m_cities )
                {
                    city->unload( nullptr );
                }

                m_cities.clear();

                CProceduralGenerator<ICityGenerator>::unload( data );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CityGeneratorDefault::generate()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto fileSystem = applicationManager->getFileSystem();

            // auto terrainGenerator = getTerrainGenerator();
            // if (terrainGenerator)
            //{
            //	auto scenes = m_proceduralWorld->getScenes();
            //	for (auto scene : scenes)
            //	{
            //		terrainGenerator->setProceduralScene(scene);
            //		terrainGenerator->generate();
            //		terrainGenerator->setProceduralScene(nullptr);
            //	}
            // }

            // SmartPtr<IProceduralCity> city = new City;
            // setCity(city);

            // SmartPtr<IRoadNetwork> roadNetwork = new CRoadNetwork;
            // city->setRoadNetwork(roadNetwork);

            // Array<SmartPtr<IRoadNode>> nodes;

            // for ( u32 i = 0; i < 10; ++i )
            //{
            //	SmartPtr<IRoadNode> node = new RoadNode;
            //	nodes.push_back(node);

            //	Vector3F position = Vector3F::UNIT_Z * 10 * i;
            //	node->setPosition(position);
            //}

            // roadNetwork->setNodes(nodes);

            if( m_roadGenerator )
            {
                for( auto city : m_cities )
                {
                    m_roadGenerator->setCity( city );
                    m_roadGenerator->generate();
                }
            }

            auto blockGenerator = getBlockGenerator();
            if( blockGenerator )
            {
                blockGenerator->generate();
            }
        }

        bool CityGeneratorDefault::isFinished() const
        {
            return true;
        }

        SmartPtr<ITerrainGenerator> CityGeneratorDefault::getTerrainGenerator() const
        {
            return m_terrainGenerator;
        }

        void CityGeneratorDefault::setTerrainGenerator( SmartPtr<ITerrainGenerator> terrainGenerator )
        {
            m_terrainGenerator = terrainGenerator;

            if( m_terrainGenerator )
            {
                auto parentGenerator = getSharedFromThis<IProceduralGenerator>();
                m_terrainGenerator->setParent( parentGenerator );
            }
        }

        SmartPtr<IBlockGenerator> CityGeneratorDefault::getBlockGenerator() const
        {
            return m_blockGenerator;
        }

        void CityGeneratorDefault::setBlockGenerator( SmartPtr<IBlockGenerator> blockGenerator )
        {
            m_blockGenerator = blockGenerator;
        }

        SmartPtr<IProceduralWorld> CityGeneratorDefault::getProceduralWorld() const
        {
            return m_proceduralWorld;
        }

        void CityGeneratorDefault::setProceduralWorld( SmartPtr<IProceduralWorld> proceduralWorld )
        {
            m_proceduralWorld = proceduralWorld;
        }

        // void CityGeneratorDefault::addScene(SmartPtr<IProceduralScene> scene)
        //{
        //	m_scenes.push_back(scene);
        // }

        // void CityGeneratorDefault::removeScene(SmartPtr<IProceduralScene> scene)
        //{
        //	auto it = std::find(m_scenes.begin(), m_scenes.end(), scene);
        //	if (it != m_scenes.end())
        //	{
        //		m_scenes.erase(it);
        //	}
        // }

        // Array<SmartPtr<IProceduralScene>> CityGeneratorDefault::getScenes() const
        //{
        //	return m_scenes;
        // }

        // void CityGeneratorDefault::setScenes(Array<SmartPtr<IProceduralScene>> scenes)
        //{
        //	m_scenes = scenes;
        // }

        void CityGeneratorDefault::setRoadGenerator( SmartPtr<IRoadGenerator> roadGenerator )
        {
            // if (m_roadGenerator)
            //{
            //	m_roadGenerator->setCityGenerator(nullptr);
            // }

            m_roadGenerator = roadGenerator;

            // if (m_roadGenerator)
            //{
            //	SmartPtr<ISharedObject> pThis = shared_from_this();
            //	SmartPtr<ICityGenerator> pThisCityGenerator =
            // fb::static_pointer_cast<ICityGenerator>(pThis);
            //	m_roadGenerator->setCityGenerator(pThisCityGenerator);
            // }

            if( m_roadGenerator )
            {
                auto parentGenerator = getSharedFromThis<IProceduralGenerator>();
                m_roadGenerator->setParent( parentGenerator );
            }
        }

        SmartPtr<IRoadGenerator> CityGeneratorDefault::getRoadGenerator() const
        {
            return m_roadGenerator;
        }

        const Array<SmartPtr<IProceduralCity>> CityGeneratorDefault::getCities() const
        {
            // RecursiveMutex::ScopedLock lock(Mutex);
            return m_cities;
        }

        void CityGeneratorDefault::removeCity( SmartPtr<IProceduralCity> city )
        {
        }

        void CityGeneratorDefault::addCity( SmartPtr<IProceduralCity> city )
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            m_cities.push_back( city );
        }

        String CityGeneratorDefault::getFilePath() const
        {
            return m_filePath;
        }

        void CityGeneratorDefault::setFilePath( const String &filePath )
        {
            m_filePath = filePath;
        }

        void CityGeneratorDefault::load( SmartPtr<ISharedObject> data )
        {
            auto filePath = getFilePath();
            loadFromFile( filePath );
        }

        SmartPtr<Properties> CityGeneratorDefault::osmDataToProperties( const String &filePath ) const
        {
            try
            {
                // Get the application manager instance
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                // Get the file system
                auto fileSystem = applicationManager->getFileSystem();
                WP_ASSERT( fileSystem );

                // Read the OSM file content
                auto text = fileSystem->readAllText( filePath );
                if( StringUtil::isNullOrEmpty( text ) )
                {
                    WP_LOG_ERROR( "File is empty or invalid: " + filePath );
                    return nullptr;
                }

                // Parse the XML content
                text = StringUtil::replaceAll( text, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>", "" );
                //auto osmData = workphone::make_ptr<Data<data::osm>>();
                //auto data = osmData->getDataAsType<data::osm>();
                //DataUtil::parseXML( text, data );

                // Convert the parsed OSM data to a Properties object
                auto properties = workphone::make_ptr<Properties>();
                //auto jsonData = DataUtil::toString( data );
                //properties->fromJSON( jsonData );

                return properties;
            }
            catch( const std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        void CityGeneratorDefault::setMeshGenerator( SmartPtr<IMeshGenerator> meshGenerator )
        {
            m_meshGenerator = meshGenerator;
        }

        SmartPtr<IMeshGenerator> CityGeneratorDefault::getMeshGenerator() const
        {
            return m_meshGenerator;
        }
    }  // namespace procedural
}  // namespace workphone
