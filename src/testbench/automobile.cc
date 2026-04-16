/**
 * @file automobile.cpp
 * @author Arthur
 * @brief Testbench for automotive zonal architecture simulation using ns-3.
 * Sets up a multi-gateway topology with sensors, backbone, and central ADCU.
 * Bypass layer 2 switching to do the point to point links and routing at
 * layer 3. It will be corrected to use layer 2 switching and TSNs scheduling in
 * the future.
 * @version 0.2
 * @date 2026-04-16
 *
 * @copyright Copyright (c) 2026
 */

//   Front Zone (Sensor Aggregation)           Back Zone / Central Server
//
//      [ Camera ]    [ Radar ]              [  ADCU  ]    [  Logs  ]
//         (n0)          (n1)                   (n4)          (n5)
//           |            |                      |             |
//     ------------------------            ------------------------
//     | Zonal Gateway 1 (n2) |            | Zonal Gateway 2 (n3) |
//     ------------------------            ------------------------
//                |                                   |
//                |            Backbone TSN           |
//                +===================================+
//                           (1 Gbps Link)

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AutomobileSimulation");

int main(int argc, char *argv[]) {
  // Simulation Configuration Parameters
  uint32_t backboneBandwidth = 1000; // Default: 1 Gbps (in Mbps)
  uint32_t backboneDelay = 10;       // in microseconds
  uint32_t gatewayBandwidth = 100;   // Default: 100 Mbps for local access
  uint32_t gatewayDelay = 50;        // in microseconds

  CommandLine cmd;
  cmd.AddValue("backboneBandwidth", "Bandwidth of the backbone TSN in Mbps",
               backboneBandwidth);
  cmd.AddValue("backboneDelay", "Delay of the backbone TSN in microseconds",
               backboneDelay);
  cmd.AddValue("gatewayBandwidth", "Bandwidth of the zonal gateways in Mbps",
               gatewayBandwidth);
  cmd.AddValue("gatewayDelay", "Delay of the zonal gateways in microseconds",
               gatewayDelay);
  cmd.Parse(argc, argv);

  // --- Node Creation ---
  NS_LOG_INFO("Creating nodes.");
  NodeContainer sensors;
  sensors.Create(2); // 0: Camera, 1: Radar

  NodeContainer logs;
  logs.Create(1); // Maintenance/Log terminal

  NodeContainer gateways;
  gateways.Create(2); // Zonal Gateways (n2 and n3)

  NodeContainer adcu;
  adcu.Create(1); // Autonomous Driving Control Unit (n4)

  // Physical Layer Setup (Point-to-Point)
  NS_LOG_INFO("Configuring physical links.");
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute(
      "DataRate", StringValue(std::to_string(gatewayBandwidth) + "Mbps"));
  p2p.SetChannelAttribute("Delay",
                          StringValue(std::to_string(gatewayDelay) + "us"));

  // Connect sensors and terminals to their respective gateways
  // (Each link is a separate NetDeviceContainer for L3 Routing)
  NetDeviceContainer devCam = p2p.Install(sensors.Get(0), gateways.Get(0));
  NetDeviceContainer devRad = p2p.Install(sensors.Get(1), gateways.Get(0));
  NetDeviceContainer devAdcu = p2p.Install(adcu.Get(0), gateways.Get(1));
  NetDeviceContainer devLog = p2p.Install(logs.Get(0), gateways.Get(1));

  // Backbone Configuration (High-speed link between gateways)
  p2p.SetDeviceAttribute(
      "DataRate", StringValue(std::to_string(backboneBandwidth) + "Mbps"));
  p2p.SetChannelAttribute("Delay",
                          StringValue(std::to_string(backboneDelay) + "us"));

  NetDeviceContainer devBackbone =
      p2p.Install(gateways.Get(0), gateways.Get(1));

  // Network Layer Setup (IP)
  NS_LOG_INFO("Installing Internet stack.");
  InternetStackHelper stack;
  stack.InstallAll(); // All nodes (endpoints + gateways) need IP for routing

  NS_LOG_INFO("Assigning IP addresses to end-devices.");
  Ipv4AddressHelper address;

  // Assigning unique subnets to each point-to-point link
  address.SetBase("10.1.1.0", "255.255.255.0");
  address.Assign(devCam);

  address.SetBase("10.1.2.0", "255.255.255.0");
  address.Assign(devRad);

  address.SetBase("10.1.3.0", "255.255.255.0");
  address.Assign(devBackbone);

  address.SetBase("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer adcuInterface = address.Assign(devAdcu);

  address.SetBase("10.1.5.0", "255.255.255.0");
  address.Assign(devLog);

  // Automatically build the routing tables to allow multi-hop communication
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // ADCU IP address for the clients
  Ipv4Address adcuIp = adcuInterface.GetAddress(0);

  // Application Setup (UDP Traffic)
  uint16_t port = 9;

  // UDP Server installed on the ADCU
  UdpServerHelper server(port);
  ApplicationContainer serverApp = server.Install(adcu.Get(0));
  serverApp.Start(Seconds(1.0));
  serverApp.Stop(Seconds(10.0));

  // Radar Client: Critical data, high frequency (10 Hz)
  UdpClientHelper radarClient(adcuIp, port);
  radarClient.SetAttribute("MaxPackets", UintegerValue(100));
  radarClient.SetAttribute("Interval", TimeValue(Seconds(0.1)));
  radarClient.SetAttribute("PacketSize", UintegerValue(512));
  ApplicationContainer radarApp = radarClient.Install(sensors.Get(1));
  radarApp.Start(Seconds(2.0));
  radarApp.Stop(Seconds(10.0));

  // Camera Client: High throughput data, lower frequency (2 Hz)
  UdpClientHelper camClient(adcuIp, port);
  camClient.SetAttribute("MaxPackets", UintegerValue(100));
  camClient.SetAttribute("Interval", TimeValue(Seconds(0.5)));
  camClient.SetAttribute("PacketSize", UintegerValue(1400));
  ApplicationContainer cameraApp = camClient.Install(sensors.Get(0));
  cameraApp.Start(Seconds(2.0));
  cameraApp.Stop(Seconds(10.0));

  // Tracing & PCAP
  // Enable PCAP on the Backbone link to observe traffic aggregation
  p2p.EnablePcap("auto-tsn-backbone", devBackbone.Get(0), true);

  // Simulation Execution and Cleanup
  NS_LOG_INFO("Running simulation.");
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}