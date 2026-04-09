

#include "tcp-westwood-experts-adaptive.h"
#include "ns3/log.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpWestwoodExpertsAdaptive");
NS_OBJECT_ENSURE_REGISTERED(TcpWestwoodExpertsAdaptive);

TypeId
TcpWestwoodExpertsAdaptive::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::TcpWestwoodExpertsAdaptive")
            .SetParent<TcpWestwoodPlus>()
            .SetGroupName("Internet")
            .AddConstructor<TcpWestwoodExpertsAdaptive>()

            .AddAttribute("NumExperts",
                          "Number of experts (N)",
                          UintegerValue(100),
                          MakeUintegerAccessor(&TcpWestwoodExpertsAdaptive::m_nExperts),
                          MakeUintegerChecker<uint32_t>(2, 1000))
            .AddAttribute("LearningRateBase",
                          "Base learning rate η_base",
                          DoubleValue(2.0),
                          MakeDoubleAccessor(&TcpWestwoodExpertsAdaptive::m_etaBase),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("ShareAlphaBase",
                          "Base sharing rate α_base",
                          DoubleValue(0.08),
                          MakeDoubleAccessor(&TcpWestwoodExpertsAdaptive::m_alphaBase),
                          MakeDoubleChecker<double>(0.0, 1.0))
            .AddAttribute("RttMin",
                          "Minimum expert RTT value (ms)",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&TcpWestwoodExpertsAdaptive::m_rttMin),
                          MakeDoubleChecker<double>(0.001))
            .AddAttribute("RttMax",
                          "Maximum expert RTT value (ms)",
                          DoubleValue(500.0),
                          MakeDoubleAccessor(&TcpWestwoodExpertsAdaptive::m_rttMax),
                          MakeDoubleChecker<double>(1.0))

            .AddAttribute("AdaptBeta",
                          "Scaling factor β for adaptive learning rate",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&TcpWestwoodExpertsAdaptive::m_adaptBeta),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("AdaptGamma",
                          "Scaling factor γ for adaptive sharing rate",
                          DoubleValue(0.5),
                          MakeDoubleAccessor(&TcpWestwoodExpertsAdaptive::m_adaptGamma),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("Momentum",
                          "Momentum blending factor μ [0, 1)",
                          DoubleValue(0.2),
                          MakeDoubleAccessor(&TcpWestwoodExpertsAdaptive::m_momentum),
                          MakeDoubleChecker<double>(0.0, 0.99))
            .AddAttribute("RevivalWindow",
                          "Trials between expert revival sweeps (W)",
                          UintegerValue(100),
                          MakeUintegerAccessor(&TcpWestwoodExpertsAdaptive::m_revivalWindow),
                          MakeUintegerChecker<uint32_t>(10, 10000))
            .AddAttribute("RevivalThreshold",
                          "Weight fraction below which experts are revived",
                          DoubleValue(0.001),
                          MakeDoubleAccessor(&TcpWestwoodExpertsAdaptive::m_revivalThreshold),
                          MakeDoubleChecker<double>(0.0, 1.0))
            .AddAttribute("RatioThreshold",
                          "Ratio (PredictedRTT / BaseRTT) above which cwnd is reduced",
                          DoubleValue(1.25),
                          MakeDoubleAccessor(&TcpWestwoodExpertsAdaptive::m_ratioThreshold),
                          MakeDoubleChecker<double>(1.0));
    return tid;
}



TcpWestwoodExpertsAdaptive::TcpWestwoodExpertsAdaptive()
    : m_nExperts(100),
      m_etaBase(2.0),
      m_alphaBase(0.08),
      m_rttMin(1.0),
      m_rttMax(500.0),
      m_adaptBeta(1.0),
      m_adaptGamma(0.5),
      m_momentum(0.2),
      m_revivalWindow(100),
      m_revivalThreshold(0.001),
      m_initialised(false),
      m_predictedRttMs(0.0),
      m_prevPredictedRttMs(0.0),
      m_meanRttMs(0.0),
      m_meanAlpha(0.05),
      m_ratioThreshold(1.25),
      m_baseRttMs(0.0),
      m_trialCount(0)
{
}

TcpWestwoodExpertsAdaptive::TcpWestwoodExpertsAdaptive(
    const TcpWestwoodExpertsAdaptive& other)
    : TcpWestwoodPlus(other),
      m_nExperts(other.m_nExperts),
      m_etaBase(other.m_etaBase),
      m_alphaBase(other.m_alphaBase),
      m_rttMin(other.m_rttMin),
      m_rttMax(other.m_rttMax),
      m_adaptBeta(other.m_adaptBeta),
      m_adaptGamma(other.m_adaptGamma),
      m_momentum(other.m_momentum),
      m_revivalWindow(other.m_revivalWindow),
      m_revivalThreshold(other.m_revivalThreshold),
      m_expertValues(other.m_expertValues),
      m_weights(other.m_weights),
      m_initialised(other.m_initialised),
      m_predictedRttMs(other.m_predictedRttMs),
      m_prevPredictedRttMs(other.m_prevPredictedRttMs),
      m_meanRttMs(other.m_meanRttMs),
      m_meanAlpha(other.m_meanAlpha),
      m_ratioThreshold(other.m_ratioThreshold),
      m_baseRttMs(other.m_baseRttMs),
      m_trialCount(other.m_trialCount)
{
}

TcpWestwoodExpertsAdaptive::~TcpWestwoodExpertsAdaptive()
{
}



