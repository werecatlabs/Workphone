#include <WPPythonBind/WPPythonBindPCH.hpp>
#include <WPPythonBind/Converters/StringConverter.hpp>
#include <boost/python.hpp>

namespace fb
{

    PyObject *String_to_python_str::convert( String const &s )
    {
        return boost::python::incref( boost::python::object( s.c_str() ).ptr() );
    }

    String_from_python_str::String_from_python_str()
    {
        boost::python::converter::registry::push_back( &convertible, &construct,
                                                       boost::python::type_id<String>() );
    }

    void *String_from_python_str::convertible( PyObject *obj_ptr )
    {
        //if( !PyString_Check( obj_ptr ) )
        //    return 0;

        return obj_ptr;
    }

    void String_from_python_str::construct(
        PyObject *obj_ptr, boost::python::converter::rvalue_from_python_stage1_data *data )
    {
        /*
        // Extract the character data from the python string
        const char *value = PyString_AsString( obj_ptr );

        // Verify that obj_ptr is a string (should be ensured by convertible())
        assert( value );

        // Grab pointer to memory into which to construct the new QString
        void *storage =
            ( (boost::python::converter::rvalue_from_python_storage<String> *)data )->storage.bytes;

        // in-place construct the new QString using the character data
        // extraced from the python object
        new( storage ) String( value );

        // Stash the memory chunk pointer for later use by boost.python
        data->convertible = storage;
        */
    }

}  // end namespace fb
