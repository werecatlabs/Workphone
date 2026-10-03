#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Test/TestUtil.hpp>
#include <Workphone/Memory/MemoryTracker.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace test
    {
        void TestUtil::setupDefault()
        {
            auto applicationManager = new core::ApplicationManager;
            WP_ASSERT( applicationManager );

            core::IApplicationManager::setInstance( applicationManager );
            WP_ASSERT( core::ApplicationManager::instance() );

            auto factoryManager = workphone::make_ptr<FactoryManager>();
            applicationManager->setFactoryManager( factoryManager );
            WP_ASSERT( applicationManager->getFactoryManager() );

            auto logManager = workphone::make_ptr<LogManagerDefault>();
            applicationManager->setLogManager( logManager );
            logManager->open( "DatabaseTests.log" );

            auto fileSystem = factoryManager->make_object<IFileSystem>();
            applicationManager->setFileSystem( fileSystem );
            WP_ASSERT( fileSystem );

            auto workingDirectory = Path::getWorkingDirectory();
            fileSystem->addFolder( workingDirectory, true );

            auto stateManager = workphone::make_ptr<StateManager>();
            applicationManager->setStateManager( stateManager );
            WP_ASSERT( applicationManager->getStateManager() );

            auto timer = workphone::make_ptr<TimerMT>();
            applicationManager->setTimer( timer );

            auto fsmManager = workphone::make_ptr<FSMManager>();
            fsmManager->load( nullptr );
            WP_ASSERT( fsmManager->isLoaded() );
            WP_ASSERT( fsmManager->isValid() );

            applicationManager->setFsmManager( fsmManager );
            WP_ASSERT( applicationManager->isValid() );
        }

        void TestUtil::setupGraphics()
        {
            auto applicationManager = new core::ApplicationManager;
            WP_ASSERT( applicationManager );

            core::IApplicationManager::setInstance( applicationManager );
            WP_ASSERT( core::ApplicationManager::instance() );

            auto factoryManager = workphone::make_ptr<FactoryManager>();
            applicationManager->setFactoryManager( factoryManager );
            WP_ASSERT( applicationManager->getFactoryManager() );

            auto logManager = workphone::make_ptr<LogManagerDefault>();
            applicationManager->setLogManager( logManager );
            logManager->open( "DatabaseTests.log" );

            auto fileSystem = factoryManager->make_object<IFileSystem>();
            applicationManager->setFileSystem( fileSystem );
            WP_ASSERT( fileSystem );

            auto workingDirectory = Path::getWorkingDirectory();
            fileSystem->addFolder( workingDirectory, true );

            auto stateManager = workphone::make_ptr<StateManager>();
            applicationManager->setStateManager( stateManager );
            WP_ASSERT( applicationManager->getStateManager() );

            auto timer = workphone::make_ptr<TimerMT>();
            applicationManager->setTimer( timer );

            auto fsmManager = workphone::make_ptr<FSMManager>();
            fsmManager->load( nullptr );
            WP_ASSERT( fsmManager->isLoaded() );
            WP_ASSERT( fsmManager->isValid() );

            applicationManager->setFsmManager( fsmManager );
            WP_ASSERT( applicationManager->isValid() );

            auto graphicsSystem = factoryManager->make_object<render::IGraphicsSystem>();
            if( graphicsSystem )
            {
                applicationManager->setGraphicsSystem( graphicsSystem );
                graphicsSystem->load( nullptr );

                if( graphicsSystem->configure( nullptr ) )
                {
                    auto window = graphicsSystem->getDefaultWindow();
                    WP_ASSERT( window );

                    auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
                    WP_ASSERT( resourceGroupManager );
                    resourceGroupManager->load( nullptr );

                    auto renderSceneManager =
                        graphicsSystem->addGraphicsScene( "DefaultSceneManager", "GameSceneManager" );
                    WP_ASSERT( graphicsSystem->getGraphicsScene() );

                    auto camera = renderSceneManager->addGraphicsObjectByType<render::IGraphicsCamera>();

                    camera->setName( "DefaultCamera" );

                    auto cameraSceneNode =
                        renderSceneManager->getRootSceneNode()->addChildSceneNode( "DefaultCamera" );
                    cameraSceneNode->attachObject( camera );

                    camera->setNearClipDistance( 0.001f );
                    camera->setFarClipDistance( 10000.f );

                    cameraSceneNode->setPosition( Vector3<real_Num>( 1000, 800, 1000 ) );
                    cameraSceneNode->lookAt( Vector3<real_Num>::zero() );
                }
            }
        }

        void TestUtil::setupGame()
        {
            WP_DEBUG_TRACE;

            auto applicationManager = new core::ApplicationManager;
            WP_ASSERT( applicationManager );

            core::IApplicationManager::setInstance( applicationManager );
            WP_ASSERT( core::ApplicationManager::instance() );

            auto factoryManager = workphone::make_ptr<FactoryManager>();
            applicationManager->setFactoryManager( factoryManager );
            WP_ASSERT( applicationManager->getFactoryManager() );

            auto logManager = workphone::make_ptr<LogManagerDefault>();
            applicationManager->setLogManager( logManager );
            logManager->open( "DatabaseTests.log" );

            auto taskManager = workphone::make_ptr<TaskManager>();
            applicationManager->setTaskManager( taskManager );

            auto fileSystem = factoryManager->make_object<IFileSystem>();
            applicationManager->setFileSystem( fileSystem );
            WP_ASSERT( fileSystem );

            auto workingDirectory = Path::getWorkingDirectory();
            fileSystem->addFolder( workingDirectory, true );

            auto mediaFolderPath = String();

#if defined WP_PLATFORM_WIN32
            mediaFolderPath = String( "../../../../Media/" );
#elif defined WP_PLATFORM_APPLE
            mediaFolderPath = String( "../../Media/" );
#else
            mediaFolderPath = String( "../../Media/" );
#endif

            auto cachePath = applicationManager->getCachePath();
            auto cacheAbsolutePath = Path::getAbsolutePath( workingDirectory, cachePath );
            fileSystem->addFolder( cacheAbsolutePath );

            auto settingsCachePath = applicationManager->getSettingsPath();
            auto settingsCacheAbsolutePath =
                Path::getAbsolutePath( workingDirectory, settingsCachePath );
            fileSystem->addFolder( settingsCacheAbsolutePath );

            fileSystem->addFolder( mediaFolderPath, true );

            applicationManager->setMediaPath( mediaFolderPath );

            auto stateManager = workphone::make_ptr<StateManager>();
            applicationManager->setStateManager( stateManager );
            WP_ASSERT( applicationManager->getStateManager() );

            auto timer = workphone::make_ptr<TimerMT>();
            applicationManager->setTimer( timer );

            auto fsmManager = workphone::make_ptr<FSMManager>();
            fsmManager->load( nullptr );
            WP_ASSERT( fsmManager->isLoaded() );
            WP_ASSERT( fsmManager->isValid() );

            applicationManager->setFsmManager( fsmManager );
            WP_ASSERT( applicationManager->isValid() );

            auto meshManager = workphone::make_ptr<MeshManager>();
            applicationManager->setMeshManager( meshManager );

            auto meshLoader = factoryManager->make_object<IMeshLoader>();
            applicationManager->setMeshLoader( meshLoader );

            auto graphicsSystem = factoryManager->make_object<render::IGraphicsSystem>();
            if( graphicsSystem )
            {
                applicationManager->setGraphicsSystem( graphicsSystem );
                graphicsSystem->load( nullptr );

                if( graphicsSystem->configure( nullptr ) )
                {
                    auto window = graphicsSystem->getDefaultWindow();
                    WP_ASSERT( window );

                    auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
                    WP_ASSERT( resourceGroupManager );
                    resourceGroupManager->load( nullptr );

                    WP_ASSERT( resourceGroupManager->isLoaded() );
                    WP_ASSERT( resourceGroupManager->isValid() );

                    auto renderSceneManager =
                        graphicsSystem->addGraphicsScene( "DefaultSceneManager", "GameSceneManager" );
                    WP_ASSERT( graphicsSystem->getGraphicsScene() );

                    auto camera = renderSceneManager->addGraphicsObjectByType<render::IGraphicsCamera>();

                    camera->setName( "DefaultCamera" );

                    auto cameraSceneNode =
                        renderSceneManager->getRootSceneNode()->addChildSceneNode( "DefaultCamera" );
                    cameraSceneNode->attachObject( camera );

                    camera->setNearClipDistance( 0.001f );
                    camera->setFarClipDistance( 10000.f );

                    cameraSceneNode->setPosition( Vector3<real_Num>( 1000, 800, 1000 ) );
                    cameraSceneNode->lookAt( Vector3<real_Num>::zero() );
                }
            }

            auto resourceDatabase = workphone::make_ptr<ResourceDatabase>();
            resourceDatabase->load( nullptr );
            applicationManager->setResourceDatabase( resourceDatabase );

            WP_ASSERT( applicationManager->getPhysicsManager() == nullptr );
            auto physicsManager = factoryManager->make_object<physics::IPhysicsManager>();
            applicationManager->setPhysicsManager( physicsManager );
            WP_ASSERT( applicationManager->getPhysicsManager() != nullptr );

            // create scene
            auto physicsScene = physicsManager->addScene();
            physicsManager->setPhysicsScene( physicsScene );

            auto scriptManager = factoryManager->make_object<IScriptManager>();
            scriptManager->load( nullptr );
            applicationManager->setScriptManager( scriptManager );

            auto sceneManager = workphone::make_ptr<scene::GameManager>();
            applicationManager->setGameManager( sceneManager );
            sceneManager->load( nullptr );

            auto scene = workphone::make_ptr<scene::GameScene>();
            scene->load( nullptr );
            scene->setLabel( "Untitled" );
            sceneManager->setCurrentScene( scene );
        }

        void TestUtil::setupFactories()
        {
            auto applicationManager = core::ApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto typeManager = TypeManager::instance();
            WP_ASSERT( typeManager );

            //FactoryUtil::addFactory<scene::CarController>();
            FactoryUtil::addFactory<scene::Constraint>();
            FactoryUtil::addFactory<scene::CollisionBox>();
            FactoryUtil::addFactory<scene::CollisionMesh>();
            FactoryUtil::addFactory<scene::Material>();
            FactoryUtil::addFactory<scene::Mesh>();
            FactoryUtil::addFactory<scene::MeshRenderer>();
            FactoryUtil::addFactory<scene::Rigidbody>();
            //FactoryUtil::addFactory<scene::WheelController>();

            FactoryUtil::addFactory<StateMessageVector3>();
            FactoryUtil::addFactory<StateMessageVector4>();
            FactoryUtil::addFactory<StateMessageUIntValue>();
            FactoryUtil::addFactory<StateMessageIntValue>();
            FactoryUtil::addFactory<StateMessageVisible>();

            factoryManager->setPoolSizeByType<StateMessageVector3>( 128 );
            factoryManager->setPoolSizeByType<StateMessageVector4>( 128 );
            factoryManager->setPoolSizeByType<StateMessageUIntValue>( 128 );
            factoryManager->setPoolSizeByType<StateMessageIntValue>( 128 );
            factoryManager->setPoolSizeByType<StateMessageVisible>( 128 );
        }

        void TestUtil::destroyDefault()
        {
            auto applicationManager = core::ApplicationManager::instance();
            WP_ASSERT( applicationManager );

            applicationManager->unload( nullptr );
            core::ApplicationManager::setInstance( nullptr );
            applicationManager = nullptr;
            WP_ASSERT( core::ApplicationManager::instance() == nullptr );

#if WP_ENABLE_MEMORY_TRACKER
            auto &memoryTracker = MemoryTracker::get();
            memoryTracker.reportLeaks();
#endif
        }

        Array<s32> TestUtil::twoSum( Array<s32> &nums, s32 target )
        {
            Map<int, int> st;
            int i, n = (int)nums.size();
            Array<s32> found;
            for( i = 0; i < n; i++ )
            {
                // int pos=;
                Map<int, int>::iterator it;
                it = st.find( target - nums[i] );
                if( it != st.end() )
                {
                    found.push_back( it->second );

                    found.push_back( i );
                    break;
                }
                st.insert( { nums[i], i } );
            }
            return found;
        }

        void TestUtil::twoSumII( Array<s32> &nums, s32 i, Array<Array<s32>> &res )
        {
            int lo = i + 1, hi = (s32)nums.size() - 1;
            while( lo < hi )
            {
                int sum = nums[i] + nums[lo] + nums[hi];
                if( sum < 0 )
                {
                    ++lo;
                }
                else if( sum > 0 )
                {
                    --hi;
                }
                else
                {
                    res.push_back( { nums[i], nums[lo++], nums[hi--] } );
                    while( lo < hi && nums[lo] == nums[lo - 1] )
                        ++lo;
                }
            }
        }

        Array<Array<s32>> TestUtil::threeSum( Array<s32> &nums )
        {
            std::sort( std::begin( nums ), std::end( nums ) );

            Array<Array<s32>> res;
            for( int i = 0; i < nums.size() && nums[i] <= 0; ++i )
            {
                if( i == 0 || nums[i - 1] != nums[i] )
                {
                    twoSumII( nums, i, res );
                }
            }

            return res;
        }

        int TestUtil::trap( Array<s32> &height )
        {
            if( height.empty() )
                return 0;

            int ans = 0;
            int size = (int)height.size();

            Array<s32> left_max( size ), right_max( size );
            left_max[0] = height[0];
            for( int i = 1; i < size; i++ )
            {
                left_max[i] = std::max( height[i], left_max[i - 1] );
            }

            right_max[size - 1] = height[size - 1];

            for( int i = size - 2; i >= 0; i-- )
            {
                right_max[i] = std::max( height[i], right_max[i + 1] );
            }

            for( int i = 1; i < size - 1; i++ )
            {
                ans += std::min( left_max[i], right_max[i] ) - height[i];
            }

            return ans;
        }

        int TestUtil::trap_On( Array<s32> &height )
        {
            if( height.empty() )
                return 0;

            int n = (int)height.size();
            Array<s32> leftMax( n );
            Array<s32> rightMax( n );

            leftMax[0] = height[0];

            for( int i = 1; i < n; i++ )
                leftMax[i] = std::max( leftMax[i - 1], height[i] );

            rightMax[n - 1] = height[n - 1];
            for( int i = n - 2; i >= 0; i-- )
                rightMax[i] = std::max( rightMax[i + 1], height[i] );

            int sum = 0;

            for( int i = 0; i < n; i++ )
                sum += std::min( leftMax[i], rightMax[i] ) - height[i];

            return sum;
        }

        Array<Array<s32>> TestUtil::multiply_naive_iteration( Array<Array<s32>> &mat1,
                                                              Array<Array<s32>> &mat2 )
        {
            int n = (int)mat1.size();
            int k = (int)mat1[0].size();
            int m = (int)mat2[0].size();

            // Product matrix will have 'n x m' size.
            Array<Array<s32>> ans( n, Array<s32>( m, 0 ) );

            for( int rowIndex = 0; rowIndex < n; ++rowIndex )
            {
                for( int elementIndex = 0; elementIndex < k; ++elementIndex )
                {
                    // If current element of mat1 is non-zero then iterate over all columns of
                    // mat2.
                    if( mat1[rowIndex][elementIndex] != 0 )
                    {
                        for( int colIndex = 0; colIndex < m; ++colIndex )
                        {
                            ans[rowIndex][colIndex] +=
                                mat1[rowIndex][elementIndex] * mat2[elementIndex][colIndex];
                        }
                    }
                }
            }

            return ans;
        }

        Array<Array<Pair<int, int>>> TestUtil::compressMatrix( Array<Array<s32>> &matrix )
        {
            int rows = (int)matrix.size();
            int cols = (int)matrix[0].size();
            Array<Array<Pair<int, int>>> compressedMatrix( rows );

            for( int row = 0; row < rows; ++row )
            {
                for( int col = 0; col < cols; ++col )
                {
                    if( matrix[row][col] != 0 )
                    {
                        compressedMatrix[row].emplace_back( matrix[row][col], col );
                    }
                }
            }
            return compressedMatrix;
        }

        Array<Array<s32>> TestUtil::multiply( Array<Array<s32>> &mat1, Array<Array<s32>> &mat2 )
        {
            int m = (int)mat1.size();
            int k = (int)mat1[0].size();
            int n = (int)mat2[0].size();

            // Store the non-zero values of each matrix.
            Array<Array<Pair<int, int>>> A = compressMatrix( mat1 );
            Array<Array<Pair<int, int>>> B = compressMatrix( mat2 );

            Array<Array<s32>> ans( m, Array<s32>( n, 0 ) );

            for( int mat1Row = 0; mat1Row < m; ++mat1Row )
            {
                // Iterate on all current 'row' non-zero elements of mat1.
                for( const auto &element1 : A[mat1Row] )
                {
                    // Multiply and add all non-zero elements of mat2
                    // where the row is equal to col of current element of mat1.
                    const auto mat1Col = element1.second;
                    for( const auto &element2 : B[mat1Col] )
                    {
                        const auto mat2Col = element2.second;
                        ans[mat1Row][mat2Col] += element1.first * element2.first;
                    }
                }
            }

            return ans;
        }
    }  // namespace test
}  // namespace workphone
