#ifndef smart_ptr_to_python_h__
#define smart_ptr_to_python_h__



# include <boost/python/refcount.hpp>
# include <boost/python/converter/shared_ptr_deleter.hpp>
//# include <boost/shared_ptr.hpp>
# include <boost/get_pointer.hpp>

#include <Workphone/Memory/SmartPtr.hpp>

namespace boost { namespace python { namespace converter { 

template <class T>
struct smart_ptr_to_python
{
	PyObject* convert(fb::SmartPtr<T> const& x)
	{
		if (!x)
			return python::detail::none();
		else
			return converter::registered<fb::SmartPtr<T> const&>::converters.to_python(&x);
	}

};

}}} // namespace boost::python::converter


#endif // smart_ptr_to_python_h__
