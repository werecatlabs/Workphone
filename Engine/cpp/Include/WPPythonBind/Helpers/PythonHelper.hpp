#ifndef PythonHelper_h__
#define PythonHelper_h__

#include <boost/python.hpp>
#include <WPPythonBind/Converters/smart_ptr_from_python.hpp>
#include <WPPythonBind/Converters/smart_ptr_to_python.hpp>

namespace fb
{

    class PythonHelper
    {
    public:
        template <class T>
        static void registerPointer()
        {
            using namespace boost::python;

            converter::smart_ptr_to_python<T>();
            converter::smart_ptr_from_python<T>();
            implicitly_convertible<SmartPtr<T>, SmartPtr<ISharedObject>>();
        }
    };

}  // end namespace fb

#endif  // PythonHelper_h__
