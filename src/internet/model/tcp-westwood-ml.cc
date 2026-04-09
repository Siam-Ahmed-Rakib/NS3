

#include "tcp-westwood-ml.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpWestwoodMl");
NS_OBJECT_ENSURE_REGISTERED (TcpWestwoodMl);

TypeId
TcpWestwoodMl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpWestwoodMl")
    .SetParent<TcpWestwoodPlus> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpWestwoodMl> ()
    .AddAttribute ("Alpha",
                   "RTT prediction smoothing factor",
                   DoubleValue (0.25),
                   MakeDoubleAccessor (&TcpWestwoodMl::m_alpha),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RatioThreshold",
                   "Ratio (PredictedRTT / BaseRTT) above which cwnd is reduced",
                   DoubleValue (1.25),
                   MakeDoubleAccessor (&TcpWestwoodMl::m_ratioThreshold),
                   MakeDoubleChecker<double> ());
  return tid;
}

TcpWestwoodMl::TcpWestwoodMl ()
{
  m_predictedRtt = Time (Seconds (0));
  m_baseRtt = Time (Seconds (0));
  m_alpha = 0.25;
  m_ratioThreshold = 1.25;
}

TcpWestwoodMl::~TcpWestwoodMl () {}

void
TcpWestwoodMl::PktsAcked (Ptr<TcpSocketState> tcb,
                          uint32_t segmentsAcked,
                          const Time& rtt)
{
  NS_LOG_INFO("RTT=" << rtt.GetMilliSeconds()
             << " PredictedRTT=" << m_predictedRtt.GetMilliSeconds());

  if (rtt.IsZero ())
    return;

  if (m_baseRtt.IsZero () || rtt < m_baseRtt)
    m_baseRtt = rtt;

  if (m_predictedRtt.IsZero ())
    m_predictedRtt = rtt;
  else
    {
      m_predictedRtt = Time (
        Seconds (m_alpha * rtt.GetSeconds () +
                 (1 - m_alpha) * m_predictedRtt.GetSeconds ())
      );
    }

  TcpWestwoodPlus::PktsAcked (tcb, segmentsAcked, rtt);
}

void
TcpWestwoodMl::IncreaseWindow (Ptr<TcpSocketState> tcb,
                               uint32_t segmentsAcked)
{

  if (tcb->m_cWnd < tcb->m_ssThresh)
    {
      tcb->m_cWnd += tcb->m_segmentSize;
      return;
    }

  if (!m_predictedRtt.IsZero () && !m_baseRtt.IsZero ())
    {
      double ratio =
        m_predictedRtt.GetSeconds () / m_baseRtt.GetSeconds ();

      if (ratio > m_ratioThreshold)
        {
          tcb->m_cWnd *= 0.9;
          return;
        }
    }

  tcb->m_cWnd +=
    (tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd;
}

Ptr<TcpCongestionOps>
TcpWestwoodMl::Fork()
{
  return CopyObject<TcpWestwoodMl> (this);
}

}

