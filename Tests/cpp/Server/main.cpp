#include <Workphone/Workphone.hpp>
#include <FBRakNet/FBRakNet.hpp>

using namespace workphone;

class MyNetworkCallback  //: public INetworkCallback
{
public:
    //virtual void handlePacket(SmartPtr<IPacket> packet)
    //{
    //	u8 packetType;
    //	packet->read(packetType);

    //	switch(packetType)
    //	{
    //	case ID_CONNECTION_LOST:
    //		break;
    //	case ID_DISCONNECTION_NOTIFICATION:
    //		break;
    //	case ID_NEW_INCOMING_CONNECTION:
    //		break;
    //	case 200:
    //		{
    //			String value;
    //			packet->read(value);
    //			std::cout << "Message: " << value.c_str();
    //			std::cout << std::endl;
    //		}
    //		break;
    //	default:
    //		{
    //		}
    //	};
    //}
};

int main()
{
    //MyNetworkCallback* netCallback = new MyNetworkCallback;

    //INetworkManager* server = createRakNetServer("");
    //server->setNetCallback(netCallback);

    //while(true)
    //{
    //	server->update(10);
    //	Sleep(100);
    //}
    //
    //FB_SAFE_DELETE(server);

    return 0;
}
