#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/AI/AiManager.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Ai/IPathfinder2.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/Sound/ISoundManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>

#if WP_USE_BOOST
#    include <boost/json.hpp>
#    include <algorithm>
#    include <cmath>
#    include <filesystem>
#    include <fstream>
#    include <limits>
#    include <sstream>
#    if defined _WIN32
#        include <Windows.h>
#        include <winhttp.h>
#        pragma comment( lib, "winhttp.lib" )
#    else
#        include <boost/asio/connect.hpp>
#        include <boost/asio/io_context.hpp>
#        include <boost/asio/ip/tcp.hpp>
#        include <boost/beast/core.hpp>
#        include <boost/beast/http.hpp>
#        include <chrono>
#    endif
#endif

namespace workphone
{
#if WP_USE_BOOST
    namespace
    {
        namespace json = boost::json;

        constexpr auto ollamaHost = "127.0.0.1";
        constexpr auto ollamaPort = "11434";
        constexpr auto ollamaTarget = "/api/generate";
        constexpr auto ollamaModel = "mistral";
        constexpr auto ollamaTimeoutMilliseconds = 120000;
        constexpr size_t maximumControlResponseSize = 256 * 1024;
        constexpr size_t maximumActionsPerResponse = 64;
#    if !defined _WIN32
        namespace asio = boost::asio;
        namespace beast = boost::beast;
        namespace http = beast::http;
        using tcp = asio::ip::tcp;
        constexpr auto ollamaTimeout = std::chrono::seconds( 120 );
#    endif

        std::string toStdString( const String &value )
        {
            return std::string( value.data(), value.size() );
        }

        json::string toJsonString( const String &value )
        {
            return json::string( value.data(), value.size() );
        }

        String getResponseText( const json::value &value )
        {
            if( !value.is_object() )
            {
                return {};
            }

            const auto &object = value.as_object();
            const auto response = object.if_contains( "response" );
            if( !response || !response->is_string() )
            {
                return {};
            }

            const auto &text = response->as_string();
            return String( text.data(), text.size() );
        }

        bool parseControlResponse( const String &response, json::object &result )
        {
            if( response.empty() || response.size() > maximumControlResponseSize )
            {
                return false;
            }

            auto begin = response.find( '{' );
            auto end = response.rfind( '}' );
            if( begin == String::npos || end == String::npos || end < begin )
            {
                return false;
            }

            const auto jsonText = json::string_view( response.data() + begin, end - begin + 1 );
            boost::system::error_code error;
            auto value = json::parse( jsonText, error );
            if( error || !value.is_object() )
            {
                return false;
            }

            result = std::move( value.as_object() );
            return true;
        }

        String getControlMessage( const String &response )
        {
            json::object root;
            if( !parseControlResponse( response, root ) )
            {
                return response;
            }

            const auto message = root.if_contains( "message" );
            if( !message || !message->is_string() )
            {
                return response;
            }

            const auto &text = message->as_string();
            return String( text.data(), text.size() );
        }

        bool getActionName( const json::object &action, json::string_view &name )
        {
            auto value = action.if_contains( "name" );
            if( !value )
            {
                value = action.if_contains( "action" );
            }
            if( !value )
            {
                value = action.if_contains( "type" );
            }

            if( !value || !value->is_string() )
            {
                return false;
            }

            name = value->as_string();
            return true;
        }

        bool getActionBool( const json::object &action, bool &value )
        {
            const auto actionValue = action.if_contains( "value" );
            if( !actionValue || !actionValue->is_bool() )
            {
                return false;
            }

            value = actionValue->as_bool();
            return true;
        }

        bool getBool( const json::object &object, json::string_view key, bool &value )
        {
            const auto property = object.if_contains( key );
            if( !property || !property->is_bool() )
            {
                return false;
            }

            value = property->as_bool();
            return true;
        }

        bool getNumber( const json::value &value, real_Num &number )
        {
            double converted = 0.0;
            if( value.is_double() )
            {
                converted = value.as_double();
            }
            else if( value.is_int64() )
            {
                converted = static_cast<double>( value.as_int64() );
            }
            else if( value.is_uint64() )
            {
                converted = static_cast<double>( value.as_uint64() );
            }
            else
            {
                return false;
            }

            if( !std::isfinite( converted ) ||
                converted < static_cast<double>( std::numeric_limits<real_Num>::lowest() ) ||
                converted > static_cast<double>( std::numeric_limits<real_Num>::max() ) )
            {
                return false;
            }

            number = static_cast<real_Num>( converted );
            return true;
        }

        bool getNumber( const json::object &object, json::string_view key, real_Num &number )
        {
            const auto property = object.if_contains( key );
            return property && getNumber( *property, number );
        }

        bool getU32( const json::object &object, json::string_view key, u32 &number )
        {
            const auto property = object.if_contains( key );
            if( !property )
            {
                return false;
            }

            if( property->is_uint64() && property->as_uint64() <= std::numeric_limits<u32>::max() )
            {
                number = static_cast<u32>( property->as_uint64() );
                return true;
            }
            if( property->is_int64() && property->as_int64() >= 0 &&
                static_cast<u64>( property->as_int64() ) <= std::numeric_limits<u32>::max() )
            {
                number = static_cast<u32>( property->as_int64() );
                return true;
            }

            return false;
        }

        bool getString( const json::object &object, json::string_view key, String &value )
        {
            const auto property = object.if_contains( key );
            if( !property || !property->is_string() )
            {
                return false;
            }

            const auto &text = property->as_string();
            if( text.empty() || text.size() > 256 )
            {
                return false;
            }

            value.assign( text.data(), text.size() );
            return true;
        }

        bool getOptionalString( const json::object &object, json::string_view key, String &value )
        {
            const auto property = object.if_contains( key );
            if( !property )
            {
                return true;
            }
            if( !property->is_string() )
            {
                return false;
            }

            const auto &text = property->as_string();
            if( text.size() > 256 )
            {
                return false;
            }
            if( !text.empty() )
            {
                value.assign( text.data(), text.size() );
            }
            return true;
        }

        bool getVector3( const json::value &value, Vector3<real_Num> &vector )
        {
            real_Num x = 0;
            real_Num y = 0;
            real_Num z = 0;

            if( value.is_array() )
            {
                const auto &array = value.as_array();
                if( array.size() != 3 || !getNumber( array[0], x ) || !getNumber( array[1], y ) ||
                    !getNumber( array[2], z ) )
                {
                    return false;
                }
            }
            else if( value.is_object() )
            {
                const auto &object = value.as_object();
                if( !getNumber( object, "x", x ) || !getNumber( object, "y", y ) ||
                    !getNumber( object, "z", z ) )
                {
                    return false;
                }
            }
            else
            {
                return false;
            }

            constexpr auto maximumMagnitude = static_cast<real_Num>( 1000000.0 );
            if( std::abs( x ) > maximumMagnitude || std::abs( y ) > maximumMagnitude ||
                std::abs( z ) > maximumMagnitude )
            {
                return false;
            }

            vector = Vector3<real_Num>( x, y, z );
            return true;
        }

        bool getVector3( const json::object &object, json::string_view key, Vector3<real_Num> &vector )
        {
            const auto property = object.if_contains( key );
            return property && getVector3( *property, vector );
        }

        bool isConfirmed( const json::object &action )
        {
            bool confirmed = false;
            return getBool( action, "confirm", confirmed ) && confirmed;
        }

        bool getMediaActionPath( core::IApplicationManager *applicationManager,
                                 const json::object &action, std::filesystem::path &result )
        {
            String relativePath;
            if( !applicationManager || !getString( action, "path", relativePath ) )
            {
                return false;
            }

            const auto candidate = std::filesystem::u8path( toStdString( relativePath ) );
            if( candidate.empty() || candidate.is_absolute() || candidate.has_root_directory() ||
                candidate.has_root_name() )
            {
                return false;
            }

            for( const auto &part : candidate )
            {
                const auto component = part.string();
                if( component.empty() || component == "." || component == ".." ||
                    component.size() > 128 || component.find_first_of( "<>:\"|?*" ) != String::npos )
                {
                    return false;
                }
            }

            const auto mediaPath = applicationManager->getMediaPath();
            if( StringUtil::isNullOrEmpty( mediaPath ) )
            {
                return false;
            }

            std::error_code error;
            auto mediaRoot = std::filesystem::weakly_canonical(
                std::filesystem::absolute( std::filesystem::u8path( toStdString( mediaPath ) ), error ),
                error );
            if( error || mediaRoot.empty() || !std::filesystem::is_directory( mediaRoot, error ) )
            {
                return false;
            }

            auto target = ( mediaRoot / candidate ).lexically_normal();
            auto relativeToRoot = std::filesystem::relative( target, mediaRoot, error );
            if( error || relativeToRoot.empty() || relativeToRoot.is_absolute() )
            {
                return false;
            }
            for( const auto &part : relativeToRoot )
            {
                if( part == ".." )
                {
                    return false;
                }
            }

            result = std::move( target );
            return true;
        }

