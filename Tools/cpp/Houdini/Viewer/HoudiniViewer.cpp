#include "ViewerPCH.hpp"
#include "HoudiniViewer.hpp"
#include <FBCore/FBCore.hpp>
#include <FBApplication/FBApplication.hpp>
#include <HAPI/HAPI.hpp>
#include <Ogre.hpp>

#include "FBGraphicsOgre/Ogre/OgreMain/include/OgreManualObject.hpp"

//#if FB_BUILD_OGRENEXT
//#include <FBGraphicsOgreNext/FBGraphicsOgre.hpp>
//#elif FB_BUILD_OGRE
//#include <FBGraphicsOgre/FBGraphicsOgre.hpp>
//#else
//#include <FBGraphicsD3D11/FBGraphicsD3D11.hpp>
//#endif

//wxIMPLEMENT_APP(fb::viewer::HoudiniViewer);

namespace fb
{
    namespace viewer
    {

        HoudiniViewer::HoudiniViewer()
        {
        }

        HoudiniViewer::~HoudiniViewer()
        {
        }

        void HoudiniViewer::createOgreMeshFromHoudini( HAPI_GeoInfo &geoInfo )
        {
            auto &mSession = *Session;
            HAPI_NodeId mNodeId = 0;

            // Create an Ogre3D manual mesh
            Ogre::ManualObject *manual = new Ogre::ManualObject( "HoudiniMesh" );
            manual->begin( "BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST );

            // Loop through the faces of the geometry
            int numFaces = geoInfo.partCount;
            std::vector<HAPI_PartInfo> partInfos( numFaces );
            std::vector<int> faceCounts( numFaces );
            HAPI_Result result = (HAPI_Result)0;//HAPI_GetPartInfo( &mSession, mNodeId, &partInfos[0], 0, numFaces );
            if( result != HAPI_RESULT_SUCCESS )
            {
                // Handle error
                return;
            }

            //result = HAPI_GetFaceCounts( &mSession, mNodeId, &faceCounts[0], 0, numFaces );
            if( result != HAPI_RESULT_SUCCESS )
            {
                // Handle error
                return;
            }

            int vertexIndex = 0;
            for( int i = 0; i < numFaces; i++ )
            {
                int faceCount = faceCounts[i];
                std::vector<int> vertexIndices( faceCount );
                //result =
                //    HAPI_GetVertexList( &mSession, mNodeId, &vertexIndices[0], vertexIndex, faceCount );
                if( result != HAPI_RESULT_SUCCESS )
                {
                    // Handle error
                    return;
                }

                for( int j = 0; j < faceCount; j++ )
                {
                    // Get the vertex position and normal
                    HAPI_AttributeInfo posAttrInfo;
                    HAPI_AttributeInfo norAttrInfo;
                    int posAttrIndex = -1;
                    int norAttrIndex = -1;

                    //result = HAPI_GetAttributeInfo( &mSession, mNodeId, vertexIndex + vertexIndices[j],
                    //                                HAPI_ATTROWNER_POINT, "P", HAPI_ATTRIBUTE_TYPE_FLOAT,
                    //                                &posAttrInfo );
                    if( result != HAPI_RESULT_SUCCESS )
                    {
                        // Handle error
                        return;
                    }

                    //result = HAPI_GetAttributeInfo( &mSession, mNodeId, vertexIndex + vertexIndices[j],
                    //                                HAPI_ATTROWNER_VERTEX, "N",
                    //                                HAPI_ATTRIBUTE_TYPE_FLOAT, &norAttrInfo );
                    if( result != HAPI_RESULT_SUCCESS )
                    {
                        // Handle error
                        return;
                    }

                    std::vector<float> posData( posAttrInfo.tupleSize );
                    std::vector<float> norData( norAttrInfo.tupleSize );
                    //result = HAPI_GetAttributeFloatData(
                    //    &mSession, mNodeId, vertexIndex + vertexIndices[j], "P", &posAttrInfo,
                    //    &posData[0], 0, posAttrInfo.count );
                    //if( result != HAPI_RESULT_SUCCESS )
                    //{
                    //    // Handle error
                    //    return;
                    //}

                    //result = HAPI_GetAttributeFloatData(
                    //    &mSession, mNodeId, vertexIndex + vertexIndices[j], "N", &norAttrInfo,
                    //    &norData[0], 0, norAttrInfo.count );
                    //if( result != HAPI_RESULT_SUCCESS )
                    //{
                    //    // Handle error
                    //    return;
                    //}

                    Ogre::Vector3 position( posData[0], posData[1], posData[2] );
                    Ogre::Vector3 normal( norData[0], norData[1], norData[2] );

                    // Add the vertex to the mesh
                }
            }
        }

        void HoudiniViewer::load( SmartPtr<ISharedObject> data )
        {
            auto applicationManager = fb::make_ptr<core::ApplicationManagerMT>();
            core::IApplicationManager::setInstance( applicationManager );

            auto logManager = fb::make_ptr<LogManager>();
            applicationManager->setLogManager( logManager );
            logManager->open( "HoudiniViewer.log" );

            auto timer = fb::make_ptr<TimerMT>();
            applicationManager->setTimer( timer );

            Session = new HAPI_Session();
            auto &session = *Session;

            CookOptions = new HAPI_CookOptions();
            *CookOptions = HAPI_CookOptions_Create();
            auto result = HAPI_CreateInProcessSession( Session );
            if( result != HAPI_RESULT_SUCCESS )
            {
                FB_LOG_ERROR( "Error creating session" );
            }

            HAPI_Initialize( Session, CookOptions, true, -1, nullptr, nullptr, nullptr, nullptr,
                             nullptr );

            auto filePath =
                String( "C:/dev/fireblade/Bin/windows/v142/x64/RelWithDebInfo/SideFX__spaceship.otl" );

            //auto filePath = String( "SideFX__spaceship.otl" );

            HAPI_AssetLibraryId AssetLibId;
            HAPI_LoadAssetLibraryFromFile( Session, filePath.c_str(), true, &AssetLibId );

            HAPI_StringHandle AssetName;
            HAPI_GetAvailableAssets( Session, AssetLibId, &AssetName, 1 );

            int BufferLength = 0;
            HAPI_GetStringBufLength( Session, AssetName, &BufferLength );

            char *Name = new char[BufferLength];

            HAPI_GetString( Session, AssetName, Name, BufferLength );

            HAPI_CreateNode( Session, -1, Name, nullptr, false, &NodeId );

            HAPI_CookNode( Session, NodeId, CookOptions );

            int CookStatus;
            HAPI_Result CookResult;

            do
            {
                CookResult = HAPI_GetStatus( Session, HAPI_STATUS_COOK_STATE, &CookStatus );
            } while( CookStatus > HAPI_STATE_MAX_READY_STATE && CookResult == HAPI_RESULT_SUCCESS );

            //// create the main application window
            //frame = new MainFrame( "FireBlade MeshViewer" );
            //frame->Show( true );
            //frame->Maximize( true );

            outputNodeId = -1;

            // Retrieve the geometry from the output node
            HAPI_GeoInfo geoInfo;
            result = HAPI_GetGeoInfo( &session, outputNodeId, &geoInfo );
            if( result != HAPI_RESULT_SUCCESS )
            {
                // Handle error
                //HAPI_UnloadAssetLibrary(&session, libraryId);
                HAPI_Cleanup( &session );
                return;
            }

            createOgreMeshFromHoudini( geoInfo );

            Thread::sleep( 3.0 );
            Refresh();
        }

        int HoudiniViewer::MainLoop()
        {
#if 0
			return wxApp::MainLoop();
#else
            auto currentThreadId = Thread::ThreadId::Primary;
            Thread::setCurrentThreadId( currentThreadId );

            auto task = Thread::Task::Primary;
            Thread::setCurrentTask( task );

            auto applicationManager = core::IApplicationManager::instance();
            FB_ASSERT( applicationManager );

            auto timer = applicationManager->getTimer();
            FB_ASSERT( timer );

            //auto loop = new ui::EventLoop;
            ////auto loop = new wxGUIEventLoop;
            //wxEventLoopBase::SetActive( loop );

            while( applicationManager->isRunning() )
            {
                //if (Pending()) // Unprocessed events in queue
                //{
                //	Dispatch(); // Dispatch next event in queue
                //}
            }
#endif

            return 0;
        }

        void HoudiniViewer::Refresh()
        {
            if( IsSessionValid() )
            {
                HAPI_NodeInfo NodeInfo;
                if( HAPI_GetNodeInfo( Session, Node, &NodeInfo ) == HAPI_RESULT_SUCCESS )
                {
                    std::string NodeName;
                    if( GetNodeName( NodeInfo, NodeName ) )
                    {
                        HAPI_ParmInfo *ParmInfos = new HAPI_ParmInfo[NodeInfo.parmCount];

                        if( HAPI_GetParameters( Session, Node, ParmInfos, 0, NodeInfo.parmCount ) ==
                            HAPI_RESULT_SUCCESS )
                        {
                            int stop = 0;
                            stop = 0;
                        }
                    }
                }
            }
        }

        bool HoudiniViewer::createGraphicsSystem()
        {
            /*
#if FB_BUILD_OGRENEXT
            auto graphicsSystem = render::FBGraphicsOgre::createGraphicsOgre();
#elif FB_BUILD_OGRE
            auto graphicsSystem = render::FBGraphicsOgre::createGraphicsOgre();
#else
            auto graphicsSystem = createGraphicsSystemD3D11();
#endif

#if FB_BUILD_OGRENEXT
            auto applicationManager = IApplicationManager::instance();
            applicationManager->setGraphicsSystem(graphicsSystem);

            graphicsSystem->load(nullptr);

            auto configuration = graphicsSystem->createConfiguration();
            FB_ASSERT(configuration);

            configuration->setCreateWindow(false);
            graphicsSystem->configure(configuration);

            auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
            graphicsSystem->loadObject(resourceGroupManager);
#elif FB_BUILD_OGRE
            auto applicationManager = IApplicationManager::instance();
            applicationManager->setGraphicsSystem(graphicsSystem);

            auto task = Thread::getCurrentTask();
            auto renderTask = graphicsSystem->getRenderTask();
            if (task == renderTask)
            {
                graphicsSystem->load(nullptr);

                auto configuration = graphicsSystem->createConfiguration();
                configuration->setCreateWindow(false);
                graphicsSystem->configure(configuration);
            }
            else
            {
                createRenderInitJob();
            }
#endif
            */

            return true;
        }

        bool HoudiniViewer::IsSessionValid()
        {
            if( HAPI_IsSessionValid( Session ) == HAPI_RESULT_SUCCESS )
            {
                return true;
            }

            return false;
        }

        bool HoudiniViewer::GetHapiString( HAPI_StringHandle Handle, std::string &HapiString )
        {
            int BufSize = 0;
            if( HAPI_GetStringBufLength( Session, Handle, &BufSize ) == HAPI_RESULT_SUCCESS )
            {
                char *Val = new char[BufSize];
                if( HAPI_GetString( Session, Handle, Val, BufSize ) == HAPI_RESULT_SUCCESS )
                {
                    HapiString = Val;
                    delete[] Val;
                    return true;
                }
                delete[] Val;
            }

            return false;
        }

        bool HoudiniViewer::GetNodeName( const HAPI_NodeInfo &NodeInfo, std::string &Name )
        {
            return GetHapiString( NodeInfo.nameSH, Name );
        }

        bool HoudiniViewer::GetParameterLabel( const HAPI_ParmInfo &ParmInfo, std::string &Label )
        {
            return GetHapiString( ParmInfo.labelSH, Label );
        }

        bool HoudiniViewer::IsParameterRootLevel( const HAPI_ParmInfo &ParmInfo )
        {
            if( ParmInfo.parentId < 0 )
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        int HoudiniViewer::GetMultiParmIndex( const HAPI_ParmInfo &ParmInfo )
        {
            int InstanceNum = ParmInfo.instanceNum;

            return ( ParmInfo.instanceStartOffset ? ( InstanceNum - 1 ) : InstanceNum );
        }

    }  // end namespace viewer
}  // end namespace fb

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow )
{
    fb::viewer::HoudiniViewer app;
    app.load( nullptr );
    app.run();
    return 0;
}
