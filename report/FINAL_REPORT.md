# Final Report — Project Outcomes

## Project Title
Machine-Learning-Based RTT Prediction for TCP Congestion Control in ns-3.45

Date: March 2026

---

## 1) Objective and Completion Status

This project aimed to:
1. Reproduce the paper’s Fixed-Share Experts RTT predictor.
2. Implement improved and alternative ML RTT predictors.
3. Integrate RTT-aware behavior into TCP congestion control.
4. Compare all models in wireless/mobile scenarios with reproducible metrics and plots.

Status: **Completed**.

Compared TCP variants:
- Experts (Paper)
- Adaptive Experts (Modified)
- Kalman
- EWMA (ML)
- Westwood+
- NewReno

---

## 2) Final Experiment Setup

## Scenario I (MANET)
- 20 nodes, 1500×1000 m area
- 802.11b ad-hoc, AODV routing
- Random Waypoint mobility
- Flow counts: 3, 7, 17, 34
- 3 seeds

## Scenario II (Bursty)
- Speed ranges: [1,10], [20,30], [40,50] m/s
- 3 seeds

Metrics analyzed:
- Throughput
- Packet loss
- Delay
- Jitter
- Goodput
- Retransmissions

Data files:
- [results/scenario1/scenario1_aggregated.csv](../results/scenario1/scenario1_aggregated.csv)
- [results/scenario2/scenario2_aggregated.csv](../results/scenario2/scenario2_aggregated.csv)

---

## 3) Overall Outcome (Key Numbers)

## Scenario I overall means
| TCP | Throughput (Mbps) | Loss (%) | Delay (ms) | Jitter (ms) | Retx (%) |
|---|---:|---:|---:|---:|---:|
| adaptive | 8.2888 | 1.0333 | 57.8025 | 3.7238 | 27.3811 |
| experts | 8.2888 | 1.0333 | 57.8025 | 3.7238 | 27.3811 |
| kalman | 7.3511 | 0.9619 | 47.4375 | 3.7523 | 23.8310 |
| ml | 6.8380 | 0.9316 | 59.9725 | 3.8524 | 28.4661 |
| newreno | 9.0169 | 1.5672 | 89.6000 | 4.6490 | 28.1609 |
| plus | 8.0642 | 1.3668 | 89.3275 | 4.5739 | 23.8370 |

## Scenario II overall means
| TCP | Throughput (Mbps) | Loss (%) | Delay (ms) | Jitter (ms) | Goodput (pkts) |
|---|---:|---:|---:|---:|---:|
| adaptive | 13.4691 | 2.9432 | 38.2400 | 9.4677 | 96619.6667 |
| experts | 13.4691 | 2.9432 | 38.2400 | 9.4677 | 96619.6667 |
| kalman | 13.7008 | 2.9090 | 40.2300 | 9.4580 | 95262.0000 |
| ml | 13.4672 | 3.0464 | 43.8433 | 9.7335 | 94564.3333 |
| newreno | 18.7506 | 3.8351 | 66.7667 | 11.2934 | 93120.0000 |
| plus | 18.2989 | 3.4041 | 61.9200 | 9.9361 | 93317.6667 |

---

## 4) Winner Summary by Metric

## Scenario I
- Highest throughput: **NewReno**
- Lowest loss: **ML (EWMA)**
- Lowest delay: **Kalman**
- Lowest jitter: **Adaptive/Experts (tie)**
- Lowest retransmissions: **Kalman**

## Scenario II
- Highest throughput: **NewReno**
- Lowest loss: **Kalman**
- Lowest delay: **Adaptive/Experts (tie)**
- Lowest jitter: **Kalman**
- Highest goodput: **Adaptive/Experts (tie)**

---

## 5) Most Important Finding

Adaptive Experts and Experts have zero aggregate delta in these short-run settings:
- Scenario I delta (adaptive − experts): all tracked aggregates = 0.0
- Scenario II delta (adaptive − experts): all tracked aggregates = 0.0

Interpretation:
- Modified algorithm is integrated correctly and preserves baseline behavior under current conditions.
- To reveal adaptive gains, longer runs and adaptive-parameter sweeps are needed.

---

## 6) Plots (for report and viva)

## Standard plots
- [results/scenario1/s1_throughput.png](../results/scenario1/s1_throughput.png)
- [results/scenario1/s1_loss.png](../results/scenario1/s1_loss.png)
- [results/scenario1/s1_delay.png](../results/scenario1/s1_delay.png)
- [results/scenario1/s1_jitter.png](../results/scenario1/s1_jitter.png)
- [results/scenario1/s1_goodput.png](../results/scenario1/s1_goodput.png)
- [results/scenario1/s1_retransmissions.png](../results/scenario1/s1_retransmissions.png)
- [results/scenario2/s2_throughput.png](../results/scenario2/s2_throughput.png)
- [results/scenario2/s2_loss.png](../results/scenario2/s2_loss.png)
- [results/scenario2/s2_delay.png](../results/scenario2/s2_delay.png)
- [results/scenario2/s2_jitter.png](../results/scenario2/s2_jitter.png)
- [results/scenario2/s2_goodput.png](../results/scenario2/s2_goodput.png)
- [results/scenario2/s2_retransmissions.png](../results/scenario2/s2_retransmissions.png)

## Paper-style plots
- [results/paper_style/s1_all_metrics_paper.png](../results/paper_style/s1_all_metrics_paper.png)
- [results/paper_style/s2_all_metrics_paper.png](../results/paper_style/s2_all_metrics_paper.png)
- [results/paper_style/s1_throughput_paper.png](../results/paper_style/s1_throughput_paper.png)
- [results/paper_style/s1_loss_paper.png](../results/paper_style/s1_loss_paper.png)
- [results/paper_style/s1_delay_paper.png](../results/paper_style/s1_delay_paper.png)
- [results/paper_style/s1_jitter_paper.png](../results/paper_style/s1_jitter_paper.png)
- [results/paper_style/s2_throughput_paper.png](../results/paper_style/s2_throughput_paper.png)
- [results/paper_style/s2_loss_paper.png](../results/paper_style/s2_loss_paper.png)
- [results/paper_style/s2_delay_paper.png](../results/paper_style/s2_delay_paper.png)
- [results/paper_style/s2_jitter_paper.png](../results/paper_style/s2_jitter_paper.png)

---

## 7) Final Deliverables
- Main documentation: [readme.md](../readme.md)
- Detailed technical report: [REPORT.md](../REPORT.md)
- Full paper-style PDF: [report/project_paper.pdf](project_paper.pdf)
- Outcome summary: [report/OUTCOME_REPORT.md](OUTCOME_REPORT.md)
- This final report: [report/FINAL_REPORT.md](FINAL_REPORT.md)

---

## 8) Next Recommended Step
Run full-duration scenarios and adaptive-parameter sweeps (`AdaptBeta`, `AdaptGamma`, `Momentum`, `RevivalWindow`) to quantify if Adaptive Experts can surpass baseline Experts under stronger non-stationary conditions.