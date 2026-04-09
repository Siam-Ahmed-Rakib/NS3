/*
 * Usage:
 *   ./ns3 run "scratch/my-experts-simulation
 *     --tcp=experts|ml|plus|newreno
 *     --nFlows=7  --nNodes=20  --simTime=1500  --run=1
 *     --nExperts=100  --eta=2.0  --shareAlpha=0.08
 *     --ratioThreshold=1.25"
 */

#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/yans-wifi-helper.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ExpertsSimulation");


static void
CwndChange(std::string context, uint32_t oldCwnd, uint32_t newCwnd)
{
    std::cout << Simulator::Now().GetSeconds() << "\t" << context << "\tCWND\t" << newCwnd
              << std::endl;
}

static void
ConnectCwndTrace()
{
    Config::Connect("/NodeList/*/$ns3::TcpL4Protocol/SocketList/*/CongestionWindow",
                    MakeCallback(&CwndChange));
}


int
main(int argc, char* argv[])
{
   
    std::string tcp = "experts";
    uint32_t nNodes = 20;
    uint32_t nFlows = 7;
    double simTime = 1500.0; 
    double areaX = 1500.0;
    double areaY = 1000.0;
    double minSpeed = 1.0;
    double maxSpeed = 50.0;
    uint32_t packetSize = 512;
    uint32_t run = 1;

   
    uint32_t nExperts = 100;
    double eta = 2.0;
    double shareAlpha = 0.08;
    double rttMin = 1.0;
    double rttMax = 500.0;
    double ratioThreshold = 1.25;

   
    double alpha = 0.25;

    
    double processNoise = 1.0;
    double measurementNoise = 10.0;

    uint32_t minPackets = 1000;
    uint32_t maxPackets = 100000;

    CommandLine cmd(__FILE__);
    cmd.AddValue("tcp", "TCP variant: experts, ml, plus, newreno", tcp);
    cmd.AddValue("nNodes", "Number of mobile nodes", nNodes);
    cmd.AddValue("nFlows", "Number of concurrent TCP flows", nFlows);
    cmd.AddValue("simTime", "Simulation time (s)", simTime);
    cmd.AddValue("areaX", "Area width (m)", areaX);
    cmd.AddValue("areaY", "Area height (m)", areaY);
    cmd.AddValue("minSpeed", "Min speed (m/s)", minSpeed);
    cmd.AddValue("maxSpeed", "Max speed (m/s)", maxSpeed);
    cmd.AddValue("packetSize", "Packet size (bytes)", packetSize);
    cmd.AddValue("run", "RNG run number", run);
    cmd.AddValue("nExperts", "Number of experts N (experts mode)", nExperts);
    cmd.AddValue("eta", "Learning rate η (experts mode)", eta);
    cmd.AddValue("shareAlpha", "Sharing rate α (experts mode)", shareAlpha);
    cmd.AddValue("rttMin", "Expert RTTmin (ms)", rttMin);
    cmd.AddValue("rttMax", "Expert RTTmax (ms)", rttMax);
    cmd.AddValue("ratioThreshold", "PredictedRTT/BaseRTT threshold", ratioThreshold);
    cmd.AddValue("alpha", "EWMA alpha (ml mode)", alpha);
    cmd.AddValue("processNoise", "Kalman process noise Q (kalman mode)", processNoise);
    cmd.AddValue("measurementNoise", "Kalman measurement noise R (kalman mode)", measurementNoise);
    cmd.AddValue("minPackets", "Min packets per flow", minPackets);
    cmd.AddValue("maxPackets", "Max packets per flow", maxPackets);
    cmd.Parse(argc, argv);

  
    if (tcp == "experts")
    {
        Config::SetDefault("ns3::TcpWestwoodExperts::NumExperts", UintegerValue(nExperts));
        Config::SetDefault("ns3::TcpWestwoodExperts::LearningRate", DoubleValue(eta));
        Config::SetDefault("ns3::TcpWestwoodExperts::ShareAlpha", DoubleValue(shareAlpha));
        Config::SetDefault("ns3::TcpWestwoodExperts::RttMin", DoubleValue(rttMin));
        Config::SetDefault("ns3::TcpWestwoodExperts::RttMax", DoubleValue(rttMax));
        Config::SetDefault("ns3::TcpWestwoodExperts::RatioThreshold",
                           DoubleValue(ratioThreshold));
        Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                           TypeIdValue(TypeId::LookupByName("ns3::TcpWestwoodExperts")));
    }
    else if (tcp == "ml")
    {
        Config::SetDefault("ns3::TcpWestwoodMl::Alpha", DoubleValue(alpha));
        Config::SetDefault("ns3::TcpWestwoodMl::RatioThreshold", DoubleValue(ratioThreshold));
        Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                           TypeIdValue(TypeId::LookupByName("ns3::TcpWestwoodMl")));
    }
    else if (tcp == "kalman")
    {
        Config::SetDefault("ns3::TcpWestwoodKalman::ProcessNoise", DoubleValue(processNoise));
        Config::SetDefault("ns3::TcpWestwoodKalman::MeasurementNoise", DoubleValue(measurementNoise));
        Config::SetDefault("ns3::TcpWestwoodKalman::RatioThreshold", DoubleValue(ratioThreshold));
        Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                           TypeIdValue(TypeId::LookupByName("ns3::TcpWestwoodKalman")));
    }
    else if (tcp == "adaptive")
    {
        Config::SetDefault("ns3::TcpWestwoodExpertsAdaptive::NumExperts", UintegerValue(nExperts));
        Config::SetDefault("ns3::TcpWestwoodExpertsAdaptive::LearningRateBase", DoubleValue(eta));
        Config::SetDefault("ns3::TcpWestwoodExpertsAdaptive::ShareAlphaBase", DoubleValue(shareAlpha));
        Config::SetDefault("ns3::TcpWestwoodExpertsAdaptive::RttMin", DoubleValue(rttMin));
        Config::SetDefault("ns3::TcpWestwoodExpertsAdaptive::RttMax", DoubleValue(rttMax));
        Config::SetDefault("ns3::TcpWestwoodExpertsAdaptive::RatioThreshold", DoubleValue(ratioThreshold));
        Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                           TypeIdValue(TypeId::LookupByName("ns3::TcpWestwoodExpertsAdaptive")));
    }
    else if (tcp == "plus")
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                           TypeIdValue(TypeId::LookupByName("ns3::TcpWestwoodPlus")));
    }
    else 
    {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                           TypeIdValue(TypeId::LookupByName("ns3::TcpNewReno")));
    }

    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(packetSize));
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode",
                       StringValue("DsssRate11Mbps"));
    RngSeedManager::SetRun(run);

   
    NodeContainer nodes;
    nodes.Create(nNodes);

    
    MobilityHelper mobility;
    ObjectFactory posFactory;
    posFactory.SetTypeId("ns3::RandomRectanglePositionAllocator");
    posFactory.Set("X",
                   StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" +
                               std::to_string(areaX) + "]"));
    posFactory.Set("Y",
                   StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" +
                               std::to_string(areaY) + "]"));
    Ptr<PositionAllocator> posAlloc = posFactory.Create()->GetObject<PositionAllocator>();

    std::string speedStr = "ns3::UniformRandomVariable[Min=" + std::to_string(minSpeed) +
                           "|Max=" + std::to_string(maxSpeed) + "]";
    mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                              "Speed",
                              StringValue(speedStr),
                              "Pause",
                              StringValue("ns3::ConstantRandomVariable[Constant=0.0]"),
                              "PositionAllocator",
                              PointerValue(posAlloc));
    mobility.SetPositionAllocator(posAlloc);
    mobility.Install(nodes);

    
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("DsssRate11Mbps"),
                                 "ControlMode",
                                 StringValue("DsssRate1Mbps"));

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");

    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

    
    AodvHelper aodv;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.0.0", "255.255.0.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    
    uint16_t basePort = 9000;
    Ptr<UniformRandomVariable> startRv = CreateObject<UniformRandomVariable>();
    startRv->SetAttribute("Min", DoubleValue(10.0));
    startRv->SetAttribute("Max", DoubleValue(simTime * 0.9));

    Ptr<UniformRandomVariable> sizeRv = CreateObject<UniformRandomVariable>();
    sizeRv->SetAttribute("Min", DoubleValue(static_cast<double>(minPackets) * packetSize));
    sizeRv->SetAttribute("Max", DoubleValue(static_cast<double>(maxPackets) * packetSize));

    Ptr<UniformRandomVariable> nodeRv = CreateObject<UniformRandomVariable>();
    nodeRv->SetAttribute("Min", DoubleValue(0));
    nodeRv->SetAttribute("Max", DoubleValue(static_cast<double>(nNodes) - 0.001));

    for (uint32_t f = 0; f < nFlows; ++f)
    {
        uint32_t src = static_cast<uint32_t>(nodeRv->GetValue());
        uint32_t dst;
        do
        {
            dst = static_cast<uint32_t>(nodeRv->GetValue());
        } while (dst == src);

        uint16_t port = basePort + f;

        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                    InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(dst));
        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(Seconds(simTime));

        BulkSendHelper bulkHelper("ns3::TcpSocketFactory",
                                  InetSocketAddress(interfaces.GetAddress(dst), port));
        bulkHelper.SetAttribute("SendSize", UintegerValue(packetSize));

        uint64_t maxBytes = static_cast<uint64_t>(sizeRv->GetValue());
        bulkHelper.SetAttribute("MaxBytes", UintegerValue(maxBytes));

        ApplicationContainer srcApp = bulkHelper.Install(nodes.Get(src));
        double startT = startRv->GetValue();
        srcApp.Start(Seconds(startT));
        srcApp.Stop(Seconds(simTime));

        NS_LOG_UNCOND("Flow " << f << ": node " << src << " -> node " << dst << " port=" << port
                               << " start=" << startT << "s maxBytes=" << maxBytes);
    }

    
    Simulator::Schedule(Seconds(12.0), &ConnectCwndTrace);

  
    LogComponentEnable("ExpertsSimulation", LOG_LEVEL_INFO);

    
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

   
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());

    uint64_t totalTxPackets = 0;
    uint64_t totalRxPackets = 0;
    uint64_t totalLostPackets = 0;
    uint64_t totalRetxPackets = 0;
    double totalThroughput = 0.0;
    double totalDelay = 0.0;
    uint64_t totalDelayCount = 0;
    double totalJitter = 0.0;
    uint64_t totalJitterCount = 0;

    for (const auto& flow : monitor->GetFlowStats())
    {
        auto t = classifier->FindFlow(flow.first);
        double duration =
            (flow.second.timeLastRxPacket - flow.second.timeFirstTxPacket).GetSeconds();
        double throughput =
            (duration > 0) ? (flow.second.rxBytes * 8.0 / duration / 1e6) : 0.0;

        double meanDelay = (flow.second.rxPackets > 0)
                               ? (flow.second.delaySum.GetMilliSeconds() /
                                  static_cast<double>(flow.second.rxPackets))
                               : 0.0;
        double meanJitter = (flow.second.rxPackets > 1)
                                ? (flow.second.jitterSum.GetMilliSeconds() /
                                   static_cast<double>(flow.second.rxPackets - 1))
                                : 0.0;

        std::cout << "\nFlow " << flow.first << " (" << t.sourceAddress << ":"
                  << t.sourcePort << " -> " << t.destinationAddress << ":"
                  << t.destinationPort << ")\n";
        std::cout << "  Tx Packets:       " << flow.second.txPackets << "\n";
        std::cout << "  Rx Packets:       " << flow.second.rxPackets << "\n";
        std::cout << "  Lost Packets:     " << flow.second.lostPackets << "\n";
        std::cout << "  Retx (forwarded): " << flow.second.timesForwarded << "\n";
        std::cout << "  Packet Loss:      "
                  << (flow.second.txPackets > 0
                          ? 100.0 * flow.second.lostPackets / flow.second.txPackets
                          : 0.0)
                  << " %\n";
        std::cout << "  Throughput:       " << throughput << " Mbps\n";
        std::cout << "  Mean Delay:       " << meanDelay << " ms\n";
        std::cout << "  Mean Jitter:      " << meanJitter << " ms\n";

        totalTxPackets += flow.second.txPackets;
        totalRxPackets += flow.second.rxPackets;
        totalLostPackets += flow.second.lostPackets;
        totalRetxPackets += flow.second.timesForwarded;
        totalThroughput += throughput;
        if (flow.second.rxPackets > 0)
        {
            totalDelay += flow.second.delaySum.GetMilliSeconds();
            totalDelayCount += flow.second.rxPackets;
        }
        if (flow.second.rxPackets > 1)
        {
            totalJitter += flow.second.jitterSum.GetMilliSeconds();
            totalJitterCount += flow.second.rxPackets - 1;
        }
    }

    double overallLoss =
        (totalTxPackets > 0) ? (100.0 * totalLostPackets / totalTxPackets) : 0.0;
    double overallDelay = (totalDelayCount > 0) ? (totalDelay / totalDelayCount) : 0.0;
    double overallJitter = (totalJitterCount > 0) ? (totalJitter / totalJitterCount) : 0.0;

    std::cout << "\n=== SUMMARY ===\n";
    std::cout << "TCP Variant:        " << tcp << "\n";
    std::cout << "Nodes:              " << nNodes << "\n";
    std::cout << "Flows:              " << nFlows << "\n";
    std::cout << "Sim Time:           " << simTime << " s\n";
    std::cout << "Total Tx Packets:   " << totalTxPackets << "\n";
    std::cout << "Total Rx Packets:   " << totalRxPackets << "\n";
    std::cout << "Total Lost Packets: " << totalLostPackets << "\n";
    std::cout << "Retransmissions:    " << totalRetxPackets << "\n";
    std::cout << "Packet Loss Rate:   " << overallLoss << " %\n";
    std::cout << "Total Throughput:   " << totalThroughput << " Mbps\n";
    std::cout << "Mean Delay:         " << overallDelay << " ms\n";
    std::cout << "Mean Jitter:        " << overallJitter << " ms\n";

    Simulator::Destroy();
    std::cout << "\nSimulation finished.\n";
    return 0;
}