void
TcpWestwoodExpertsAdaptive::InitExperts()
{
    m_expertValues.resize(m_nExperts);
    m_weights.resize(m_nExperts);

    double initWeight = 1.0 / static_cast<double>(m_nExperts);


    for (uint32_t i = 0; i < m_nExperts; ++i)
    {
        double iOneBased = static_cast<double>(i + 1);
        double n = static_cast<double>(m_nExperts);
        m_expertValues[i] = m_rttMin + m_rttMax * std::pow(2.0, (iOneBased - n) / 4.0);
        m_weights[i] = initWeight;
    }

    m_initialised = true;
    NS_LOG_INFO("Adaptive Experts initialised: N=" << m_nExperts
                << " η_base=" << m_etaBase << " α_base=" << m_alphaBase
                << " β=" << m_adaptBeta << " γ=" << m_adaptGamma
                << " μ=" << m_momentum << " W=" << m_revivalWindow
                << " range=[" << m_expertValues.front()
                << ", " << m_expertValues.back() << "] ms");
}



void
TcpWestwoodExpertsAdaptive::RescaleWeights()
{
    double maxW = *std::max_element(m_weights.begin(), m_weights.end());
    if (maxW < RESCALE_LOWER || std::isinf(1.0 / maxW))
    {
        double initW = 1.0 / static_cast<double>(m_nExperts);
        for (auto& w : m_weights)
            w = initW;
        return;
    }
    double scale = 1.0 / maxW;
    for (auto& w : m_weights)
        w *= scale;
}



void
TcpWestwoodExpertsAdaptive::ReviveDeadExperts()
{
    double sumW = 0.0;
    for (auto& w : m_weights)
        sumW += w;
    double meanW = sumW / static_cast<double>(m_nExperts);


    uint32_t revived = 0;
    double uniformW = 1.0 / static_cast<double>(m_nExperts);
    for (uint32_t i = 0; i < m_nExperts; ++i)
    {
        if (m_weights[i] < m_revivalThreshold * meanW)
        {
            m_weights[i] = uniformW;
            ++revived;
        }
    }

    if (revived > 0)
    {
        NS_LOG_INFO("Revived " << revived << " dead experts at trial " << m_trialCount);

        double newSum = 0.0;
        for (auto& w : m_weights)
            newSum += w;
        double normalise = sumW / newSum;
        for (auto& w : m_weights)
            w *= normalise;
    }
}



void
TcpWestwoodExpertsAdaptive::PktsAcked(Ptr<TcpSocketState> tcb,
                                       uint32_t segmentsAcked,
                                       const Time& rtt)
{
    if (rtt.IsZero())
        return;

    if (!m_initialised)
        InitExperts();

    double y_t = rtt.GetMilliSeconds();


    if (m_baseRttMs <= 0.0 || y_t < m_baseRttMs)
        m_baseRttMs = y_t;


    if (m_meanRttMs <= 0.0)
        m_meanRttMs = y_t;
    else
        m_meanRttMs = (1.0 - m_meanAlpha) * m_meanRttMs + m_meanAlpha * y_t;


    double sumWx = 0.0;
    double sumW = 0.0;
    for (uint32_t i = 0; i < m_nExperts; ++i)
    {
        sumWx += m_weights[i] * m_expertValues[i];
        sumW += m_weights[i];
    }
    double rawPrediction = (sumW > 0.0) ? (sumWx / sumW) : y_t;

    if (m_trialCount == 0)
    {
        m_predictedRttMs = rawPrediction;
    }
    else
    {

        m_predictedRttMs = (1.0 - m_momentum) * rawPrediction
                         + m_momentum * m_prevPredictedRttMs;
    }
    m_prevPredictedRttMs = m_predictedRttMs;


    double innovation = std::abs(y_t - m_predictedRttMs);
    double normInnovation = (m_meanRttMs > 0.0) ? (innovation / m_meanRttMs) : 0.0;

    normInnovation = std::min(normInnovation, 5.0);


    double eta_t = m_etaBase * (1.0 + m_adaptBeta * normInnovation);


    double alpha_t = std::min(m_alphaBase * (1.0 + m_adaptGamma * normInnovation), 0.5);

    NS_LOG_INFO("Trial " << m_trialCount << ": RTT=" << y_t
                << "ms  Predicted=" << m_predictedRttMs << "ms"
                << "  η_t=" << eta_t << "  α_t=" << alpha_t
                << "  innovation=" << innovation << "ms");


    for (uint32_t i = 0; i < m_nExperts; ++i)
    {
        double x_i = m_expertValues[i];
        double loss;
        if (x_i >= y_t)
        {
            loss = (x_i - y_t) * (x_i - y_t);
        }
        else
        {
            loss = 2.0 * y_t;
        }
        m_weights[i] *= std::exp(-eta_t * loss);

        if (m_weights[i] < 1e-300)
            m_weights[i] = 1e-300;
    }


    double pool = 0.0;
    for (uint32_t i = 0; i < m_nExperts; ++i)
    {
        pool += alpha_t * m_weights[i];
    }
    double poolPerExpert = pool / static_cast<double>(m_nExperts);
    for (uint32_t i = 0; i < m_nExperts; ++i)
    {
        m_weights[i] = (1.0 - alpha_t) * m_weights[i] + poolPerExpert;
    }

    ++m_trialCount;
    if (m_trialCount % RESCALE_INTERVAL == 0)
    {
        RescaleWeights();
    }
    if (m_trialCount % m_revivalWindow == 0)
    {
        ReviveDeadExperts();
    }


    TcpWestwoodPlus::PktsAcked(tcb, segmentsAcked, rtt);
}



void
TcpWestwoodExpertsAdaptive::IncreaseWindow(Ptr<TcpSocketState> tcb,
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
TcpWestwoodExpertsAdaptive::Fork()
{
    return CopyObject<TcpWestwoodExpertsAdaptive>(this);
}

}
