
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/yans-wifi-helper.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiMobileSimulation");

int
main(int argc, char* argv[])
{
   
    std::string tcp = "experts";
    uint32_t nNodes = 20;
    uint32_t nFlows = 10;
    uint32_t packetsPerSec = 100;
    double speed = 5.0;
    double simTime = 100.0;
    uint32_t packetSize = 1024;
    uint32_t run = 1;
    double areaX = 500.0;
    double areaY = 500.0;

    
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
    cmd.AddValue("nNodes", "Number of mobile nodes", nNodes);
    cmd.AddValue("nFlows", "Number of concurrent TCP flows", nFlows);
    cmd.AddValue("packetsPerSec", "Packets per second per flow", packetsPerSec);
    cmd.AddValue("speed", "Node speed (m/s)", speed);
    cmd.AddValue("simTime", "Simulation time (s)", simTime);
    cmd.AddValue("packetSize", "Packet size (bytes)", packetSize);
    cmd.AddValue("run", "RNG run number", run);
    cmd.AddValue("areaX", "Area width (m)", areaX);
    cmd.AddValue("areaY", "Area height (m)", areaY);
    cmd.AddValue("nExperts", "Number of experts N", nExperts);
    cmd.AddValue("eta", "Learning rate", eta);
    cmd.AddValue("shareAlpha", "Sharing rate", shareAlpha);
    cmd.AddValue("rttMin", "Expert RTTmin (ms)", rttMin);
    cmd.AddValue("rttMax", "Expert RTTmax (ms)", rttMax);
    cmd.AddValue("ratioThreshold", "PredictedRTT/BaseRTT threshold", ratioThreshold);
    cmd.AddValue("alpha", "EWMA alpha (ml mode)", alpha);
    cmd.AddValue("processNoise", "Kalman process noise Q", processNoise);
    cmd.AddValue("measurementNoise", "Kalman measurement noise R", measurementNoise);
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

    std::string speedStr =
        "ns3::ConstantRandomVariable[Constant=" + std::to_string(speed) + "]";
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

    
    BasicEnergySourceHelper energySourceHelper;
    energySourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(300.0));
    energy::EnergySourceContainer sources = energySourceHelper.Install(nodes);

    WifiRadioEnergyModelHelper radioEnergyHelper;
    radioEnergyHelper.Install(devices, sources);

    
    AodvHelper aodv;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.0.0", "255.255.0.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    
    uint16_t basePort = 9000;
    uint64_t dataRate = static_cast<uint64_t>(packetsPerSec) * packetSize * 8;
    std::string dataRateStr = std::to_string(dataRate) + "bps";

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

       
        OnOffHelper onoff("ns3::TcpSocketFactory",
                          InetSocketAddress(interfaces.GetAddress(dst), port));
        onoff.SetAttribute("DataRate", DataRateValue(DataRate(dataRateStr)));
        onoff.SetAttribute("PacketSize", UintegerValue(packetSize));
        onoff.SetAttribute("OnTime",
                           StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime",
                           StringValue("ns3::ConstantRandomVariable[Constant=0]"));

        ApplicationContainer srcApp = onoff.Install(nodes.Get(src));
        srcApp.Start(Seconds(1.0 + f * 0.1));
        srcApp.Stop(Seconds(simTime));

        NS_LOG_UNCOND("Flow " << f << ": node " << src << " -> node " << dst << " port=" << port
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
        double meanDelay = (flow.second.rxPackets > 0)
                               ? (flow.second.delaySum.GetMilliSeconds() /
                                  static_cast<double>(flow.second.rxPackets))
                               : 0.0;

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

    double pdr =
        (totalTxPackets > 0) ? (100.0 * totalRxPackets / totalTxPackets) : 0.0;
    double dropRatio =
        (totalTxPackets > 0) ? (100.0 * totalLostPackets / totalTxPackets) : 0.0;
    double overallDelay =
        (totalDelayCount > 0) ? (totalDelay / totalDelayCount) : 0.0;

    
    double totalEnergy = 0.0;
    for (auto it = sources.Begin(); it != sources.End(); ++it)
    {
        double remaining = (*it)->GetRemainingEnergy();
        totalEnergy += (300.0 - remaining);
    }

  
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "TCP Variant:        " << tcp << std::endl;
    std::cout << "Nodes:              " << nNodes << std::endl;
    std::cout << "Flows:              " << nFlows << std::endl;
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
