#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

#if WP_ENABLE_PYTHON
#    include <boost/python.hpp>

// An abstract base class
class Base : public boost::noncopyable
{
public:
    virtual ~Base() {};
    virtual std::string hello() = 0;
};

// C++ derived class
class CppDerived : public Base
{
public:
    ~CppDerived() override
    {
    }

    std::string hello() override
    {
        return "Hello from C++!";
    }
};

// Familiar Boost.Python wrapper class for Base
struct BaseWrap : Base, boost::python::wrapper<Base>
{
    std::string hello() override
    {
#    if BOOST_WORKAROUND( BOOST_MSVC, <= 1300 )
        // workaround for VC++ 6.x or 7.0, see
        // http://boost.org/libs/python/doc/tutorial/doc/html/python/exposing.html#python.class_virtual_functions
        return python::call<std::string>( this->get_override( "hello" ).ptr() );
#    else
        return this->get_override( "hello" )();
#    endif
    }
};

// Pack the Base class wrapper into a module
BOOST_PYTHON_MODULE( embedded_hello )
{
    namespace python = boost::python;
    python::class_<BaseWrap, boost::noncopyable> base( "Base" );
}

void exec_test()
{
    namespace python = boost::python;
    std::cout << "registering extension module embedded_hello..." << std::endl;

    /*
    // Register the module with the interpreter
    //auto module = PyInit_embedded_hello();
    if( PyImport_AppendInittab( "embedded_hello", PyInit_embedded_hello ) == -1 )
        throw std::runtime_error(
            "Failed to add embedded_hello to the interpreter's "
            "builtin modules" );

    std::cout << "defining Python class derived from Base..." << std::endl;

    // Retrieve the main module
    python::object main = python::import( "__main__" );

    // Retrieve the main module's namespace
    python::object global( main.attr( "__dict__" ) );

    // Define the derived class in Python.
    python::object result = exec(
        "from embedded_hello import *        \n"
        "class PythonDerived(Base):          \n"
        "    def hello(self):                \n"
        "        return 'Hello from Python!' \n",
        global, global );

    python::object PythonDerived = global["PythonDerived"];

    // Creating and using instances of the C++ class is as easy as always.
    CppDerived cpp;
    BOOST_TEST( cpp.hello() == "Hello from C++!" );

    std::cout << "testing derived class from C++..." << std::endl;

    // But now creating and using instances of the Python class is almost
    // as easy!
    python::object py_base = PythonDerived();
    Base &py = python::extract<Base &>( py_base ) BOOST_EXTRACT_WORKAROUND;

    // Make sure the right 'hello' method is called.
    BOOST_TEST( py.hello() == "Hello from Python!" );

    std::cout << "success!" << std::endl;
    */
}

BOOST_AUTO_TEST_CASE( python_tests )
{
    using namespace workphone;

    ////test
    //auto obj = workphone::make_ptr<CSharedObject<ISharedObject>>();
    //createObject( "Splash", obj );

    //int stop = 0;
    //stop = 0;
}

BOOST_AUTO_TEST_CASE( python_create_object )
{
    using namespace workphone;

    ////test
    //auto obj = workphone::make_ptr<CSharedObject<ISharedObject>>();
    //createObject( "Splash", obj );

    //int stop = 0;
    //stop = 0;
}

BOOST_AUTO_TEST_CASE( python_get_variables )
{
    using namespace workphone;

    ////test
    //auto obj = workphone::make_ptr<CSharedObject<ISharedObject>>();
    //createObject( "Splash", obj );

    //int stop = 0;
    //stop = 0;
}

#endif
