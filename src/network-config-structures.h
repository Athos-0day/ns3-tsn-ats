#ifndef SIMULATION_PARAMETERS_H
#define SIMULATION_PARAMETERS_H

#include "ns3/core-module.h"
#include <string>
#include <vector>

namespace ns3 {

/**
 * @brief Global simulation timing and logging configuration.
 */
struct SimulationConfig {
  Time start = Seconds(0.0);
  Time stop = Seconds(10.0);
  bool enableLogging = true;
};

/**
 * @brief Physical layer attributes for Ethernet links.
 */
struct LinkConfig {
  DataRate dataRate = DataRate("100Mb/s");
  Time propagationDelay = NanoSeconds(50);
};

/**
 * @brief Internal switch performance attributes.
 */
struct SwitchConfig {
  Time minLat = MicroSeconds(2);
  Time maxLat = MicroSeconds(5);
};

/**
 * @brief Detailed description of a network traffic stream.
 */
struct TrafficFlow {
  std::string name;
  uint32_t streamId;
  uint32_t pcp;
  uint32_t vlanId;
  uint32_t dei;
  Mac48Address destination;
  uint32_t payloadSize;
  uint32_t burstSize;
  Time period;
  Time interFrame;
  Time offset;
  Time jitter;
  Time startTime;
  Time stopTime;
};

/**
 * @brief Configuration for an End System (Node) containing multiple flows.
 */
struct EndSystemConfig {
  std::string nodeName;
  std::vector<TrafficFlow> flows;

  uint32_t GetMaxPcp() const {
    uint32_t maxPcp = 0;
    for (const auto &f : flows) {
      if (f.pcp > maxPcp)
        maxPcp = f.pcp;
    }
    return maxPcp;
  }
};

} // namespace ns3

#endif // SIMULATION_PARAMETERS_H