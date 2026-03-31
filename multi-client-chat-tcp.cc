#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <sstream>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpChatSimulation");

/* ---------------- SERVER APPLICATION ---------------- */

class TcpServerApp : public Application
{
private:
    Ptr<Socket> socket;
    uint16_t port;

public:
    TcpServerApp() : socket(0), port(8080) {}

    void Configure(uint16_t p)
    {
        port = p;
    }

    virtual void StartApplication()
    {
        socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());

        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);
        socket->Bind(local);
        socket->Listen();

        socket->SetRecvCallback(MakeCallback(&TcpServerApp::ReceivePacket, this));
    }

    void ReceivePacket(Ptr<Socket> sock)
    {
        Ptr<Packet> packet;

        while ((packet = sock->Recv()))
        {
            uint32_t size = packet->GetSize();
            double time = Simulator::Now().GetSeconds();

            std::stringstream reply;

            reply << "ACK | Time: "
                  << time
                  << " sec | Packet Size: "
                  << size
                  << " bytes";

            std::string response = reply.str();

            Ptr<Packet> sendPacket =
                Create<Packet>((uint8_t *)response.c_str(), response.length());

            sock->Send(sendPacket);
        }
    }
};

/* ---------------- CLIENT APPLICATION ---------------- */

class TcpClientApp : public Application
{
private:
    Ptr<Socket> socket;
    Address serverAddress;
    Time interval;
    std::string name;

public:
    TcpClientApp() : socket(0) {}

    void Configure(Address addr, Time t, std::string id)
    {
        serverAddress = addr;
        interval = t;
        name = id;
    }

    virtual void StartApplication()
    {
        socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());

        socket->Connect(serverAddress);

        socket->SetRecvCallback(MakeCallback(&TcpClientApp::ReceiveReply, this));

        SendMessage();
    }

    void SendMessage()
    {
        std::string text = "Hello from " + name;

        Ptr<Packet> pkt =
            Create<Packet>((uint8_t *)text.c_str(), text.length());

        socket->Send(pkt);

        Simulator::Schedule(interval, &TcpClientApp::SendMessage, this);
    }

    void ReceiveReply(Ptr<Socket> sock)
    {
        Ptr<Packet> packet = sock->Recv();

        std::vector<uint8_t> buffer(packet->GetSize());
        packet->CopyData(buffer.data(), packet->GetSize());

        std::string message((char *)buffer.data(), packet->GetSize());

        NS_LOG_UNCOND(name << " received -> " << message);
    }
};

/* ---------------- MAIN FUNCTION ---------------- */

int main(int argc, char *argv[])
{
    NodeContainer nodes;
    nodes.Create(3); // server + 2 clients

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

    /* Server */

    Ptr<TcpServerApp> server = CreateObject<TcpServerApp>();
    server->Configure(8080);
    nodes.Get(0)->AddApplication(server);

    /* Client 1 */

    Ptr<TcpClientApp> client1 = CreateObject<TcpClientApp>();
    client1->Configure(
        InetSocketAddress(iface1.GetAddress(1), 8080),
        Seconds(3.0),
        "CLIENT_1");

    nodes.Get(1)->AddApplication(client1);

    /* Client 2 */

    Ptr<TcpClientApp> client2 = CreateObject<TcpClientApp>();
    client2->Configure(
        InetSocketAddress(iface2.GetAddress(1), 8080),
        Seconds(5.0),
        "CLIENT_2");

    nodes.Get(2)->AddApplication(client2);

    NS_LOG_UNCOND("TCP Simulation Started");

    Simulator::Stop(Seconds(15));

    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_UNCOND("TCP Simulation Finished");

    return 0;
}
