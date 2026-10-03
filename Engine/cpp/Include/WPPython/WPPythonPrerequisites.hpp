#ifndef WPPythonPrerequisites_h__
#define WPPythonPrerequisites_h__

#include <boost/python/object_fwd.hpp>
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace boost
{
    namespace python
    {
        class dict;
    }
    
}

namespace fb
{

    class PythonObjectData;
    typedef SmartPtr<PythonObjectData> PythonObjectDataPtr;

}  // namespace fb

#endif  // WPPythonPrerequisites_h__
