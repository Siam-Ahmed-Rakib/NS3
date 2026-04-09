

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/spectrum-module.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WpanStaticSimulation");

int
main(int argc, char* argv[])
{
    
    std::string tcp = "experts";
    uint32_t nNodes = 20;
    uint32_t nFlows = 10;
    uint32_t packetsPerSec = 100;
    uint32_t coverageMultiplier = 1;
    double simTime = 100.0;
    uint32_t packetSize = 80;
    uint32_t run = 1;
    double txRange = 30.0; 

   
    uint32_t nExperts = 100;
    double eta = 2.0;
    double shareAlpha = 0.08;
    double ratioThreshold = 1.25;
    double alpha = 0.25;
    double processNoise = 1.0;
    double measurementNoise = 10.0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("tcp", "TCP variant: experts, adaptive, kalman, ml, plus, newreno", tcp);
    cmd.AddValue("nNodes", "Number of static nodes", nNodes);
    cmd.AddValue("nFlows", "Number of flows", nFlows);
    cmd.AddValue("packetsPerSec", "Packets per second per flow", packetsPerSec);
    cmd.AddValue("coverageMultiplier", "Coverage area multiplier (x Tx_range)", coverageMultiplier);
    cmd.AddValue("simTime", "Simulation time (s)", simTime);
    cmd.AddValue("packetSize", "Packet size (bytes)", packetSize);
    cmd.AddValue("run", "RNG run number", run);
    cmd.AddValue("txRange", "Tx range (m)", txRange);
    cmd.AddValue("nExperts", "Number of experts N", nExperts);
    cmd.AddValue("eta", "Learning rate", eta);
    cmd.AddValue("shareAlpha", "Sharing rate", shareAlpha);
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
        Config::SetDefault("ns3::TcpWestwoodExperts::RttMin", DoubleValue(1.0));
        Config::SetDefault("ns3::TcpWestwoodExperts::RttMax", DoubleValue(500.0));
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
        Config::SetDefault("ns3::TcpWestwoodExpertsAdaptive::RttMin", DoubleValue(1.0));
        Config::SetDefault("ns3::TcpWestwoodExpertsAdaptive::RttMax", DoubleValue(500.0));
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

   
    NodeContainer nodes;
    nodes.Create(nNodes);

    
    double areaSize = coverageMultiplier * txRange;
    uint32_t gridWidth = static_cast<uint32_t>(std::ceil(std::sqrt(static_cast<double>(nNodes))));
    double spacing = (gridWidth > 1) ? (areaSize / (gridWidth - 1)) : 0.0;

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(std::max(spacing, 1.0)),
                                  "DeltaY", DoubleValue(std::max(spacing, 1.0)),
                                  "GridWidth", UintegerValue(gridWidth),
                                  "LayoutType", StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

   
    LrWpanHelper lrWpanHelper;
    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);
    lrWpanHelper.CreateAssociatedPan(lrwpanDevices, 0);

  
    SixLowPanHelper sixLowPanHelper;
    NetDeviceContainer sixDevices = sixLowPanHelper.Install(lrwpanDevices);

 
    InternetStackHelper internetv6;
    internetv6.SetIpv4StackInstall(false);
    internetv6.Install(nodes);

   
    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:f00d::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer interfaces = ipv6.Assign(sixDevices);

    
    for (uint32_t i = 0; i < interfaces.GetN(); i++)
    {
        interfaces.SetForwarding(i, true);
        interfaces.SetDefaultRouteInAllNodes(i);
    }

  
    BasicEnergySourceHelper energyHelper;
    energyHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0));
    energy::EnergySourceContainer sources = energyHelper.Install(nodes);

   
    uint16_t basePort = 9000;
    uint64_t dataRate = static_cast<uint64_t>(packetsPerSec) * packetSize * 8;
    std::string dataRateStr = std::to_string(dataRate) + "bps";

    Ptr<UniformRandomVariable> nodeRv = CreateObject<UniformRandomVariable>();
    nodeRv->SetAttribute("Min", DoubleValue(0));
    nodeRv->SetAttribute("Max", DoubleValue(static_cast<double>(nNodes) - 0.001));

  

    uint32_t actualFlows = nFlows;

    for (uint32_t f = 0; f < actualFlows; ++f)
    {
        uint32_t src = static_cast<uint32_t>(nodeRv->GetValue());
        uint32_t dst;
        do
        {
            dst = static_cast<uint32_t>(nodeRv->GetValue());
        } while (dst == src);

        uint16_t port = basePort + f;

      
        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                    Inet6SocketAddress(Ipv6Address::GetAny(), port));
        ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(dst));
        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(Seconds(simTime));

        
        Ipv6Address dstAddr = interfaces.GetAddress(dst, 1);
        OnOffHelper onoff("ns3::TcpSocketFactory",
                          Inet6SocketAddress(dstAddr, port));
        onoff.SetAttribute("DataRate", DataRateValue(DataRate(dataRateStr)));
        onoff.SetAttribute("PacketSize", UintegerValue(packetSize));
        onoff.SetAttribute("OnTime",
                           StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime",
                           StringValue("ns3::ConstantRandomVariable[Constant=0]"));

        ApplicationContainer srcApp = onoff.Install(nodes.Get(src));
        srcApp.Start(Seconds(2.0 + f * 0.2));
        srcApp.Stop(Seconds(simTime));

        NS_LOG_UNCOND("Flow " << f << ": node " << src << " -> node " << dst << " port=" << port);
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

    double pdr =
        (totalTxPackets > 0) ? (100.0 * totalRxPackets / totalTxPackets) : 0.0;
    double dropRatio =
        (totalTxPackets > 0) ? (100.0 * totalLostPackets / totalTxPackets) : 0.0;
    double overallDelay =
        (totalDelayCount > 0) ? (totalDelay / totalDelayCount) : 0.0;

   
    double txEnergyPerPkt = 0.000174;  
    double rxEnergyPerPkt = 0.000154; 
    double idlePowerW = 0.00128;      
    double totalEnergy = (totalTxPackets * txEnergyPerPkt) +
                         (totalRxPackets * rxEnergyPerPkt) +
                         (nNodes * simTime * idlePowerW);

    
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "TCP Variant:        " << tcp << std::endl;
    std::cout << "Nodes:              " << nNodes << std::endl;
    std::cout << "Flows:              " << actualFlows << std::endl;
    std::cout << "PacketsPerSec:      " << packetsPerSec << std::endl;
    std::cout << "CoverageMultiplier: " << coverageMultiplier << std::endl;
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