        bool createMediaFolder( core::IApplicationManager *applicationManager,
                                const json::object &action )
        {
            std::filesystem::path folderPath;
            if( !getMediaActionPath( applicationManager, action, folderPath ) )
            {
                return false;
            }

            std::error_code error;
            std::filesystem::create_directories( folderPath, error );
            return !error && std::filesystem::is_directory( folderPath, error ) && !error;
        }

        bool createMediaProject( core::IApplicationManager *applicationManager,
                                 const json::object &action )
        {
            std::filesystem::path projectPath;
            if( !getMediaActionPath( applicationManager, action, projectPath ) )
            {
                return false;
            }

            const auto defaultProductName = projectPath.filename().string();
            String productName( defaultProductName.data(), defaultProductName.size() );
            String companyName = "Lioncat";
            String currentScenePath;
            String projectTemplate;
            if( !getOptionalString( action, "product_name", productName ) )
            {
                return false;
            }
            if( !getOptionalString( action, "company_name", companyName ) )
            {
                return false;
            }
            if( !getOptionalString( action, "current_scene", currentScenePath ) )
            {
                return false;
            }
            if( !currentScenePath.empty() )
            {
                const auto scenePath = std::filesystem::u8path( toStdString( currentScenePath ) );
                if( scenePath.is_absolute() )
                {
                    return false;
                }
                for( const auto &part : scenePath )
                {
                    if( part == ".." )
                    {
                        return false;
                    }
                }
            }
            if( !getOptionalString( action, "template", projectTemplate ) )
            {
                return false;
            }
            if( !projectTemplate.empty() )
            {
                if( projectTemplate != "hello_world_ui" && projectTemplate != "catch_game_3d" &&
                    projectTemplate != "racing_game_3d" && projectTemplate != "flight_simulator_3d" )
                {
                    return false;
                }
            }

            std::error_code error;
            for( const auto *folder : { "Assets", "Assets/Scenes", "Assets/Scripts", "Cache", "Engine",
                                        "Plugin", "Scripts", "SettingsCache" } )
            {
                std::filesystem::create_directories( projectPath / folder, error );
                if( error )
                {
                    return false;
                }
            }

            const auto projectFile = projectPath / "project.fbproject";
            auto projectFileExists = false;
            error.clear();
            const auto projectFileStatus = std::filesystem::status( projectFile, error );
            if( error == std::errc::no_such_file_or_directory )
            {
                error.clear();
            }
            else if( error )
            {
                return false;
            }
            else if( std::filesystem::is_regular_file( projectFileStatus ) )
            {
                projectFileExists = true;
            }
            else if( std::filesystem::exists( projectFileStatus ) )
            {
                return false;
            }

            if( !projectFileExists )
            {
                json::object project;
                project["projectVersion"] = "1.0.0";
                project["uuid"] = toJsonString( StringUtil::getUUID() );
                project["productName"] = toJsonString( productName );
                project["companyName"] = toJsonString( companyName );
                project["currentScenePath"] = toJsonString( currentScenePath );
                project["applicationType"] = "lua";
                project["applicationFilePath"] = "";
                project["scriptFilePaths"] =
                    !projectTemplate.empty() ? json::array( { "Scripts/Main.lua" } ) : json::array();
                project["resourceFolders"] = json::array( { "Assets" } );
                project["archive"] = false;

                const auto projectContents = json::serialize( project );
                std::ofstream output( projectFile, std::ios::out | std::ios::binary | std::ios::trunc );
                if( !output )
                {
                    return false;
                }
                output.write( projectContents.data(),
                              static_cast<std::streamsize>( projectContents.size() ) );
                output.close();
                if( !output )
                {
                    return false;
                }
            }

            const char *projectScript = nullptr;
            if( projectTemplate == "hello_world_ui" )
            {
                static constexpr auto helloWorldScript =
                    R"(local application = IApplicationManager.instance()
local gameManager = application and application:getGameManager()
local scene = gameManager and gameManager:getCurrentScene()

if gameManager and scene then
    local previous = gameManager:getActorByName("HelloWorld.UI")
    if previous then
        gameManager:destroyActor(previous)
    end

    local actor = gameManager:createActor()
    actor:setName("HelloWorld.UI")
    scene:addActor(actor)

    local layout = actor:addComponent("LayoutTransform")
    if layout then
        layout:setPosition(Vector2F(0.0, 0.0))
        layout:setSize(Vector2F(640.0, 160.0))
        layout:setZOrder(10)
    end

    local text = actor:addComponent("Text")
    if text then
        text:setText("Hello World!")
        text:setColour(ColourF(0.94, 0.98, 1.0, 1.0))
        text:setHorizontalAlignment(1)
        text:setVerticalAlignment(1)
    end

    local material = actor:addComponent("Material")
    if material then
        material:setMaterialPath("DefaultUI.mat")
    end
else
    print("HelloWorld: game scene UI is unavailable")
end
)";
                projectScript = helloWorldScript;
            }
            else if( projectTemplate == "catch_game_3d" )
            {
                static constexpr auto catchGameScript =
                    R"LUA(-- Self-contained 3D catch game generated by the Workphone AI project action.
-- The Mesh components deliberately reference shared engine media assets by name.
class 'CatchGameMain' (BaseComponent)

local function clamp(value, minimum, maximum)
    return math.max(minimum, math.min(maximum, value))
end

function CatchGameMain:__init(component)
    BaseComponent.__init(self, component)
    self.config = {
        halfWidth = 7.0,
        playerY = -4.0,
        playerSpeed = 8.5,
        playerWidth = 2.2,
        itemSize = 0.65,
        itemSpeed = 3.8,
        spawnInterval = 0.75,
        maxMisses = 5,
    }
    self.items = {}
    self.nextItemId = 1
    self.playerX = 0.0
    self.spawnTimer = 0.0
    self.score = 0
    self.misses = 0
    self.gameOver = false
    self.restartWasDown = false
    self.started = false
end

function CatchGameMain:__finalize()
    self:shutdown()
    BaseComponent.__finalize(self)
end

function CatchGameMain:createChild(parent, name)
    local actor = self.gameManager:createActor()
    actor:setName(name)
    parent:addChild(actor)
    return actor
end

function CatchGameMain:createCube(parent, name, x, y, z, sx, sy, sz)
    local actor = self:createChild(parent, name)
    actor:setPosition(Vector3F(x, y, z))
    actor:setScale(Vector3F(sx, sy, sz))

    local mesh = actor:addComponent("Mesh")
    if mesh then
        mesh:setMeshPath("cube_internal.fbmeshbin")
        mesh:load(nil)
    end

    local material = actor:addComponent("Material")
    if material then
        material:setMaterialPath("Standard.mat")
    end

    local renderer = actor:addComponent("MeshRenderer")
    if renderer then
        renderer:load(nil)
        renderer:updateMaterials()
    end
    return actor
end

function CatchGameMain:createText(parent, name, value, x, y, width, height)
    local actor = self:createChild(parent, name)
    local layout = actor:addComponent("LayoutTransform")
    if layout then
        layout:setPosition(Vector2F(x, y))
        layout:setSize(Vector2F(width, height))
        layout:setZOrder(20)
    end
    local text = actor:addComponent("Text")
    if text then
        text:setText(value)
        text:setColour(ColourF(0.94, 0.98, 1.0, 1.0))
        text:setHorizontalAlignment(1)
        text:setVerticalAlignment(1)
    end
    local material = actor:addComponent("Material")
    if material then
        material:setMaterialPath("DefaultUI.mat")
    end
    return text
end

function CatchGameMain:start()
    if self.started then
        return true
    end

    local application = IApplicationManager.instance()
    self.gameManager = application and application:getGameManager()
    self.input = application and application:getInput()
    self.inputDevices = application and application:getInputDeviceManager()
    self.timer = application and application:getTimer()
    local owner = self:getActor()
    if not self.gameManager or not owner then
        print("CatchGameMain: game manager or owner actor is unavailable")
        return false
    end

    self.visualRoot = self:createChild(owner, "CatchGame3D.World")
    self:createCube(self.visualRoot, "Arena.Floor", 0.0, 0.5, 1.5, 8.0, 10.0, 0.15)
    self:createCube(self.visualRoot, "Arena.LeftWall", -7.75, 0.5, 0.0, 0.25, 10.0, 0.5)
    self:createCube(self.visualRoot, "Arena.RightWall", 7.75, 0.5, 0.0, 0.25, 10.0, 0.5)
    self.playerActor = self:createCube(
        self.visualRoot, "Player.Catcher", self.playerX, self.config.playerY, 0.0,
        self.config.playerWidth, 0.45, 0.65)
    self.hudText =
        self:createText(self.visualRoot, "CatchGame3D.HUD", "", 0.0, -310.0, 700.0, 55.0)
    self.messageText = self:createText(
        self.visualRoot, "CatchGame3D.Message", "Move with A/D or Left/Right",
        0.0, 310.0, 760.0, 55.0)
    self.started = true
    self:updateHud()
    return true
end

function CatchGameMain:shutdown()
    if self.gameManager and self.visualRoot then
        self.gameManager:destroyActor(self.visualRoot)
    end
    self.items = {}
    self.visualRoot = nil
    self.playerActor = nil
    self.hudText = nil
    self.messageText = nil
    self.started = false
end

function CatchGameMain:isKeyDown(code)
    return self.inputDevices and code ~= nil and self.inputDevices:isKeyPressed(code)
end

function CatchGameMain:readHorizontalInput()
    local axis = self.input and self.input:getAxisValue(0) or 0.0
    local left = self:isKeyDown(KeyCode and KeyCode.Left)
        or self:isKeyDown(KeyCode and KeyCode.A)
    local right = self:isKeyDown(KeyCode and KeyCode.Right)
        or self:isKeyDown(KeyCode and KeyCode.D)
    if left ~= right then
        axis = left and -1.0 or 1.0
    end
    return clamp(tonumber(axis) or 0.0, -1.0, 1.0)
end

function CatchGameMain:isRestartDown()
    return self:isKeyDown(KeyCode and KeyCode.R)
        or self:isKeyDown(KeyCode and KeyCode.Return)
end

function CatchGameMain:updateHud()
    if self.hudText then
        self.hudText:setText(
            "Score: " .. tostring(self.score)
            .. "    Misses: " .. tostring(self.misses)
            .. "/" .. tostring(self.config.maxMisses))
    end
end

function CatchGameMain:spawnItem()
    local id = self.nextItemId
    self.nextItemId = id + 1
    local range = self.config.halfWidth - self.config.itemSize
    local x = -range + (((id * 37) % 101) / 100.0) * range * 2.0
    local item = {
        id = id,
        x = x,
        y = 6.0,
    }
    item.actor = self:createCube(
        self.visualRoot, "FallingCube." .. tostring(id), item.x, item.y, 0.0,
        self.config.itemSize, self.config.itemSize, self.config.itemSize)
    table.insert(self.items, item)
end

function CatchGameMain:removeItem(index)
    local item = self.items[index]
    if item and self.gameManager and item.actor then
        self.gameManager:destroyActor(item.actor)
    end
    table.remove(self.items, index)
end

function CatchGameMain:isCaught(item)
    local horizontalDistance = math.abs(item.x - self.playerX)
    local catchWidth = (self.config.playerWidth + self.config.itemSize) * 0.5
    return item.y <= self.config.playerY + 0.55
        and item.y >= self.config.playerY - 0.55
        and horizontalDistance <= catchWidth
end

function CatchGameMain:reset()
    for index = #self.items, 1, -1 do
        self:removeItem(index)
    end
    self.nextItemId = 1
    self.playerX = 0.0
    self.spawnTimer = 0.0
    self.score = 0
    self.misses = 0
    self.gameOver = false
    if self.playerActor then
        self.playerActor:setPosition(Vector3F(self.playerX, self.config.playerY, 0.0))
    end
    if self.messageText then
        self.messageText:setText("Move with A/D or Left/Right")
    end
    self:updateHud()
end

function CatchGameMain:update()
    if not self.started and not self:start() then
        return
    end

    local restartDown = self:isRestartDown()
    if restartDown and not self.restartWasDown then
        self:reset()
    end
    self.restartWasDown = restartDown
    if self.gameOver then
        return
    end

    local deltaTime = self.timer and self.timer:getDeltaTime() or (1.0 / 60.0)
    deltaTime = clamp(tonumber(deltaTime) or 0.0, 0.0, 0.1)
    self.playerX = clamp(
        self.playerX + self:readHorizontalInput() * self.config.playerSpeed * deltaTime,
        -self.config.halfWidth + self.config.playerWidth * 0.5,
        self.config.halfWidth - self.config.playerWidth * 0.5)
    if self.playerActor then
        self.playerActor:setPosition(Vector3F(self.playerX, self.config.playerY, 0.0))
    end

    self.spawnTimer = self.spawnTimer + deltaTime
    while self.spawnTimer >= self.config.spawnInterval do
        self.spawnTimer = self.spawnTimer - self.config.spawnInterval
        self:spawnItem()
    end

    local hudChanged = false
    for index = #self.items, 1, -1 do
        local item = self.items[index]
        item.y = item.y - self.config.itemSpeed * deltaTime
        if item.actor then
            item.actor:setPosition(Vector3F(item.x, item.y, 0.0))
        end
        if self:isCaught(item) then
            self.score = self.score + 1
            self:removeItem(index)
            hudChanged = true
        elseif item.y < -5.5 then
            self.misses = self.misses + 1
            self:removeItem(index)
            hudChanged = true
        end
    end

    if self.misses >= self.config.maxMisses then
        self.gameOver = true
        if self.messageText then
            self.messageText:setText("Game over - press R or Enter to restart")
        end
    end
    if hudChanged then
        self:updateHud()
    end
end

function launchCatchGame3D()
    local application = IApplicationManager.instance()
    local gameManager = application and application:getGameManager()
    local scene = gameManager and gameManager:getCurrentScene()
    if not gameManager or not scene then
        print("launchCatchGame3D: game scene is unavailable")
        return nil
    end

    local previous = gameManager:getActorByName("CatchGame3D")
    if previous then
        gameManager:destroyActor(previous)
    end

    local actor = gameManager:createActor()
    actor:setName("CatchGame3D")
    scene:addActor(actor)
    local component = actor:addComponentById(UserComponent.typeInfo())
    if not component then
        gameManager:destroyActor(actor)
        print("launchCatchGame3D: failed to create UserComponent")
        return nil
    end
    component:setClassName("CatchGameMain")
    return actor
end

launchCatchGame3D()
)LUA";
                projectScript = catchGameScript;
            }
            else if( projectTemplate == "racing_game_3d" )
            {
                static constexpr auto racingGameScript =
                    R"LUA(-- Self-contained arcade racing game generated by the Workphone AI project action.
-- Every world object uses cube_internal.fbmeshbin from the shared engine media folder.
class 'RacingGameMain' (BaseComponent)

local function clamp(value, minimum, maximum)
    return math.max(minimum, math.min(maximum, value))
end

function RacingGameMain:__init(component)
    BaseComponent.__init(self, component)
    self.config = {
        roadHalfWidth = 6.0,
        playerY = -4.5,
        playerWidth = 1.15,
        playerHeight = 1.8,
        steeringSpeed = 7.5,
        baseSpeed = 5.0,
        maximumSpeed = 12.0,
        accelerationPerDistance = 0.025,
        trafficWidth = 1.1,
        trafficHeight = 1.8,
        baseSpawnInterval = 1.15,
        minimumSpawnInterval = 0.55,
        startingLives = 3,
    }
    self.traffic = {}
    self.roadMarkers = {}
    self.nextTrafficId = 1
    self.playerX = 0.0
    self.speed = self.config.baseSpeed
    self.distance = 0.0
    self.bestDistance = 0.0
    self.overtakes = 0
    self.lives = self.config.startingLives
    self.spawnTimer = 0.0
    self.restartWasDown = false
    self.gameOver = false
    self.started = false
end

function RacingGameMain:__finalize()
    self:shutdown()
    BaseComponent.__finalize(self)
end

function RacingGameMain:createChild(parent, name)
    local actor = self.gameManager:createActor()
    actor:setName(name)
    parent:addChild(actor)
    return actor
end

function RacingGameMain:createCube(parent, name, x, y, z, sx, sy, sz)
    local actor = self:createChild(parent, name)
    actor:setPosition(Vector3F(x, y, z))
    actor:setScale(Vector3F(sx, sy, sz))

    local mesh = actor:addComponent("Mesh")
    if mesh then
        mesh:setMeshPath("cube_internal.fbmeshbin")
        mesh:load(nil)
    end

    local material = actor:addComponent("Material")
    if material then
        material:setMaterialPath("Standard.mat")
    end

    local renderer = actor:addComponent("MeshRenderer")
    if renderer then
        renderer:load(nil)
        renderer:updateMaterials()
    end
    return actor
end

function RacingGameMain:createText(parent, name, value, x, y, width, height)
    local actor = self:createChild(parent, name)
    local layout = actor:addComponent("LayoutTransform")
    if layout then
        layout:setPosition(Vector2F(x, y))
        layout:setSize(Vector2F(width, height))
        layout:setZOrder(20)
    end
    local text = actor:addComponent("Text")
    if text then
        text:setText(value)
        text:setColour(ColourF(0.94, 0.98, 1.0, 1.0))
        text:setHorizontalAlignment(1)
        text:setVerticalAlignment(1)
    end
    local material = actor:addComponent("Material")
    if material then
        material:setMaterialPath("DefaultUI.mat")
    end
    return text
end

function RacingGameMain:createRoad()
    self:createCube(self.worldRoot, "Road.Surface", 0.0, 0.5, 1.5, 7.0, 11.0, 0.15)
    self:createCube(self.worldRoot, "Road.LeftBarrier", -6.75, 0.5, 0.0, 0.2, 11.0, 0.65)
    self:createCube(self.worldRoot, "Road.RightBarrier", 6.75, 0.5, 0.0, 0.2, 11.0, 0.65)

    for lane = 1, 2 do
        local x = lane == 1 and -2.0 or 2.0
        for row = 1, 5 do
            local marker = {
                x = x,
                y = -6.0 + row * 3.0,
            }
            marker.actor = self:createCube(
                self.worldRoot,
                "Road.Marker." .. tostring(lane) .. "." .. tostring(row),
                marker.x, marker.y, 0.6, 0.10, 0.8, 0.08)
            table.insert(self.roadMarkers, marker)
        end
    end
end

function RacingGameMain:start()
    if self.started then
        return true
    end

    local application = IApplicationManager.instance()
    self.gameManager = application and application:getGameManager()
    self.input = application and application:getInput()
    self.inputDevices = application and application:getInputDeviceManager()
    self.timer = application and application:getTimer()
    local owner = self:getActor()
    if not self.gameManager or not owner then
        print("RacingGameMain: game manager or owner actor is unavailable")
        return false
    end

    self.worldRoot = self:createChild(owner, "CubeRacer.World")
    self:createRoad()
    self.playerActor = self:createCube(
        self.worldRoot, "Player.Car", self.playerX, self.config.playerY, 0.0,
        self.config.playerWidth, self.config.playerHeight, 0.65)
    self.hudText =
        self:createText(self.worldRoot, "CubeRacer.HUD", "", 0.0, -310.0, 820.0, 55.0)
    self.messageText = self:createText(
        self.worldRoot, "CubeRacer.Message", "Steer with A/D or Left/Right",
        0.0, 310.0, 820.0, 55.0)
    self.started = true
    self:updateHud()
    return true
end

function RacingGameMain:shutdown()
    if self.gameManager and self.worldRoot then
        self.gameManager:destroyActor(self.worldRoot)
    end
    self.traffic = {}
    self.roadMarkers = {}
    self.worldRoot = nil
    self.playerActor = nil
    self.hudText = nil
    self.messageText = nil
    self.started = false
end

function RacingGameMain:isKeyDown(code)
    return self.inputDevices and code ~= nil and self.inputDevices:isKeyPressed(code)
end

function RacingGameMain:readSteering()
    local axis = self.input and self.input:getAxisValue(0) or 0.0
    local left = self:isKeyDown(KeyCode and KeyCode.Left)
        or self:isKeyDown(KeyCode and KeyCode.A)
    local right = self:isKeyDown(KeyCode and KeyCode.Right)
        or self:isKeyDown(KeyCode and KeyCode.D)
    if left ~= right then
        axis = left and -1.0 or 1.0
    end
    return clamp(tonumber(axis) or 0.0, -1.0, 1.0)
end

function RacingGameMain:isRestartDown()
    return self:isKeyDown(KeyCode and KeyCode.R)
        or self:isKeyDown(KeyCode and KeyCode.Return)
end

function RacingGameMain:updateHud()
    if self.hudText then
        self.hudText:setText(
            "Distance: " .. tostring(math.floor(self.distance))
            .. "m    Speed: " .. tostring(math.floor(self.speed * 20.0))
            .. "    Overtakes: " .. tostring(self.overtakes)
            .. "    Lives: " .. tostring(self.lives))
    end
end

function RacingGameMain:getSpawnInterval()
    local speedGain = self.speed - self.config.baseSpeed
    return clamp(
        self.config.baseSpawnInterval - speedGain * 0.06,
        self.config.minimumSpawnInterval,
        self.config.baseSpawnInterval)
end

function RacingGameMain:spawnTraffic()
    local id = self.nextTrafficId
    self.nextTrafficId = id + 1
    local lanes = { -4.0, 0.0, 4.0 }
    local laneIndex = ((id * 2 + math.floor(self.distance / 25.0)) % #lanes) + 1
    local vehicle = {
        id = id,
        x = lanes[laneIndex],
        y = 7.0,
    }
    vehicle.actor = self:createCube(
        self.worldRoot, "Traffic.Car." .. tostring(id), vehicle.x, vehicle.y, 0.0,
        self.config.trafficWidth, self.config.trafficHeight, 0.65)
    table.insert(self.traffic, vehicle)
end

function RacingGameMain:removeTraffic(index)
    local vehicle = self.traffic[index]
    if vehicle and self.gameManager and vehicle.actor then
        self.gameManager:destroyActor(vehicle.actor)
    end
    table.remove(self.traffic, index)
end

function RacingGameMain:hasCollision(vehicle)
    local horizontalLimit =
        (self.config.playerWidth + self.config.trafficWidth) * 0.5
    local verticalLimit =
        (self.config.playerHeight + self.config.trafficHeight) * 0.5
    return math.abs(vehicle.x - self.playerX) <= horizontalLimit
        and math.abs(vehicle.y - self.config.playerY) <= verticalLimit
end

function RacingGameMain:updateRoad(deltaTime)
    for _, marker in ipairs(self.roadMarkers) do
        marker.y = marker.y - self.speed * deltaTime
        if marker.y < -6.5 then
            marker.y = marker.y + 15.0
        end
        if marker.actor then
            marker.actor:setPosition(Vector3F(marker.x, marker.y, 0.6))
        end
    end
end

function RacingGameMain:reset()
    for index = #self.traffic, 1, -1 do
        self:removeTraffic(index)
    end
    self.nextTrafficId = 1
    self.playerX = 0.0
    self.speed = self.config.baseSpeed
    self.distance = 0.0
    self.overtakes = 0
    self.lives = self.config.startingLives
    self.spawnTimer = 0.0
    self.gameOver = false
    if self.playerActor then
        self.playerActor:setPosition(Vector3F(self.playerX, self.config.playerY, 0.0))
    end
    if self.messageText then
        self.messageText:setText("Steer with A/D or Left/Right")
    end
    self:updateHud()
end

function RacingGameMain:update()
    if not self.started and not self:start() then
        return
    end

    local restartDown = self:isRestartDown()
    if restartDown and not self.restartWasDown then
        self:reset()
    end
    self.restartWasDown = restartDown
    if self.gameOver then
        return
    end

    local deltaTime = self.timer and self.timer:getDeltaTime() or (1.0 / 60.0)
    deltaTime = clamp(tonumber(deltaTime) or 0.0, 0.0, 0.1)
    local playerLimit = self.config.roadHalfWidth - self.config.playerWidth * 0.5
    self.playerX = clamp(
        self.playerX + self:readSteering() * self.config.steeringSpeed * deltaTime,
        -playerLimit, playerLimit)
    if self.playerActor then
        self.playerActor:setPosition(Vector3F(self.playerX, self.config.playerY, 0.0))
    end

    self.distance = self.distance + self.speed * deltaTime
    self.bestDistance = math.max(self.bestDistance, self.distance)
    self.speed = clamp(
        self.config.baseSpeed + self.distance * self.config.accelerationPerDistance,
        self.config.baseSpeed, self.config.maximumSpeed)
    self:updateRoad(deltaTime)

    self.spawnTimer = self.spawnTimer + deltaTime
    local spawnInterval = self:getSpawnInterval()
    while self.spawnTimer >= spawnInterval do
        self.spawnTimer = self.spawnTimer - spawnInterval
        self:spawnTraffic()
        spawnInterval = self:getSpawnInterval()
    end

    for index = #self.traffic, 1, -1 do
        local vehicle = self.traffic[index]
        vehicle.y = vehicle.y - self.speed * deltaTime
        if vehicle.actor then
            vehicle.actor:setPosition(Vector3F(vehicle.x, vehicle.y, 0.0))
        end
        if self:hasCollision(vehicle) then
            self.lives = self.lives - 1
            self.speed = math.max(self.config.baseSpeed, self.speed * 0.75)
            self:removeTraffic(index)
        elseif vehicle.y < -6.5 then
            self.overtakes = self.overtakes + 1
            self:removeTraffic(index)
        end
    end

    if self.lives <= 0 then
        self.gameOver = true
        if self.messageText then
            self.messageText:setText(
                "Race over - distance " .. tostring(math.floor(self.distance))
                .. "m. Press R or Enter to restart")
        end
    end
    self:updateHud()
end

function launchCubeRacer()
    local application = IApplicationManager.instance()
    local gameManager = application and application:getGameManager()
    local scene = gameManager and gameManager:getCurrentScene()
    if not gameManager or not scene then
        print("launchCubeRacer: game scene is unavailable")
        return nil
    end

    local previous = gameManager:getActorByName("CubeRacer")
    if previous then
        gameManager:destroyActor(previous)
    end

    local actor = gameManager:createActor()
    actor:setName("CubeRacer")
    scene:addActor(actor)
    local component = actor:addComponentById(UserComponent.typeInfo())
    if not component then
        gameManager:destroyActor(actor)
        print("launchCubeRacer: failed to create UserComponent")
        return nil
    end
    component:setClassName("RacingGameMain")
    return actor
end

launchCubeRacer()
)LUA";
                projectScript = racingGameScript;
            }
            else if( projectTemplate == "flight_simulator_3d" )
            {
                static constexpr auto flightSimulatorScript =
                    R"LUA(-- Self-contained flight simulator generated by the Workphone AI project action.
-- The airframe and course use cube_internal.fbmeshbin from the shared engine media folder.
-- VehicleController type 1 delegates flight forces to WPVehiclePhysics aerodynamics.
class 'FlightSimulatorMain' (BaseComponent)

local function clamp(value, minimum, maximum)
    return math.max(minimum, math.min(maximum, value))
end

local function vectorLength(value)
    if not value then
        return 0.0
    end
    return math.sqrt(value.x * value.x + value.y * value.y + value.z * value.z)
end

function FlightSimulatorMain:__init(component)
    BaseComponent.__init(self, component)
    self.config = {
        mass = 950.0,
        airDensity = 1.225,
        aerodynamicArea = 17.5,
        rollDamping = 1.4,
        throttleRate = 0.35,
        spawn = Vector3F(0.0, 18.0, 20.0),
        initialSpeed = 38.0,
        checkpointRadius = 13.0,
        crashAltitude = -8.0,
        stallSpeed = 15.0,
    }
    self.checkpoints = {
        { x = 0.0, y = 20.0, z = -90.0 },
        { x = 28.0, y = 32.0, z = -190.0 },
        { x = -32.0, y = 24.0, z = -300.0 },
        { x = 0.0, y = 42.0, z = -420.0 },
    }
    self.throttle = 0.72
    self.pitch = 0.0
    self.roll = 0.0
    self.yaw = 0.0
    self.checkpointIndex = 1
    self.checkpointsPassed = 0
    self.laps = 0
    self.elapsed = 0.0
    self.crashed = false
    self.restartWasDown = false
    self.started = false
end

function FlightSimulatorMain:__finalize()
    self:shutdown()
    BaseComponent.__finalize(self)
end

function FlightSimulatorMain:createChild(parent, name)
    local actor = self.gameManager:createActor()
    actor:setName(name)
    parent:addChild(actor)
    return actor
end

function FlightSimulatorMain:createCube(parent, name, x, y, z, sx, sy, sz)
    local actor = self:createChild(parent, name)
    actor:setPosition(Vector3F(x, y, z))
    actor:setScale(Vector3F(sx, sy, sz))

    local mesh = actor:addComponent("Mesh")
    if mesh then
        mesh:setMeshPath("cube_internal.fbmeshbin")
        mesh:load(nil)
    end
    local material = actor:addComponent("Material")
    if material then
        material:setMaterialPath("Standard.mat")
    end
    local renderer = actor:addComponent("MeshRenderer")
    if renderer then
        renderer:load(nil)
        renderer:updateMaterials()
    end
    return actor
end

function FlightSimulatorMain:createText(parent, name, value, x, y, width, height, zOrder)
    local actor = self:createChild(parent, name)
    local layout = actor:addComponent("LayoutTransform")
    if layout then
        layout:setPosition(Vector2F(x, y))
        layout:setSize(Vector2F(width, height))
        layout:setZOrder(zOrder or 30)
    end
    local text = actor:addComponent("Text")
    if text then
        text:setText(value)
        text:setColour(ColourF(0.90, 0.97, 1.0, 1.0))
        text:setHorizontalAlignment(1)
        text:setVerticalAlignment(1)
    end
    local material = actor:addComponent("Material")
    if material then
        material:setMaterialPath("DefaultUI.mat")
    end
    return text
end

function FlightSimulatorMain:createCourse()
    self:createCube(self.worldRoot, "Runway", 0.0, -0.5, -50.0, 12.0, 0.25, 90.0)
    for index, checkpoint in ipairs(self.checkpoints) do
        local prefix = "Checkpoint." .. tostring(index)
        self:createCube(
            self.worldRoot, prefix .. ".Left",
            checkpoint.x - 9.0, checkpoint.y, checkpoint.z, 0.35, 5.0, 0.35)
        self:createCube(
            self.worldRoot, prefix .. ".Right",
            checkpoint.x + 9.0, checkpoint.y, checkpoint.z, 0.35, 5.0, 0.35)
        self:createCube(
            self.worldRoot, prefix .. ".Top",
            checkpoint.x, checkpoint.y + 5.0, checkpoint.z, 9.0, 0.35, 0.35)
        self:createCube(
            self.worldRoot, prefix .. ".Bottom",
            checkpoint.x, checkpoint.y - 5.0, checkpoint.z, 9.0, 0.35, 0.35)
    end
end

function FlightSimulatorMain:createAircraft()
    local spawn = self.config.spawn
    self.aircraftActor = self:createChild(self.worldRoot, "Player.Aircraft")
    self.aircraftActor:setPosition(spawn)

    local collision = self.aircraftActor:addComponent("CollisionBox")
    if collision then
        collision:setExtents(Vector3F(4.8, 0.8, 3.5))
    end

    self.rigidbody = self.aircraftActor:addComponent("Rigidbody")
    if self.rigidbody then
        self.rigidbody:setMass(self.config.mass)
        self.rigidbody:setMassSpaceInertiaTensor(Vector3F(1500.0, 2300.0, 2700.0))
        self.rigidbody:setMaxLinearVelocity(220.0)
        self.rigidbody:setMaxAngularVelocity(4.0)
    end

    self.vehicleController = self.aircraftActor:addComponent("VehicleController")
    if self.vehicleController then
        self.vehicleController:setMass(self.config.mass)
        self.vehicleController:setMOI(Vector3F(1500.0, 2300.0, 2700.0))
        self.vehicleController:setAirDensity(self.config.airDensity)
        self.vehicleController:setAerodynamicSectionMultiplier(
            self.config.aerodynamicArea)
        self.vehicleController:setRollwiseDamping(self.config.rollDamping)
        self.vehicleController:setVehicleType(1)
        self.vehicleController:setAircraftControls(self.throttle, 0.0, 0.0, 0.0)
    end

    self:createCube(
        self.aircraftActor, "Airframe.Fuselage", 0.0, 0.0, 0.0, 0.7, 0.6, 3.2)
    self:createCube(
        self.aircraftActor, "Airframe.MainWing", 0.0, 0.0, -0.3, 5.2, 0.12, 1.0)
    self:createCube(
        self.aircraftActor, "Airframe.HorizontalTail", 0.0, 0.25, 2.55, 2.0, 0.10, 0.55)
    self:createCube(
        self.aircraftActor, "Airframe.VerticalTail", 0.0, 0.75, 2.55, 0.12, 0.85, 0.55)
    self:createCube(
        self.aircraftActor, "Airframe.Cockpit", 0.0, 0.55, -1.0, 0.45, 0.35, 0.8)
end

function FlightSimulatorMain:start()
    if self.started then
        return true
    end

    local application = IApplicationManager.instance()
    self.gameManager = application and application:getGameManager()
    self.inputDevices = application and application:getInputDeviceManager()
    self.timer = application and application:getTimer()
    local owner = self:getActor()
    if not self.gameManager or not owner then
        print("FlightSimulatorMain: game manager or owner actor is unavailable")
        return false
    end

    self.worldRoot = self:createChild(owner, "FlightSimulator.World")
    self:createCourse()
    self:createAircraft()
    self.hudText = self:createText(
        owner, "FlightSimulator.HUD", "", 0.0, -300.0, 1050.0, 70.0, 40)
    self.statusText = self:createText(
        owner, "FlightSimulator.Status",
        "W/S throttle | arrows pitch | A/D roll | Q/E yaw | R restart",
        0.0, 300.0, 1100.0, 60.0, 40)

    self.started = true
    self:resetFlight()
    return true
end

function FlightSimulatorMain:shutdown()
    if self.gameManager and self.worldRoot then
        self.gameManager:destroyActor(self.worldRoot)
    end
    if self.gameManager and self.hudText then
        local hudActor = self.gameManager:getActorByName("FlightSimulator.HUD")
        if hudActor then
            self.gameManager:destroyActor(hudActor)
        end
    end
    if self.gameManager and self.statusText then
        local statusActor = self.gameManager:getActorByName("FlightSimulator.Status")
        if statusActor then
            self.gameManager:destroyActor(statusActor)
        end
    end
    self.worldRoot = nil
    self.aircraftActor = nil
    self.rigidbody = nil
    self.vehicleController = nil
    self.hudText = nil
    self.statusText = nil
    self.started = false
end

function FlightSimulatorMain:isKeyDown(code)
    return self.inputDevices and code ~= nil and self.inputDevices:isKeyPressed(code)
end

function FlightSimulatorMain:readAxis(negativeKey, positiveKey)
    local negative = self:isKeyDown(negativeKey)
    local positive = self:isKeyDown(positiveKey)
    if negative == positive then
        return 0.0
    end
    return negative and -1.0 or 1.0
end

function FlightSimulatorMain:readControls(deltaTime)
    local throttleAxis = self:readAxis(
        KeyCode and KeyCode.S, KeyCode and KeyCode.W)
    self.throttle = clamp(
        self.throttle + throttleAxis * self.config.throttleRate * deltaTime,
        0.0, 1.0)
    self.pitch = self:readAxis(
        KeyCode and KeyCode.Down, KeyCode and KeyCode.Up)
    self.roll = self:readAxis(
        KeyCode and KeyCode.A, KeyCode and KeyCode.D)
    self.yaw = self:readAxis(
        KeyCode and KeyCode.Q, KeyCode and KeyCode.E)
end

function FlightSimulatorMain:resetFlight()
    self.throttle = 0.72
    self.pitch = 0.0
    self.roll = 0.0
    self.yaw = 0.0
    self.checkpointIndex = 1
    self.checkpointsPassed = 0
    self.laps = 0
    self.elapsed = 0.0
    self.crashed = false

    if self.aircraftActor then
        self.aircraftActor:setPosition(self.config.spawn)
        self.aircraftActor:setOrientation(Quaternion())
    end
    if self.rigidbody then
        self.rigidbody:setLinearVelocity(Vector3F(0.0, 0.0, -self.config.initialSpeed))
        self.rigidbody:setAngularVelocity(Vector3F(0.0, 0.0, 0.0))
    end
    if self.vehicleController then
        self.vehicleController:setAircraftControls(self.throttle, 0.0, 0.0, 0.0)
    end
    if self.statusText then
        self.statusText:setText(
            "W/S throttle | arrows pitch | A/D roll | Q/E yaw | R restart")
    end
    self:updateHud()
end

function FlightSimulatorMain:updateCheckpoint(position)
    local checkpoint = self.checkpoints[self.checkpointIndex]
    if not checkpoint then
        return
    end

    local dx = position.x - checkpoint.x
    local dy = position.y - checkpoint.y
    local dz = position.z - checkpoint.z
    local radius = self.config.checkpointRadius
    if dx * dx + dy * dy + dz * dz <= radius * radius then
        self.checkpointsPassed = self.checkpointsPassed + 1
        self.checkpointIndex = self.checkpointIndex + 1
        if self.checkpointIndex > #self.checkpoints then
            self.checkpointIndex = 1
            self.laps = self.laps + 1
        end
        if self.statusText then
            self.statusText:setText(
                "Checkpoint cleared! Next gate " .. tostring(self.checkpointIndex))
        end
    end
end

function FlightSimulatorMain:updateHud()
    if not self.hudText or not self.aircraftActor then
        return
    end
    local position = self.aircraftActor:getPosition()
    local velocity = self.rigidbody and self.rigidbody:getLinearVelocity() or Vector3F()
    local speed = vectorLength(velocity)
    self.hudText:setText(
        "ALT " .. tostring(math.floor(position.y)) .. "m"
        .. "   IAS " .. tostring(math.floor(speed * 3.6)) .. "km/h"
        .. "   THR " .. tostring(math.floor(self.throttle * 100.0)) .. "%"
        .. "   GATE " .. tostring(self.checkpointIndex) .. "/" .. tostring(#self.checkpoints)
        .. "   LAPS " .. tostring(self.laps)
        .. "   TIME " .. tostring(math.floor(self.elapsed)) .. "s")
end

function FlightSimulatorMain:update()
    if not self.started and not self:start() then
        return
    end

    local restartDown = self:isKeyDown(KeyCode and KeyCode.R)
    if restartDown and not self.restartWasDown then
        self:resetFlight()
    end
    self.restartWasDown = restartDown
    if self.crashed then
        return
    end

    local deltaTime = self.timer and self.timer:getDeltaTime() or (1.0 / 60.0)
    deltaTime = clamp(tonumber(deltaTime) or 0.0, 0.0, 0.1)
    self.elapsed = self.elapsed + deltaTime
    self:readControls(deltaTime)

    if self.vehicleController then
        self.vehicleController:setAircraftControls(
            self.throttle, self.pitch, self.roll, self.yaw)
    end

    local position = self.aircraftActor:getPosition()
    local velocity = self.rigidbody and self.rigidbody:getLinearVelocity() or Vector3F()
    local speed = vectorLength(velocity)
    self:updateCheckpoint(position)

    if position.y < self.config.crashAltitude then
        self.crashed = true
        self.throttle = 0.0
        if self.vehicleController then
            self.vehicleController:setAircraftControls(0.0, 0.0, 0.0, 0.0)
        end
        if self.statusText then
            self.statusText:setText("Aircraft down - press R to restart the flight")
        end
    elseif speed < self.config.stallSpeed and self.statusText then
        self.statusText:setText("STALL WARNING - lower the nose and add throttle")
    end

    self:updateHud()
end

function launchFlightSimulator()
    local application = IApplicationManager.instance()
    local gameManager = application and application:getGameManager()
    local scene = gameManager and gameManager:getCurrentScene()
    if not gameManager or not scene then
        print("launchFlightSimulator: game scene is unavailable")
        return nil
    end

    local previous = gameManager:getActorByName("FlightSimulator")
    if previous then
        gameManager:destroyActor(previous)
    end

    local actor = gameManager:createActor()
    actor:setName("FlightSimulator")
    scene:addActor(actor)
    local component = actor:addComponentById(UserComponent.typeInfo())
    if not component then
        gameManager:destroyActor(actor)
        print("launchFlightSimulator: failed to create UserComponent")
        return nil
    end
    component:setClassName("FlightSimulatorMain")
    return actor
end

launchFlightSimulator()
)LUA";
                projectScript = flightSimulatorScript;
            }

