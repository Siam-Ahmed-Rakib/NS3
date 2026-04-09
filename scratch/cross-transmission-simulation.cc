/*
 * Usage:
 *   ./ns3 run "scratch/cross-transmission-simulation
 *     --tcp=experts --nWired=3 --nWireless=17 --nFlows=10
 *     --packetsPerSec=100 --simTime=100 --run=1"
 */

#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/bridge-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/yans-wifi-helper.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CrossTransmissionSimulation");

int
main(int argc, char* argv[])
{
    
    std::string tcp = "experts";
    uint32_t nWired = 3;
    uint32_t nWireless = 17;
    uint32_t nFlows = 10;
    uint32_t packetsPerSec = 100;
    double speed = 5.0;
    double simTime = 100.0;
    uint32_t packetSize = 1024;
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

    CommandLine cmd(__FILE__);
    cmd.AddValue("tcp", "TCP variant: experts, adaptive, kalman, ml, plus, newreno", tcp);
    cmd.AddValue("nWired", "Number of wired nodes", nWired);
    cmd.AddValue("nWireless", "Number of wireless nodes", nWireless);
    cmd.AddValue("nFlows", "Number of cross flows (wired->wireless + wireless->wired)", nFlows);
    cmd.AddValue("packetsPerSec", "Packets per second per flow", packetsPerSec);
    cmd.AddValue("speed", "Wireless node speed (m/s)", speed);
    cmd.AddValue("simTime", "Simulation time (s)", simTime);
    cmd.AddValue("packetSize", "Packet size (bytes)", packetSize);
    cmd.AddValue("run", "RNG run number", run);
    cmd.AddValue("nExperts", "Experts N", nExperts);
    cmd.AddValue("eta", "Learning rate", eta);
    cmd.AddValue("shareAlpha", "Sharing rate", shareAlpha);
    cmd.AddValue("rttMin", "Expert RTTmin (ms)", rttMin);
    cmd.AddValue("rttMax", "Expert RTTmax (ms)", rttMax);
    cmd.AddValue("ratioThreshold", "Threshold", ratioThreshold);
    cmd.AddValue("alpha", "EWMA alpha (ml mode)", alpha);
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

    
    NodeContainer wiredNodes;
    wiredNodes.Create(nWired);

    NodeContainer gatewayNode;
    gatewayNode.Create(1); 

    NodeContainer wirelessNodes;
    wirelessNodes.Create(nWireless);

  
    NodeContainer csmaNodes;
    csmaNodes.Add(wiredNodes);
    csmaNodes.Add(gatewayNode);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    NetDeviceContainer csmaDevices = csma.Install(csmaNodes);

   
    NodeContainer wifiStaNodes;
    wifiStaNodes.Add(gatewayNode);
    wifiStaNodes.Add(wirelessNodes);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue("DsssRate11Mbps"),
                                 "ControlMode", StringValue("DsssRate1Mbps"));

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");

    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer wifiDevices = wifi.Install(wifiPhy, wifiMac, wifiStaNodes);

   
    MobilityHelper wiredMobility;
    wiredMobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                       "MinX", DoubleValue(0.0),
                                       "MinY", DoubleValue(0.0),
                                       "DeltaX", DoubleValue(10.0),
                                       "DeltaY", DoubleValue(0.0),
                                       "GridWidth", UintegerValue(nWired + 1),
                                       "LayoutType", StringValue("RowFirst"));
    wiredMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    wiredMobility.Install(wiredNodes);

   
    MobilityHelper gwMobility;
    gwMobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue(static_cast<double>(nWired) * 10.0),
                                    "MinY", DoubleValue(0.0),
                                    "DeltaX", DoubleValue(0.0),
                                    "DeltaY", DoubleValue(0.0),
                                    "GridWidth", UintegerValue(1),
                                    "LayoutType", StringValue("RowFirst"));
    gwMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    gwMobility.Install(gatewayNode);

    
    MobilityHelper wirelessMobility;
    ObjectFactory posFactory;
    posFactory.SetTypeId("ns3::RandomRectanglePositionAllocator");
    posFactory.Set("X", StringValue("ns3::UniformRandomVariable[Min=30.0|Max=530.0]"));
    posFactory.Set("Y", StringValue("ns3::UniformRandomVariable[Min=-250.0|Max=250.0]"));
    Ptr<PositionAllocator> posAlloc = posFactory.Create()->GetObject<PositionAllocator>();

    std::string speedStr = "ns3::ConstantRandomVariable[Constant=" + std::to_string(speed) + "]";
    wirelessMobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                                      "Speed", StringValue(speedStr),
                                      "Pause", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"),
                                      "PositionAllocator", PointerValue(posAlloc));
    wirelessMobility.SetPositionAllocator(posAlloc);
    wirelessMobility.Install(wirelessNodes);

   
    BasicEnergySourceHelper energySourceHelper;
    energySourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(300.0));

  
    NodeContainer allWifiNodes;
    allWifiNodes.Add(gatewayNode);
    allWifiNodes.Add(wirelessNodes);
    energy::EnergySourceContainer sources = energySourceHelper.Install(allWifiNodes);

    WifiRadioEnergyModelHelper radioEnergyHelper;
    radioEnergyHelper.Install(wifiDevices, sources);

   
    AodvHelper aodv;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(wiredNodes);
    internet.Install(gatewayNode);
    internet.Install(wirelessNodes);

  
    Ipv4AddressHelper addressWired;
    addressWired.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer wiredInterfaces = addressWired.Assign(csmaDevices);

   
    Ipv4AddressHelper addressWireless;
    addressWireless.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer wirelessInterfaces = addressWireless.Assign(wifiDevices);

   
    uint16_t basePort = 9000;
    uint64_t dataRate = static_cast<uint64_t>(packetsPerSec) * packetSize * 8;
    std::string dataRateStr = std::to_string(dataRate) + "bps";

    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();

    uint32_t halfFlows = nFlows / 2;

   
    for (uint32_t f = 0; f < halfFlows; ++f)
    {
        uint32_t src = f % nWired; 
        rng->SetAttribute("Min", DoubleValue(0));
        rng->SetAttribute("Max", DoubleValue(static_cast<double>(nWireless) - 0.001));
        uint32_t dstIdx = static_cast<uint32_t>(rng->GetValue());

        uint16_t port = basePort + f;

      
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                    InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer sinkApp = sinkHelper.Install(wirelessNodes.Get(dstIdx));
        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(Seconds(simTime));

        
        Ipv4Address dstAddr = wirelessInterfaces.GetAddress(dstIdx + 1);
        OnOffHelper onoff("ns3::TcpSocketFactory",
                          InetSocketAddress(dstAddr, port));
        onoff.SetAttribute("DataRate", DataRateValue(DataRate(dataRateStr)));
        onoff.SetAttribute("PacketSize", UintegerValue(packetSize));
        onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

        ApplicationContainer srcApp = onoff.Install(wiredNodes.Get(src));
        srcApp.Start(Seconds(2.0 + f * 0.1));
        srcApp.Stop(Seconds(simTime));

        NS_LOG_UNCOND("CrossFlow " << f << " [WIRED->WIFI]: wired[" << src << "] -> wireless["
                                   << dstIdx << "] port=" << port);
    }

   
    for (uint32_t f = halfFlows; f < nFlows; ++f)
    {
        rng->SetAttribute("Min", DoubleValue(0));
        rng->SetAttribute("Max", DoubleValue(static_cast<double>(nWireless) - 0.001));
        uint32_t srcIdx = static_cast<uint32_t>(rng->GetValue());
        uint32_t dst = f % nWired; 

        uint16_t port = basePort + f;

        
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                    InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer sinkApp = sinkHelper.Install(wiredNodes.Get(dst));
        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(Seconds(simTime));

        
        Ipv4Address dstAddr = wiredInterfaces.GetAddress(dst);
        OnOffHelper onoff("ns3::TcpSocketFactory",
                          InetSocketAddress(dstAddr, port));
        onoff.SetAttribute("DataRate", DataRateValue(DataRate(dataRateStr)));
        onoff.SetAttribute("PacketSize", UintegerValue(packetSize));
        onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

        ApplicationContainer srcApp = onoff.Install(wirelessNodes.Get(srcIdx));
        srcApp.Start(Seconds(2.0 + f * 0.1));
        srcApp.Stop(Seconds(simTime));

        NS_LOG_UNCOND("CrossFlow " << f << " [WIFI->WIRED]: wireless[" << srcIdx << "] -> wired["
                                   << dst << "] port=" << port);
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

   
    double totalEnergy = 0.0;
    for (auto it = sources.Begin(); it != sources.End(); ++it)
    {
        double remaining = (*it)->GetRemainingEnergy();
        totalEnergy += (300.0 - remaining);
    }

    
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "TCP Variant:        " << tcp << std::endl;
    std::cout << "Wired Nodes:        " << nWired << std::endl;
    std::cout << "Wireless Nodes:     " << nWireless << std::endl;
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
