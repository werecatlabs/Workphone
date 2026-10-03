#ifndef smart_ptr_deleter_h__
#define smart_ptr_deleter_h__



namespace boost { namespace python { namespace converter { 

struct smart_ptr_deleter
{
    smart_ptr_deleter(handle<> owner);
    ~smart_ptr_deleter();

    void operator()(void const*);
        
    handle<> owner;
};

}}} // namespace boost::python::converter

#endif // smart_ptr_deleter_h__
