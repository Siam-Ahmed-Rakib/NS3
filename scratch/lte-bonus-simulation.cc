/*
 * Usage:
 *   ./ns3 run "scratch/lte-bonus-simulation
 *     --tcp=experts --nNodes=20 --nFlows=10
 *     --packetsPerSec=100 --speed=5 --simTime=60 --run=1"
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LteBonusSimulation");

int
main(int argc, char* argv[])
{
    
    std::string tcp = "experts";
    uint32_t nNodes = 20;   
    uint32_t nFlows = 10;
    uint32_t packetsPerSec = 100;
    double speed = 5.0;
    double simTime = 60.0;
    uint32_t packetSize = 1024;
    uint32_t run = 1;
    uint32_t nEnbs = 1;

   
    uint32_t nExperts = 100;
    double eta = 2.0;
    double shareAlpha = 0.08;
    double rttMin = 1.0;
    double rttMax = 500.0;
    double ratioThreshold = 1.25;
    double alpha = 0.25;
    double processNoise = 1.0;
    double measurementNoise = 10.0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("tcp", "TCP variant: experts, adaptive, kalman, ml, plus, newreno", tcp);
    cmd.AddValue("nNodes", "Number of UEs", nNodes);
    cmd.AddValue("nFlows", "Number of TCP flows", nFlows);
    cmd.AddValue("packetsPerSec", "Packets per second per flow", packetsPerSec);
    cmd.AddValue("speed", "UE speed (m/s)", speed);
    cmd.AddValue("simTime", "Simulation time (s)", simTime);
    cmd.AddValue("packetSize", "Packet size (bytes)", packetSize);
    cmd.AddValue("run", "RNG run number", run);
    cmd.AddValue("nEnbs", "Number of eNBs", nEnbs);
    cmd.AddValue("nExperts", "Experts N", nExperts);
    cmd.AddValue("eta", "Learning rate", eta);
    cmd.AddValue("shareAlpha", "Sharing rate", shareAlpha);
    cmd.AddValue("rttMin", "RTTmin (ms)", rttMin);
    cmd.AddValue("rttMax", "RTTmax (ms)", rttMax);
    cmd.AddValue("ratioThreshold", "Threshold", ratioThreshold);
    cmd.AddValue("alpha", "EWMA alpha", alpha);
    cmd.AddValue("processNoise", "Kalman Q", processNoise);
    cmd.AddValue("measurementNoise", "Kalman R", measurementNoise);
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
        Config::SetDefault("ns3::TcpWestwoodKalman::MeasurementNoise",
                           DoubleValue(measurementNoise));
        Config::SetDefault("ns3::TcpWestwoodKalman::RatioThreshold",
                           DoubleValue(ratioThreshold));
        Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                           TypeIdValue(TypeId::LookupByName("ns3::TcpWestwoodKalman")));
    }
    else if (tcp == "adaptive")
    {
        Config::SetDefault("ns3::TcpWestwoodExpertsAdaptive::NumExperts",
                           UintegerValue(nExperts));
        Config::SetDefault("ns3::TcpWestwoodExpertsAdaptive::LearningRateBase", DoubleValue(eta));
        Config::SetDefault("ns3::TcpWestwoodExpertsAdaptive::ShareAlphaBase",
                           DoubleValue(shareAlpha));
        Config::SetDefault("ns3::TcpWestwoodExpertsAdaptive::RttMin", DoubleValue(rttMin));
        Config::SetDefault("ns3::TcpWestwoodExpertsAdaptive::RttMax", DoubleValue(rttMax));
        Config::SetDefault("ns3::TcpWestwoodExpertsAdaptive::RatioThreshold",
                           DoubleValue(ratioThreshold));
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
    RngSeedManager::SetRun(run);

   
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    Ptr<Node> pgw = epcHelper->GetPgwNode();

   
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(MilliSeconds(10)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);

    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    
    NodeContainer enbNodes;
    enbNodes.Create(nEnbs);
    NodeContainer ueNodes;
    ueNodes.Create(nNodes);

    
    MobilityHelper enbMobility;
    enbMobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                     "MinX", DoubleValue(0.0),
                                     "MinY", DoubleValue(0.0),
                                     "DeltaX", DoubleValue(500.0),
                                     "DeltaY", DoubleValue(0.0),
                                     "GridWidth", UintegerValue(nEnbs),
                                     "LayoutType", StringValue("RowFirst"));
    enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMobility.Install(enbNodes);

    
    MobilityHelper ueMobility;
    ObjectFactory posFactory;
    posFactory.SetTypeId("ns3::RandomRectanglePositionAllocator");
    posFactory.Set("X", StringValue("ns3::UniformRandomVariable[Min=-250.0|Max=250.0]"));
    posFactory.Set("Y", StringValue("ns3::UniformRandomVariable[Min=-250.0|Max=250.0]"));
    Ptr<PositionAllocator> posAlloc = posFactory.Create()->GetObject<PositionAllocator>();

    std::string speedStr = "ns3::ConstantRandomVariable[Constant=" + std::to_string(speed) + "]";
    ueMobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                                "Speed", StringValue(speedStr),
                                "Pause", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"),
                                "PositionAllocator", PointerValue(posAlloc));
    ueMobility.SetPositionAllocator(posAlloc);
    ueMobility.Install(ueNodes);

   
    MobilityHelper fixedMobility;
    fixedMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    fixedMobility.Install(remoteHostContainer);

    
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);


    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));

    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ueNodes.Get(u)->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    
    for (uint32_t i = 0; i < ueLteDevs.GetN(); i++)
    {
        lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(i % nEnbs));
    }

 
    uint16_t basePort = 50000;
    uint64_t dataRate = static_cast<uint64_t>(packetsPerSec) * packetSize * 8;
    std::string dataRateStr = std::to_string(dataRate) + "bps";

    Ptr<UniformRandomVariable> ueRng = CreateObject<UniformRandomVariable>();
    ueRng->SetAttribute("Min", DoubleValue(0));
    ueRng->SetAttribute("Max", DoubleValue(static_cast<double>(nNodes) - 0.001));

    uint32_t actualFlows = std::min(nFlows, nNodes);

    for (uint32_t f = 0; f < actualFlows; ++f)
    {
        uint32_t ueIdx = static_cast<uint32_t>(ueRng->GetValue());
        uint16_t port = basePort + f;

        
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                    InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer sinkApp = sinkHelper.Install(remoteHost);
        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(Seconds(simTime));

       
        OnOffHelper client("ns3::TcpSocketFactory",
                          InetSocketAddress(remoteHostAddr, port));
        client.SetAttribute("DataRate", DataRateValue(DataRate(dataRateStr)));
        client.SetAttribute("PacketSize", UintegerValue(packetSize));
        client.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        client.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

        ApplicationContainer clientApp = client.Install(ueNodes.Get(ueIdx));
        clientApp.Start(Seconds(2.0 + f * 0.1));
        clientApp.Stop(Seconds(simTime));

        NS_LOG_UNCOND("Flow " << f << ": UE[" << ueIdx << "] -> RemoteHost port=" << port
                               << " rate=" << dataRateStr);
    }

   
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

   
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

   
    monitor->CheckForLostPackets();

    uint64_t totalTxPackets = 0;
    uint64_t totalRxPackets = 0;
    uint64_t totalLostPackets = 0;
    double totalThroughput = 0.0;
    double totalDelay = 0.0;
    uint64_t totalDelayCount = 0;

    for (const auto& flow : monitor->GetFlowStats())
    {
        double duration =
            (flow.second.timeLastRxPacket - flow.second.timeFirstTxPacket).GetSeconds();
        double throughput =
            (duration > 0) ? (flow.second.rxBytes * 8.0 / duration / 1e6) : 0.0;

        totalTxPackets += flow.second.txPackets;
        totalRxPackets += flow.second.rxPackets;
        totalLostPackets += flow.second.lostPackets;
        totalThroughput += throughput;
        if (flow.second.rxPackets > 0)
        {
            totalDelay += flow.second.delaySum.GetMilliSeconds();
            totalDelayCount += flow.second.rxPackets;
        }
    }

    double pdr = (totalTxPackets > 0) ? (100.0 * totalRxPackets / totalTxPackets) : 0.0;
    double dropRatio = (totalTxPackets > 0) ? (100.0 * totalLostPackets / totalTxPackets) : 0.0;
    double overallDelay = (totalDelayCount > 0) ? (totalDelay / totalDelayCount) : 0.0;

   
    double txPowerW = 1.0;
    double rxPowerW = 0.5;
    double idlePowerW = 0.05;
    double avgTxTimePerPkt = 0.001;
    double avgRxTimePerPkt = 0.001;
    double totalEnergy = (totalTxPackets * avgTxTimePerPkt * txPowerW) +
                         (totalRxPackets * avgRxTimePerPkt * rxPowerW) +
                         (nNodes * simTime * idlePowerW);

   
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "TCP Variant:        " << tcp << std::endl;
    std::cout << "Nodes:              " << nNodes << std::endl;
    std::cout << "Flows:              " << actualFlows << std::endl;
    std::cout << "PacketsPerSec:      " << packetsPerSec << std::endl;
    std::cout << "Speed:              " << speed << " m/s" << std::endl;
    std::cout << "Sim Time:           " << simTime << " s" << std::endl;
    std::cout << "Total Tx Packets:   " << totalTxPackets << std::endl;
    std::cout << "Total Rx Packets:   " << totalRxPackets << std::endl;
    std::cout << "Total Lost Packets: " << totalLostPackets << std::endl;
    std::cout << "Packet Delivery:    " << pdr << " %" << std::endl;
    std::cout << "Packet Drop Ratio:  " << dropRatio << " %" << std::endl;
    std::cout << "Total Throughput:   " << totalThroughput << " Mbps" << std::endl;
    std::cout << "Mean Delay:         " << overallDelay << " ms" << std::endl;
    std::cout << "Energy Consumed:    " << totalEnergy << " J" << std::endl;

    Simulator::Destroy();
    std::cout << "\nSimulation finished." << std::endl;
    return 0;
}
