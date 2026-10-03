#ifndef Viewer_h__
#define Viewer_h__

#include "ViewerPrerequisites.hpp"
#include <FBApplication/CApplicationClient.hpp>
#include <HAPI/HAPI_Common.hpp>

namespace fb
{
    namespace viewer
    {

        class HoudiniViewer : public application::CApplicationClient
        {
        public:
            HoudiniViewer();
            ~HoudiniViewer();

            void load( SmartPtr<ISharedObject> data );
            int MainLoop();

            void Refresh();
            bool createGraphicsSystem();

            // Returns true if the set session is valid
            bool IsSessionValid();

            // Places the node name in Name
            // Return val: indicates whether the function succeeded
            bool GetNodeName( const HAPI_NodeInfo &NodeInfo, std::string &Name );

            // Places the string in HapiString
            // Return val: indicates whether the function succeeded
            bool GetHapiString( HAPI_StringHandle Handle, std::string &HapiString );

            // Places the node label in Label
            // Return val: indicates whether the function succeeded
            bool GetParameterLabel( const HAPI_ParmInfo &ParmInfo, std::string &Label );

            // Returns whether the parameter is a root level parameter
            bool IsParameterRootLevel( const HAPI_ParmInfo &ParmInfo );

            // Gets the 0-based index of a multi-parameter instance
            int GetMultiParmIndex( const HAPI_ParmInfo &ParmInfo );

        private:
            void createOgreMeshFromHoudini(HAPI_GeoInfo& geoInfo);

            HAPI_NodeId outputNodeId = -1;

            HAPI_Session *Session = nullptr;
            HAPI_NodeId Node;
            HAPI_NodeId NodeId;
            HAPI_CookOptions *CookOptions = nullptr;

            MainFrame *frame = nullptr;
        };

    }  // end namespace viewer
}  // end namespace fb

#endif  // Viewer_h__