            if( projectScript )
            {
                const auto scriptFile = projectPath / "Scripts" / "Main.lua";
                std::ofstream scriptOutput( scriptFile,
                                            std::ios::out | std::ios::binary | std::ios::trunc );
                if( !scriptOutput )
                {
                    return false;
                }
                scriptOutput.write(
                    projectScript,
                    static_cast<std::streamsize>( std::char_traits<char>::length( projectScript ) ) );
                scriptOutput.close();
                if( !scriptOutput )
                {
                    return false;
                }
            }

            error.clear();
            return std::filesystem::is_regular_file( projectFile, error ) && !error &&
                   std::filesystem::file_size( projectFile, error ) > 0 && !error;
        }

        SmartPtr<scene::IGameActor> findActor( core::IApplicationManager *applicationManager,
                                               const json::object &action )
        {
            String actorName;
            if( !applicationManager || !getString( action, "actor", actorName ) )
            {
                return nullptr;
            }

            const auto gameManager = applicationManager->getGameManager();
            const auto scene = gameManager ? gameManager->getCurrentScene() : nullptr;
            return scene ? scene->findActorByName( actorName ) : nullptr;
        }

        bool applyOptionalActorProperties( const json::object &action,
                                           SmartPtr<scene::IGameActor> actor )
        {
            if( !actor )
            {
                return false;
            }

            bool value = false;
            if( getBool( action, "enabled", value ) )
            {
                actor->setEnabled( value );
            }
            if( getBool( action, "visible", value ) )
            {
                actor->setVisible( value );
            }
            if( getBool( action, "static", value ) )
            {
                actor->setStatic( value );
            }
            if( getBool( action, "smooth_motion", value ) )
            {
                actor->setSmoothMotion( value );
            }

            if( auto transform = actor->getTransform() )
            {
                Vector3<real_Num> vector;
                if( getVector3( action, "position", vector ) )
                {
                    transform->setPosition( vector );
                }
                if( getVector3( action, "rotation", vector ) )
                {
                    transform->setRotation( vector );
                }
                if( getVector3( action, "scale", vector ) && std::abs( vector.x ) > 0.000001 &&
                    std::abs( vector.y ) > 0.000001 && std::abs( vector.z ) > 0.000001 )
                {
                    transform->setScale( vector );
                }
            }

            return true;
        }

        bool applyControlAction( core::IApplicationManager *applicationManager,
                                 const json::object &action )
        {
            json::string_view name;
            if( !applicationManager || !getActionName( action, name ) )
            {
                return false;
            }

            if( name == "play" )
            {
                applicationManager->setPaused( false );
                applicationManager->setPlaying( true );
                return true;
            }
            if( name == "stop" )
            {
                applicationManager->setPaused( false );
                applicationManager->setPlaying( false );
                return true;
            }
            if( name == "pause" )
            {
                applicationManager->setPaused( true );
                return true;
            }
            if( name == "resume" )
            {
                applicationManager->setPaused( false );
                return true;
            }

            bool value = false;
            if( name == "set_playing" && getActionBool( action, value ) )
            {
                applicationManager->setPlaying( value );
                return true;
            }
            if( name == "set_paused" && getActionBool( action, value ) )
            {
                applicationManager->setPaused( value );
                return true;
            }
            if( name == "set_running" && getActionBool( action, value ) )
            {
                if( !value && !isConfirmed( action ) )
                {
                    return false;
                }

                applicationManager->setRunning( value );
                return true;
            }
            if( name == "set_renderer_enabled" && getActionBool( action, value ) )
            {
                applicationManager->setEnableRenderer( value );
                return true;
            }
            if( name == "set_editor_mode" && getActionBool( action, value ) )
            {
                applicationManager->setEditor( value );
                return true;
            }
            if( name == "set_editor_camera" && getActionBool( action, value ) )
            {
                applicationManager->setEditorCamera( value );
                return true;
            }
            if( name == "set_pause_menu_active" && getActionBool( action, value ) )
            {
                applicationManager->setPauseMenuActive( value );
                return true;
            }
            if( name == "request_quit" )
            {
                if( !isConfirmed( action ) )
                {
                    return false;
                }

                applicationManager->setQuit( true );
                return true;
            }

            if( name == "set_master_volume" )
            {
                real_Num volume = 0;
                auto soundManager = applicationManager->getSoundManager();
                if( !soundManager || !getNumber( action, "value", volume ) || volume < 0.0 ||
                    volume > 1.0 )
                {
                    return false;
                }

                soundManager->setVolume( static_cast<f32>( volume ) );
                return true;
            }
            if( name == "set_fixed_timestep" )
            {
                real_Num fixedTimestep = 0;
                auto timer = applicationManager->getTimer();
                if( !timer || !getNumber( action, "value", fixedTimestep ) || fixedTimestep < 0.0001 ||
                    fixedTimestep > 1.0 )
                {
                    return false;
                }

                timer->setFixedTimeInterval( static_cast<f64>( fixedTimestep ) );
                return true;
            }

            if( name == "create_folder" )
            {
                return createMediaFolder( applicationManager, action );
            }
            if( name == "create_project" )
            {
                return createMediaProject( applicationManager, action );
            }

            auto gameManager = applicationManager->getGameManager();
            auto currentScene = gameManager ? gameManager->getCurrentScene() : nullptr;

            if( name == "set_scene_label" )
            {
                String label;
                if( !currentScene || !getString( action, "value", label ) )
                {
                    return false;
                }

                currentScene->setLabel( label );
                return true;
            }
            if( name == "set_scene_state" )
            {
                String state;
                if( !currentScene || !getString( action, "value", state ) )
                {
                    return false;
                }

                if( state == "edit" )
                {
                    currentScene->setState( scene::IGameScene::State::Edit );
                }
                else if( state == "play" )
                {
                    currentScene->setState( scene::IGameScene::State::Play );
                }
                else if( state == "reset" )
                {
                    currentScene->setState( scene::IGameScene::State::Reset );
                }
                else
                {
                    return false;
                }

                return true;
            }
            if( name == "save_scene" )
            {
                if( !currentScene )
                {
                    return false;
                }

                currentScene->saveScene();
                return true;
            }
            if( name == "clear_scene" )
            {
                if( !currentScene || !isConfirmed( action ) )
                {
                    return false;
                }

                currentScene->clear( true );
                return true;
            }

            if( name == "create_actor" )
            {
                String actorName;
                if( !gameManager || !currentScene || !getString( action, "actor", actorName ) ||
                    currentScene->findActorByName( actorName ) )
                {
                    return false;
                }

                Vector3<real_Num> vector;
                if( action.if_contains( "position" ) && !getVector3( action, "position", vector ) )
                {
                    return false;
                }
                if( action.if_contains( "rotation" ) && !getVector3( action, "rotation", vector ) )
                {
                    return false;
                }
                if( action.if_contains( "scale" ) &&
                    ( !getVector3( action, "scale", vector ) || std::abs( vector.x ) <= 0.000001 ||
                      std::abs( vector.y ) <= 0.000001 || std::abs( vector.z ) <= 0.000001 ) )
                {
                    return false;
                }
                bool propertyValue = false;
                for( const auto property :
                     { json::string_view( "enabled" ), json::string_view( "visible" ),
                       json::string_view( "static" ), json::string_view( "smooth_motion" ) } )
                {
                    if( action.if_contains( property ) && !getBool( action, property, propertyValue ) )
                    {
                        return false;
                    }
                }

                auto actor = gameManager->createActor();
                if( !actor )
                {
                    return false;
                }

                actor->setName( actorName );
                applyOptionalActorProperties( action, actor );
                currentScene->addActor( actor );
                return true;
            }

            auto actor = findActor( applicationManager, action );
            if( !actor )
            {
                return false;
            }

            bool cascade = false;
            getBool( action, "cascade", cascade );

            if( name == "destroy_actor" )
            {
                if( !gameManager || !isConfirmed( action ) )
                {
                    return false;
                }

                auto destroyCascade = true;
                getBool( action, "cascade", destroyCascade );
                gameManager->destroyActor( actor, destroyCascade );
                return true;
            }
            if( name == "rename_actor" )
            {
                String newName;
                if( !getString( action, "value", newName ) || currentScene->findActorByName( newName ) )
                {
                    return false;
                }

                actor->setName( newName );
                return true;
            }
            if( name == "set_actor_enabled" && getActionBool( action, value ) )
            {
                actor->setEnabled( value, cascade );
                return true;
            }
            if( name == "set_actor_visible" && getActionBool( action, value ) )
            {
                actor->setVisible( value, cascade );
                return true;
            }
            if( name == "set_actor_static" && getActionBool( action, value ) )
            {
                actor->setStatic( value, cascade );
                return true;
            }
            if( name == "set_actor_smooth_motion" && getActionBool( action, value ) )
            {
                actor->setSmoothMotion( value, cascade );
                return true;
            }
            if( name == "set_actor_collision_mask" )
            {
                u32 mask = 0;
                if( !getU32( action, "value", mask ) )
                {
                    return false;
                }

                actor->setCollisionMask( mask, cascade );
                return true;
            }

            auto transform = actor->getTransform();
            if( !transform )
            {
                return false;
            }

            Vector3<real_Num> vector;
            if( name == "set_actor_position" && getVector3( action, "value", vector ) )
            {
                transform->setPosition( vector );
                return true;
            }
            if( name == "translate_actor" && getVector3( action, "value", vector ) )
            {
                transform->setPosition( transform->getPosition() + vector );
                return true;
            }
            if( name == "set_actor_rotation" && getVector3( action, "value", vector ) )
            {
                transform->setRotation( vector );
                return true;
            }
            if( name == "rotate_actor" && getVector3( action, "value", vector ) )
            {
                transform->setRotation( transform->getRotation() + vector );
                return true;
            }
            if( name == "set_actor_scale" && getVector3( action, "value", vector ) &&
                std::abs( vector.x ) > 0.000001 && std::abs( vector.y ) > 0.000001 &&
                std::abs( vector.z ) > 0.000001 )
            {
                transform->setScale( vector );
                return true;
            }
            if( name == "set_actor_transform" )
            {
                Vector3<real_Num> position;
                Vector3<real_Num> rotation;
                Vector3<real_Num> scale;
                const auto hasPosition = action.if_contains( "position" ) != nullptr;
                const auto hasRotation = action.if_contains( "rotation" ) != nullptr;
                const auto hasScale = action.if_contains( "scale" ) != nullptr;

                if( !hasPosition && !hasRotation && !hasScale )
                {
                    return false;
                }
                if( hasPosition && !getVector3( action, "position", position ) )
                {
                    return false;
                }
                if( hasRotation && !getVector3( action, "rotation", rotation ) )
                {
                    return false;
                }
                if( hasScale &&
                    ( !getVector3( action, "scale", scale ) || std::abs( scale.x ) <= 0.000001 ||
                      std::abs( scale.y ) <= 0.000001 || std::abs( scale.z ) <= 0.000001 ) )
                {
                    return false;
                }

                if( hasPosition )
                {
                    transform->setPosition( position );
                }
                if( hasRotation )
                {
                    transform->setRotation( rotation );
                }
                if( hasScale )
                {
                    transform->setScale( scale );
                }

                return true;
            }

            return false;
        }

        String makeControlPrompt( const String &prompt )
        {
            String controlPrompt =
                "You are the control interpreter for the Workphone game engine. "
                "Return exactly one JSON object and no markdown. The object must use this schema: "
                "{\"message\":\"short user-facing result\",\"actions\":["
                "{\"name\":\"action_name\",\"actor\":\"optional actor name\","
                "\"path\":\"optional media-relative path\",\"value\":\"action-specific value\","
                "\"product_name\":\"optional product name\","
                "\"company_name\":\"optional company name\","
                "\"current_scene\":\"optional project-relative scene\","
                "\"template\":\"optional supported template\"}]}. "
                "Runtime actions: play, stop, pause, resume; boolean set_playing, set_paused, "
                "set_running, set_renderer_enabled, set_editor_mode, set_editor_camera, "
                "set_pause_menu_active; set_running=false and request_quit require confirm=true; "
                "set_master_volume from 0 to 1; and set_fixed_timestep in seconds. "
                "Media actions: create_folder and create_project use a media-relative path field. "
                "create_project optionally accepts product_name, company_name, and a "
                "project-relative current_scene. Supported templates are hello_world_ui, which "
                "creates a Hello World Text element in the game scene UI, and catch_game_3d, "
                "which creates a complete Lua catch game, and racing_game_3d, which creates an "
                "arcade racing game, and flight_simulator_3d, which creates a Lua flight course "
                "with scene UI and a VehicleController backed by WPVehiclePhysics aerodynamics. "
                "The game templates use cube_internal.fbmeshbin. "
                "Scene actions: set_scene_label, set_scene_state with edit/play/reset, save_scene, "
                "and clear_scene with confirm=true. "
                "Actor actions: create_actor; destroy_actor with confirm=true; rename_actor; "
                "boolean set_actor_enabled, set_actor_visible, set_actor_static, and "
                "set_actor_smooth_motion; set_actor_collision_mask; set_actor_position, "
                "translate_actor, set_actor_rotation, rotate_actor, set_actor_scale, and "
                "set_actor_transform. Actor actions identify the target with actor. "
                "Vectors are [x,y,z]. create_actor and set_actor_transform may instead use "
                "position, rotation, and scale fields. cascade is an optional boolean. "
                "Never invent actions, paths, code, shell commands, or arguments. "
                "For a request that does not require engine control, return an empty actions array. "
                "User request: ";
            controlPrompt += prompt;
            return controlPrompt;
        }

        String parseOllamaResponse( const String &body, bool streaming )
        {
            if( !streaming )
            {
                boost::system::error_code error;
                const auto value = json::parse( json::string_view( body.data(), body.size() ), error );
                return error ? String() : getResponseText( value );
            }

            String responseText;
            std::istringstream lines( body );
            std::string line;
            while( std::getline( lines, line ) )
            {
                if( line.empty() )
                {
                    continue;
                }

                boost::system::error_code error;
                const auto value = json::parse( line, error );
                if( !error )
                {
                    responseText += getResponseText( value );
                }
            }

            return responseText;
        }

