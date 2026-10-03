#ifndef WPOISKeyConverter_h__
#define WPOISKeyConverter_h__

#include <Workphone/Core/Singleton.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <OISKeyboard.h>

namespace workphone
{
    class OISKeyConverter : public Singleton<OISKeyConverter>
    {
    public:
        OISKeyConverter();
        ~OISKeyConverter();

        OIS::KeyCode getCodeFromString( const String &key ) const;

    private:
        using Keys = std::map<String, OIS::KeyCode>;
        Keys keys;
    };
}  // namespace workphone

#endif  // WPOISKeyConverter_h__
