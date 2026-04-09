

#ifndef TCP_WESTWOOD_EXPERTS_ADAPTIVE_H
#define TCP_WESTWOOD_EXPERTS_ADAPTIVE_H

#include "tcp-westwood-plus.h"
#include "tcp-congestion-ops.h"
#include "tcp-recovery-ops.h"

#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/traced-value.h"

#include <vector>

namespace ns3
{

class Time;


class TcpWestwoodExpertsAdaptive : public TcpWestwoodPlus
{
  public:
    static TypeId GetTypeId(void);
    TcpWestwoodExpertsAdaptive();
    TcpWestwoodExpertsAdaptive(const TcpWestwoodExpertsAdaptive& other);
    virtual ~TcpWestwoodExpertsAdaptive();

  protected:
    virtual void PktsAcked(Ptr<TcpSocketState> tcb,
                           uint32_t segmentsAcked,
                           const Time& rtt) override;

    virtual void IncreaseWindow(Ptr<TcpSocketState> tcb,
                                uint32_t segmentsAcked) override;

    virtual Ptr<TcpCongestionOps> Fork() override;

  private:
    void InitExperts();
    void RescaleWeights();
    void ReviveDeadExperts();


    uint32_t m_nExperts;
    double   m_etaBase;
    double   m_alphaBase;
    double   m_rttMin;
    double   m_rttMax;


    double   m_adaptBeta;
    double   m_adaptGamma;
    double   m_momentum;
    uint32_t m_revivalWindow;
    double   m_revivalThreshold;


    std::vector<double> m_expertValues;
    std::vector<double> m_weights;
    bool   m_initialised;
    double m_predictedRttMs;
    double m_prevPredictedRttMs;
    double m_meanRttMs;
    double m_meanAlpha;


    double m_ratioThreshold;
    double m_baseRttMs;


    static constexpr double RESCALE_LOWER = 1e-100;
    static constexpr uint32_t RESCALE_INTERVAL = 50;
    uint32_t m_trialCount;
};

}

#endif
