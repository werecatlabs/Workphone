#ifndef StringConverter_h__
#define StringConverter_h__

#include <Workphone/Base/StringTypes.hpp>
#include <boost/python.hpp>

namespace fb
{
    struct String_to_python_str
    {
        static PyObject *convert( String const &s );
    };

    struct String_from_python_str
    {
        String_from_python_str();

        // Determine if obj_ptr can be converted in a QString
        static void *convertible( PyObject *obj_ptr );

        // Convert obj_ptr into a QString
        static void construct( PyObject *obj_ptr,
                               boost::python::converter::rvalue_from_python_stage1_data *data );
    };

}  // namespace fb

#endif  // StringConverter_h__
