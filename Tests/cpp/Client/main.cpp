#include <Workphone/Workphone.hpp>
#include <FBRakNet/FBRakNet.hpp>


using namespace lioncat;


//class MyNetworkCallback : public INetworkCallback
//{
//public:
//	virtual void handlePacket(SmartPtr<IPacket> packet)
//	{
//		// Print the values to the console.
//		//std::cout << "Message: " << str.c_str();
//		//std::cout << " Position: " << vec.X << " " << vec.Y << " " << vec.Z;
//		//std::cout << " Height: " << height << " ft";
//		std::cout << std::endl;
//	}
//};


int main()
{
    //MyNetworkCallback* netCallback = new MyNetworkCallback;

    //INetworkManager* client = createRakNetClient("");
    //client->setNetCallback(netCallback);

    ////client->connect("127.0.0.1", 15822, "Neko");
    //client->connect("127.0.0.1", 15822, "Rumpelstiltskin");

    //while(true)
    //{
    //	client->update(10);

    //	SmartPtr<IPacket> packet = client->createPacket();
    //	packet->write((u8)200);
    //	packet->write("Help I am stuck on a mountain!");
    //	client->sendOutPacket(packet);

    //	Sleep(1000);
    //}

    //FB_SAFE_DELETE(client);

    return 0;
}
