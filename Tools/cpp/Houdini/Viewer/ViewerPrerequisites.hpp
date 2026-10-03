#ifndef MeshViewerPrerequisites_h__
#define MeshViewerPrerequisites_h__



#define _WINSOCKAPI_ 

#include <FBCore/Memory/SmartPtr.hpp>
#include <FBCore/Thread/Threading.hpp>
#include <FBCore/FBCoreHeaders.hpp>
#include <FBCore/Base/Singleton.hpp>
#include <FBCore/FBCorePrerequisites.hpp>
#include <FBApplication/FBApplicationPrerequisites.hpp>
#include <FBProcedural/FBProceduralPrerequisites.hpp>

class HAPI_Session;
class HAPI_CookOptions;

namespace fb
{
	namespace ui
	{
		class wxFourWaySplitter;

	}

	namespace viewer
	{
		class MainFrame;
	}
}

#endif // MeshViewerPrerequisites_h__
