#ifndef WPLua_h__
#define WPLua_h__

#include <WPLua/WPLuaPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <WPLua/WPLuaAutolink.hpp>

namespace workphone
{

    /** @class WPLua
     * @brief
     *      This class is the main class of the Lua plugin.
     * @note
     *      This class is a singleton.
     */
    class WPLua_API WPLua : public ISharedObject
    {
    public:
        /** Constructor.*/
        WPLua();

        /** Destructor.*/
        ~WPLua() override;

        /** @copydoc ISharedObject::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISharedObject::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** Get the instance of the plugin.
         * @return
         *      The instance of the plugin.
         */
        static SmartPtr<WPLua> instance();

        /** Set the instance of the plugin.
         * @param plugin
         *      The instance of the plugin.
         */
        static void setInstance( SmartPtr<WPLua> plugin );

    protected:
        /** Plugin object. */
        static SmartPtr<WPLua> m_sPlugin;
    };

}  // namespace workphone

#endif  // WPLua_h__
