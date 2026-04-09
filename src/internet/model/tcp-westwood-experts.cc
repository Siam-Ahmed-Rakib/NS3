

#include "tcp-westwood-experts.h"
#include "ns3/log.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpWestwoodExperts");
NS_OBJECT_ENSURE_REGISTERED(TcpWestwoodExperts);

TypeId
TcpWestwoodExperts::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::TcpWestwoodExperts")
            .SetParent<TcpWestwoodPlus>()
            .SetGroupName("Internet")
            .AddConstructor<TcpWestwoodExperts>()
            .AddAttribute("NumExperts",
                          "Number of experts (N)",
                          UintegerValue(100),
                          MakeUintegerAccessor(&TcpWestwoodExperts::m_nExperts),
                          MakeUintegerChecker<uint32_t>(2, 1000))
            .AddAttribute("LearningRate",
                          "Learning rate η for exponential weight updates",
                          DoubleValue(2.0),
                          MakeDoubleAccessor(&TcpWestwoodExperts::m_eta),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("ShareAlpha",
                          "Sharing rate α for weight redistribution",
                          DoubleValue(0.08),
                          MakeDoubleAccessor(&TcpWestwoodExperts::m_shareAlpha),
                          MakeDoubleChecker<double>(0.0, 1.0))
            .AddAttribute("RttMin",
                          "Minimum expert RTT value (ms)",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&TcpWestwoodExperts::m_rttMin),
                          MakeDoubleChecker<double>(0.001))
            .AddAttribute("RttMax",
                          "Maximum expert RTT value (ms)",
                          DoubleValue(500.0),
                          MakeDoubleAccessor(&TcpWestwoodExperts::m_rttMax),
                          MakeDoubleChecker<double>(1.0))
            .AddAttribute("RatioThreshold",
                          "Ratio (PredictedRTT / BaseRTT) above which cwnd is reduced",
                          DoubleValue(1.25),
                          MakeDoubleAccessor(&TcpWestwoodExperts::m_ratioThreshold),
                          MakeDoubleChecker<double>(1.0));
    return tid;
}



TcpWestwoodExperts::TcpWestwoodExperts()
    : m_nExperts(100),
      m_eta(2.0),
      m_shareAlpha(0.08),
      m_rttMin(1.0),
      m_rttMax(500.0),
      m_initialised(false),
      m_predictedRttMs(0.0),
      m_ratioThreshold(1.25),
      m_baseRttMs(0.0),
      m_trialCount(0)
{
}

TcpWestwoodExperts::TcpWestwoodExperts(const TcpWestwoodExperts& other)
    : TcpWestwoodPlus(other),
      m_nExperts(other.m_nExperts),
      m_eta(other.m_eta),
      m_shareAlpha(other.m_shareAlpha),
      m_rttMin(other.m_rttMin),
      m_rttMax(other.m_rttMax),
      m_expertValues(other.m_expertValues),
      m_weights(other.m_weights),
      m_initialised(other.m_initialised),
      m_predictedRttMs(other.m_predictedRttMs),
      m_ratioThreshold(other.m_ratioThreshold),
      m_baseRttMs(other.m_baseRttMs),
      m_trialCount(other.m_trialCount)
{
}

TcpWestwoodExperts::~TcpWestwoodExperts()
{
}



void
TcpWestwoodExperts::InitExperts()
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
    NS_LOG_INFO("Experts initialised: N=" << m_nExperts << " η=" << m_eta
                                          << " α=" << m_shareAlpha
                                          << " range=[" << m_expertValues.front()
                                          << ", " << m_expertValues.back() << "] ms");
}



void
TcpWestwoodExperts::RescaleWeights()
{
    double maxW = *std::max_element(m_weights.begin(), m_weights.end());
    if (maxW < RESCALE_LOWER || std::isinf(1.0 / maxW))
    {

        double initW = 1.0 / static_cast<double>(m_nExperts);
        for (auto& w : m_weights)
            w = initW;
        NS_LOG_DEBUG("Weights reinitialised (all collapsed)");
        return;
    }
    double scale = 1.0 / maxW;
    for (auto& w : m_weights)
        w *= scale;
}



void
TcpWestwoodExperts::PktsAcked(Ptr<TcpSocketState> tcb,
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


    double sumWx = 0.0;
    double sumW = 0.0;
    for (uint32_t i = 0; i < m_nExperts; ++i)
    {
        sumWx += m_weights[i] * m_expertValues[i];
        sumW += m_weights[i];
    }
    m_predictedRttMs = (sumW > 0.0) ? (sumWx / sumW) : y_t;

    NS_LOG_INFO("Trial " << m_trialCount << ": RTT=" << y_t
                         << "ms  Predicted=" << m_predictedRttMs << "ms"
                         << "  BaseRTT=" << m_baseRttMs << "ms");


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
        m_weights[i] *= std::exp(-m_eta * loss);


        if (m_weights[i] < 1e-300)
            m_weights[i] = 1e-300;
    }


    double pool = 0.0;
    for (uint32_t i = 0; i < m_nExperts; ++i)
    {
        pool += m_shareAlpha * m_weights[i];
    }
    double poolPerExpert = pool / static_cast<double>(m_nExperts);
    for (uint32_t i = 0; i < m_nExperts; ++i)
    {
        m_weights[i] = (1.0 - m_shareAlpha) * m_weights[i] + poolPerExpert;
    }


    ++m_trialCount;
    if (m_trialCount % RESCALE_INTERVAL == 0)
    {
        RescaleWeights();
    }


    TcpWestwoodPlus::PktsAcked(tcb, segmentsAcked, rtt);
}



void
TcpWestwoodExperts::IncreaseWindow(Ptr<TcpSocketState> tcb,
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
TcpWestwoodExperts::Fork()
{
    return CopyObject<TcpWestwoodExperts>(this);
}

}
