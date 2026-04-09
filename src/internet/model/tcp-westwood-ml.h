



#ifndef TCP_WESTWOOD_ML_H
#define TCP_WESTWOOD_ML_H

#include "tcp-westwood-plus.h"

#include "tcp-congestion-ops.h"
#include "tcp-recovery-ops.h"

#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/traced-value.h"

namespace ns3
{

class Time;



 class TcpWestwoodMl : public TcpWestwoodPlus
{
public:
  static TypeId GetTypeId (void);
  TcpWestwoodMl ();
  virtual ~TcpWestwoodMl ();

protected:
  virtual void PktsAcked (Ptr<TcpSocketState> tcb,
                          uint32_t segmentsAcked,
                          const Time& rtt) override;

  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb,
                               uint32_t segmentsAcked) override;

  virtual Ptr<TcpCongestionOps> Fork() override;

private:
  Time m_predictedRtt;
  Time m_baseRtt;
  double m_alpha;
  double m_ratioThreshold;
};


}

#endif