#    if defined _WIN32
        class WinHttpHandle
        {
        public:
            explicit WinHttpHandle( HINTERNET handle = nullptr ) : m_handle( handle )
            {
            }

            ~WinHttpHandle()
            {
                if( m_handle )
                {
                    WinHttpCloseHandle( m_handle );
                }
            }

            WinHttpHandle( const WinHttpHandle & ) = delete;
            WinHttpHandle &operator=( const WinHttpHandle & ) = delete;

            operator HINTERNET() const
            {
                return m_handle;
            }

        private:
            HINTERNET m_handle;
        };

        bool requestOllama( const String &requestBody, String &responseBody )
        {
            WinHttpHandle session( WinHttpOpen( L"Workphone", WINHTTP_ACCESS_TYPE_NO_PROXY,
                                                WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0 ) );
            if( !session )
            {
                return false;
            }

            WinHttpSetTimeouts( session, ollamaTimeoutMilliseconds, ollamaTimeoutMilliseconds,
                                ollamaTimeoutMilliseconds, ollamaTimeoutMilliseconds );

            WinHttpHandle connection( WinHttpConnect( session, L"127.0.0.1", 11434, 0 ) );
            if( !connection )
            {
                return false;
            }

            WinHttpHandle request( WinHttpOpenRequest( connection, L"POST", L"/api/generate", nullptr,
                                                       WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                       0 ) );
            if( !request )
            {
                return false;
            }

            constexpr auto headers = L"Content-Type: application/json\r\n";
            if( !WinHttpSendRequest( request, headers, static_cast<DWORD>( -1L ),
                                     const_cast<char *>( requestBody.data() ),
                                     static_cast<DWORD>( requestBody.size() ),
                                     static_cast<DWORD>( requestBody.size() ), 0 ) ||
                !WinHttpReceiveResponse( request, nullptr ) )
            {
                return false;
            }

            DWORD statusCode = 0;
            DWORD statusCodeSize = sizeof( statusCode );
            if( !WinHttpQueryHeaders( request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                      WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize,
                                      WINHTTP_NO_HEADER_INDEX ) ||
                statusCode < 200 || statusCode >= 300 )
            {
                return false;
            }

            for( ;; )
            {
                DWORD available = 0;
                if( !WinHttpQueryDataAvailable( request, &available ) )
                {
                    return false;
                }

                if( available == 0 )
                {
                    break;
                }

                const auto offset = responseBody.size();
                responseBody.resize( offset + available );

                DWORD bytesRead = 0;
                if( !WinHttpReadData( request, responseBody.data() + offset, available, &bytesRead ) )
                {
                    return false;
                }

                responseBody.resize( offset + bytesRead );
            }

            return true;
        }
