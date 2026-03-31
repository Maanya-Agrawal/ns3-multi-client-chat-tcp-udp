#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <sstream>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ChatSimulation");

/* ---------------- SERVER APPLICATION ---------------- */

class ServerApp : public Application
{
private:
    Ptr<Socket> socket;
    uint16_t portNumber;

public:
    ServerApp() : socket(0), portNumber(8080) {}

    void Configure(uint16_t port)
    {
        portNumber = port;
    }

    virtual void StartApplication()
    {
        socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());

        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), portNumber);
        socket->Bind(local);

        socket->SetRecvCallback(MakeCallback(&ServerApp::ReceivePacket, this));
    }

    void ReceivePacket(Ptr<Socket> sock)
    {
        Ptr<Packet> packet;
        Address sender;

        while ((packet = sock->RecvFrom(sender)))
        {
            uint32_t packetSize = packet->GetSize();
            double currentTime = Simulator::Now().GetSeconds();

            std::stringstream response;

            response << "ACK | Time: "
                     << currentTime
                     << " sec | Packet Size: "
                     << packetSize
                     << " bytes";

            std::string reply = response.str();

            NS_LOG_UNCOND("Server received packet from "
                          << InetSocketAddress::ConvertFrom(sender).GetIpv4());

            Ptr<Packet> replyPacket =
                Create<Packet>((uint8_t *)reply.c_str(), reply.length());

            sock->SendTo(replyPacket, 0, sender);
        }
    }
};

/* ---------------- CLIENT APPLICATION ---------------- */

class ClientApp : public Application
{
private:
    Ptr<Socket> socket;
    Address serverAddress;
    Time sendInterval;
    std::string clientId;

public:
    ClientApp() : socket(0) {}

    void Configure(Address addr, Time interval, std::string name)
    {
        serverAddress = addr;
        sendInterval = interval;
        clientId = name;
    }

    virtual void StartApplication()
    {
        socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());

        socket->Connect(serverAddress);

        socket->SetRecvCallback(MakeCallback(&ClientApp::ReceiveResponse, this));

        SendMessage();
    }

    void SendMessage()
    {
        std::string text = "Message from " + clientId;

        Ptr<Packet> pkt =
            Create<Packet>((uint8_t *)text.c_str(), text.length());

        socket->Send(pkt);

        Simulator::Schedule(sendInterval,
                            &ClientApp::SendMessage,
                            this);
    }

    void ReceiveResponse(Ptr<Socket> sock)
    {
        Ptr<Packet> packet = sock->Recv();

        std::vector<uint8_t> buffer(packet->GetSize());
        packet->CopyData(buffer.data(), packet->GetSize());

        std::string message =
            std::string((char *)buffer.data(), packet->GetSize());

        NS_LOG_UNCOND(clientId << " received -> " << message);
    }
};

/* ---------------- MAIN SIMULATION ---------------- */

int main(int argc, char *argv[])
{
    NodeContainer nodes;
    nodes.Create(3); // 1 server + 2 clients

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer link1 = p2p.Install(nodes.Get(1), nodes.Get(0));
    NetDeviceContainer link2 = p2p.Install(nodes.Get(2), nodes.Get(0));

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer iface1 = address.Assign(link1);

    address.SetBase("10.2.1.0", "255.255.255.0");
    Ipv4InterfaceContainer iface2 = address.Assign(link2);

    /* Install Server */

    Ptr<ServerApp> server = CreateObject<ServerApp>();
    server->Configure(8080);

    nodes.Get(0)->AddApplication(server);

    /* Client 1 */

    Ptr<ClientApp> client1 = CreateObject<ClientApp>();

    client1->Configure(
        InetSocketAddress(iface1.GetAddress(1), 8080),
        Seconds(3.0),
        "CLIENT_1");

    nodes.Get(1)->AddApplication(client1);

    /* Client 2 */

    Ptr<ClientApp> client2 = CreateObject<ClientApp>();

    client2->Configure(
        InetSocketAddress(iface2.GetAddress(1), 8080),
        Seconds(5.0),
        "CLIENT_2");

    nodes.Get(2)->AddApplication(client2);

    NS_LOG_UNCOND("Simulation Started");

    Simulator::Stop(Seconds(15));

    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_UNCOND("Simulation Completed");

    return 0;
}
