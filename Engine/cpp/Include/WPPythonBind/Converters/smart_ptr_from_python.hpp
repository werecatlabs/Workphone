#ifndef smart_ptr_from_python_h__
#define smart_ptr_from_python_h__



# include <boost/python/handle.hpp>
#include <WPPythonBind\Converters\smart_ptr_deleter.hpp>
# include <boost/python/converter/from_python.hpp>
# include <boost/python/converter/rvalue_from_python_data.hpp>
# include <boost/python/converter/registered.hpp>
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
# include <boost/python/converter/pytype_function.hpp>
#endif

#include <Workphone/Memory/SmartPtr.hpp>

namespace boost 
{ 
	namespace python 
	{
		namespace converter { 

		template <class T>
		struct smart_ptr_from_python
		{
			smart_ptr_from_python()
			{
				converter::registry::insert(&convertible, &construct, type_id<fb::SmartPtr<T> >()
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
					, &converter::expected_from_python_type_direct<T>::get_pytype
#endif
					);
			}

		private:
			static void* convertible(PyObject* p)
			{
				if (p == Py_None)
					return p;

				return converter::get_lvalue_from_python(p, converter::registered<T>::converters);
			}

			static void construct(PyObject* source, converter::rvalue_from_python_stage1_data* data)
			{
				void* const storage = ((converter::rvalue_from_python_storage<fb::SmartPtr<T> >*)data)->storage.bytes;
				// Deal with the "None" case.
				if (data->convertible == source)
					new (storage) fb::SmartPtr<T>();
				else
				{
					//fb::SmartPtr<void> hold_convertible_ref_count(
					//	(void*)0, smart_ptr_deleter(handle<>(borrowed(source))) );
					//// use aliasing constructor
					//new (storage) fb::SmartPtr<T>(
					//	hold_convertible_ref_count,
					//	static_cast<T*>(data->convertible));
					//	
					new (storage) fb::SmartPtr<T>(static_cast<T*>(data->convertible));
				}

				data->convertible = storage;
			}
		};

		}
	}
} // namespace boost::python::converter

#endif // smart_ptr_from_python_h__
