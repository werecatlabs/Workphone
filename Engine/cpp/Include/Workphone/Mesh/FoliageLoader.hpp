#ifndef NGPlantLoader_h__
#define NGPlantLoader_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone
{
    class WPCore_API FoliageLoader
    {
    public:
        static SmartPtr<IMesh> loadMesh( const String &plantFilePath );
    };
}  // namespace workphone

#endif  // NGPlantLoader_h__
