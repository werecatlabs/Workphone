#include <WPPythonBind/WPPythonBindPCH.hpp>
#include <WPPythonBind/Bindings/BindCore.hpp>
#include <Workphone/Workphone.hpp>
#include <boost/python.hpp>
#include <WPPythonBind/Helpers/FactoryHelper.hpp>
#include <WPPythonBind/Helpers/StringUtilHelper.hpp>

namespace fb
{

    python_Integer _or( python_Integer val0, python_Integer val1 )
    {
        return val0 | val1;
    }

    python_Integer _shiftLeft( python_Integer val0, python_Integer val1 )
    {
        return val0 << val1;
    }

    void bindCore()
    {
        using namespace boost::python;

        class_<IFactory, SmartPtr<IFactory>, bases<ISharedObject>, boost::noncopyable>( "IFactory", no_init )
            .def( "create", FactoryHelper::create )
            .def( "createFromScript", FactoryHelper::createFromScript )
            .def( "createById", FactoryHelper::createById );

        class_<StringUtil, boost::noncopyable>( "StringUtil", no_init )
            .def( "hash", StringUtilHelper::_getStringHash )
            .staticmethod( "hash" )
            .def( "hashLowerCase", StringUtilHelper::_getStringHashLower )
            .staticmethod( "hashLowerCase" )
            .def( "toString", StringUtilHelper::_toStringInt )
            .def( "toString", StringUtilHelper::_toStringNumber )
            .def( "toString", StringUtilHelper::_toStringObject<IObject> )
            .def( "toString", StringUtilHelper::_toStringObject<scene::IActor> )
            //.def( "toString", StringUtilHelper::_toStringObject<VehicleTemplate> )
            .staticmethod( "toString" );

        class_<BitUtil, boost::noncopyable>( "BitUtil", no_init )
            .def( "bit_and", BitUtil::bit_and<python_Integer> )
            .staticmethod( "bit_and" )
            .def( "bit_or", _or )
            .staticmethod( "bit_or" )
            .def( "shift_left", _shiftLeft )
            .staticmethod( "shift_left" );
    }

}  // end namespace fb