#    else
        bool requestOllama( const String &requestBody, String &responseBody )
        {
            asio::io_context context;
            tcp::resolver resolver( context );
            beast::tcp_stream stream( context );

            const auto endpoints = resolver.resolve( ollamaHost, ollamaPort );
            stream.expires_after( ollamaTimeout );
            stream.connect( endpoints );

            http::request<http::string_body> request( http::verb::post, ollamaTarget, 11 );
            request.set( http::field::host, ollamaHost );
            request.set( http::field::user_agent, "Workphone" );
            request.set( http::field::content_type, "application/json" );
            request.body() = requestBody;
            request.prepare_payload();

            stream.expires_after( ollamaTimeout );
            http::write( stream, request );

            beast::flat_buffer buffer;
            http::response<http::string_body> response;
            stream.expires_after( ollamaTimeout );
            http::read( stream, buffer, response );

            boost::system::error_code shutdownError;
            stream.socket().shutdown( tcp::socket::shutdown_both, shutdownError );

            if( response.result_int() < 200 || response.result_int() >= 300 )
            {
                return false;
            }

            responseBody = std::move( response.body() );
            return true;
        }
#    endif

        String queryOllama( const String &prompt, bool streaming )
        {
            try
            {
                json::object requestBody;
                requestBody["model"] = ollamaModel;
                requestBody["prompt"] = json::string( prompt.data(), prompt.size() );
                requestBody["stream"] = streaming;
                requestBody["format"] = "json";
                requestBody["options"] = json::object( { { "temperature", 0.0 }, { "seed", 42 } } );

                String responseBody;
                if( !requestOllama( json::serialize( requestBody ), responseBody ) )
                {
                    return {};
                }

                return parseOllamaResponse( responseBody, streaming );
            }
            catch( const std::exception & )
            {
                return {};
            }
        }
    }  // namespace
