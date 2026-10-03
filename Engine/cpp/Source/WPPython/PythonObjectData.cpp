#include <WPPython/PythonObjectData.hpp>

namespace fb
{

    PythonObjectData::PythonObjectData()
    {
    }

    PythonObjectData::~PythonObjectData()
    {
    }

    void PythonObjectData::setOwner( SmartPtr<ISharedObject> owner )
    {
        m_owner = owner;
    }

    SmartPtr<ISharedObject> PythonObjectData::getOwner() const
    {
        return m_owner;
    }

    void *PythonObjectData::getObjectData() const
    {
        return (void *)&m_object;
    }

    String PythonObjectData::getClassName() const
    {
        return m_className;
    }

    void PythonObjectData::setClassName( const String &className )
    {
        m_className = className;
    }

    boost::python::object &PythonObjectData::getObject()
    {
        return m_object;
    }

}  // end namespace fb
