

#ifndef TCP_WESTWOOD_KALMAN_H
#define TCP_WESTWOOD_KALMAN_H

#include "tcp-westwood-plus.h"
#include "tcp-congestion-ops.h"
#include "tcp-recovery-ops.h"

#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/traced-value.h"

namespace ns3
{

class Time;


class TcpWestwoodKalman : public TcpWestwoodPlus
{
  public:
    static TypeId GetTypeId(void);
    TcpWestwoodKalman();
    TcpWestwoodKalman(const TcpWestwoodKalman& other);
    virtual ~TcpWestwoodKalman();

  protected:
    virtual void PktsAcked(Ptr<TcpSocketState> tcb,
                           uint32_t segmentsAcked,
                           const Time& rtt) override;

    virtual void IncreaseWindow(Ptr<TcpSocketState> tcb,
                                uint32_t segmentsAcked) override;

    virtual Ptr<TcpCongestionOps> Fork() override;

  private:

    double m_processNoise;
    double m_measurementNoise;


    double m_stateEstimate;
    double m_errorCovariance;
    double m_kalmanGain;
    bool   m_initialised;


    double m_innovationEma;
    double m_adaptAlpha;
    bool   m_adaptiveNoise;


    double m_ratioThreshold;
    double m_baseRttMs;
    double m_predictedRttMs;
};

}

#endif