#endif

    WP_CLASS_REGISTER_DERIVED( workphone, AiManager, IAiManager );

    AiManager::AiManager() = default;

    AiManager::~AiManager() = default;

    SmartPtr<IPathfinder2> AiManager::getPathfinder2() const
    {
        return m_pathfinder2;
    }

    void AiManager::setPathfinder2( SmartPtr<IPathfinder2> pathfinder )
    {
        m_pathfinder2 = pathfinder;
    }

    String AiManager::query( const String &prompt ) const
    {
#if WP_USE_BOOST
        const auto response = query_ollama( makeControlPrompt( prompt ) );
        processResponse( response );
        return getControlMessage( response );
#else
        return query_ollama( prompt );
#endif
    }

    bool AiManager::processResponse( const String &response ) const
    {
#if WP_USE_BOOST
        json::object root;
        if( !parseControlResponse( response, root ) )
        {
            return false;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return false;
        }

        bool appliedAction = false;
        if( const auto actions = root.if_contains( "actions" ); actions && actions->is_array() )
        {
            if( actions->as_array().size() > maximumActionsPerResponse )
            {
                return false;
            }

            for( const auto &action : actions->as_array() )
            {
                if( action.is_object() )
                {
                    appliedAction =
                        applyControlAction( applicationManager, action.as_object() ) || appliedAction;
                }
            }
        }
        else
        {
            appliedAction = applyControlAction( applicationManager, root );
        }

        return appliedAction;
#else
        return false;
#endif
    }

    String AiManager::query_ollama( const String &prompt ) const
    {
#if WP_USE_BOOST
        return queryOllama( prompt, false );
#else
        return {};
#endif
    }

    String AiManager::query_ollama2( const String &prompt ) const
    {
#if WP_USE_BOOST
        return queryOllama( prompt, true );
#else
        return {};
#endif
    }

}  // namespace workphone
