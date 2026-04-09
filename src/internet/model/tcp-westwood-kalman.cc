

#include "tcp-westwood-kalman.h"
#include "ns3/log.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpWestwoodKalman");
NS_OBJECT_ENSURE_REGISTERED(TcpWestwoodKalman);

TypeId
TcpWestwoodKalman::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::TcpWestwoodKalman")
            .SetParent<TcpWestwoodPlus>()
            .SetGroupName("Internet")
            .AddConstructor<TcpWestwoodKalman>()
            .AddAttribute("ProcessNoise",
                          "Process noise variance Q (ms²). Models how much the "
                          "true RTT can change between consecutive ACKs.",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&TcpWestwoodKalman::m_processNoise),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("MeasurementNoise",
                          "Initial measurement noise variance R (ms²). Models "
                          "the jitter/noise in individual RTT samples.",
                          DoubleValue(10.0),
                          MakeDoubleAccessor(&TcpWestwoodKalman::m_measurementNoise),
                          MakeDoubleChecker<double>(0.001))
            .AddAttribute("AdaptiveNoise",
                          "Enable adaptive measurement noise estimation. "
                          "If true, R is updated from observed innovations.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&TcpWestwoodKalman::m_adaptiveNoise),
                          MakeBooleanChecker())
            .AddAttribute("AdaptAlpha",
                          "Smoothing factor for adaptive noise estimation (0-1).",
                          DoubleValue(0.1),
                          MakeDoubleAccessor(&TcpWestwoodKalman::m_adaptAlpha),
                          MakeDoubleChecker<double>(0.0, 1.0))
            .AddAttribute("RatioThreshold",
                          "Ratio (PredictedRTT / BaseRTT) above which cwnd is reduced.",
                          DoubleValue(1.25),
                          MakeDoubleAccessor(&TcpWestwoodKalman::m_ratioThreshold),
                          MakeDoubleChecker<double>(1.0));
    return tid;
}



TcpWestwoodKalman::TcpWestwoodKalman()
    : m_processNoise(1.0),
      m_measurementNoise(10.0),
      m_stateEstimate(0.0),
      m_errorCovariance(100.0),
      m_kalmanGain(0.0),
      m_initialised(false),
      m_innovationEma(0.0),
      m_adaptAlpha(0.1),
      m_adaptiveNoise(true),
      m_ratioThreshold(1.25),
      m_baseRttMs(0.0),
      m_predictedRttMs(0.0)
{
}

TcpWestwoodKalman::TcpWestwoodKalman(const TcpWestwoodKalman& other)
    : TcpWestwoodPlus(other),
      m_processNoise(other.m_processNoise),
      m_measurementNoise(other.m_measurementNoise),
      m_stateEstimate(other.m_stateEstimate),
      m_errorCovariance(other.m_errorCovariance),
      m_kalmanGain(other.m_kalmanGain),
      m_initialised(other.m_initialised),
      m_innovationEma(other.m_innovationEma),
      m_adaptAlpha(other.m_adaptAlpha),
      m_adaptiveNoise(other.m_adaptiveNoise),
      m_ratioThreshold(other.m_ratioThreshold),
      m_baseRttMs(other.m_baseRttMs),
      m_predictedRttMs(other.m_predictedRttMs)
{
}

TcpWestwoodKalman::~TcpWestwoodKalman()
{
}



void
TcpWestwoodKalman::PktsAcked(Ptr<TcpSocketState> tcb,
                              uint32_t segmentsAcked,
                              const Time& rtt)
{
    if (rtt.IsZero())
        return;

    double z = rtt.GetMilliSeconds();


    if (m_baseRttMs <= 0.0 || z < m_baseRttMs)
        m_baseRttMs = z;


    if (!m_initialised)
    {
        m_stateEstimate = z;
        m_errorCovariance = m_measurementNoise;
        m_innovationEma = 0.0;
        m_initialised = true;
        m_predictedRttMs = z;
        NS_LOG_INFO("Kalman initialised: x̂=" << z << "ms  P=" << m_errorCovariance
                                              << "  Q=" << m_processNoise
                                              << "  R=" << m_measurementNoise);
        TcpWestwoodPlus::PktsAcked(tcb, segmentsAcked, rtt);
        return;
    }


    double xPrior = m_stateEstimate;
    double pPrior = m_errorCovariance + m_processNoise;


    double innovation = z - xPrior;


    if (m_adaptiveNoise)
    {
        double innovSq = innovation * innovation;
        m_innovationEma = m_adaptAlpha * innovSq
                        + (1.0 - m_adaptAlpha) * m_innovationEma;

        double estR = std::max(m_innovationEma - pPrior, 0.5);
        m_measurementNoise = estR;
    }


    m_kalmanGain = pPrior / (pPrior + m_measurementNoise);

    m_stateEstimate = xPrior + m_kalmanGain * innovation;
    m_errorCovariance = (1.0 - m_kalmanGain) * pPrior;


    if (m_stateEstimate < 0.1)
        m_stateEstimate = 0.1;

    m_predictedRttMs = m_stateEstimate;

    NS_LOG_INFO("Kalman: z=" << z << "ms  x̂=" << m_predictedRttMs
                             << "ms  K=" << m_kalmanGain
                             << "  P=" << m_errorCovariance
                             << "  R=" << m_measurementNoise
                             << "  base=" << m_baseRttMs << "ms");


    TcpWestwoodPlus::PktsAcked(tcb, segmentsAcked, rtt);
}



void
TcpWestwoodKalman::IncreaseWindow(Ptr<TcpSocketState> tcb,
                                  uint32_t segmentsAcked)
{

    if (tcb->m_cWnd < tcb->m_ssThresh)
    {
        tcb->m_cWnd += tcb->m_segmentSize;
        return;
    }


    if (m_predictedRttMs > 0.0 && m_baseRttMs > 0.0)
    {
        double ratio = m_predictedRttMs / m_baseRttMs;
        if (ratio > m_ratioThreshold)
        {
            NS_LOG_INFO("RTT ratio " << ratio << " > " << m_ratioThreshold
                                     << " → reducing cwnd by 10%");
            tcb->m_cWnd = static_cast<uint32_t>(tcb->m_cWnd * 0.9);
            return;
        }
    }


    tcb->m_cWnd += (tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd;
}



Ptr<TcpCongestionOps>
TcpWestwoodKalman::Fork()
{
    return CopyObject<TcpWestwoodKalman>(this);
}

}
