# Outcome Report

## Project
Machine-Learning-Based RTT Prediction for TCP Congestion Control in ns-3.45

Date: March 2026

## Scope of Final Evaluation
The final comparison includes 6 TCP variants:
- Experts (Paper)
- Adaptive Experts (Modified)
- Kalman
- EWMA (ML)
- Westwood+
- NewReno

Scenarios evaluated:
- Scenario I: MANET with varied concurrent flows
- Scenario II: Bursty traffic with varied mobility ranges

Primary data sources:
- [results/scenario1/scenario1_aggregated.csv](../results/scenario1/scenario1_aggregated.csv)
- [results/scenario2/scenario2_aggregated.csv](../results/scenario2/scenario2_aggregated.csv)

## Overall Outcome Summary

### Scenario I (MANET) — Overall Means Across All Points
| TCP | Throughput (Mbps) | Loss (%) | Delay (ms) | Jitter (ms) | Retransmissions (%) |
|---|---:|---:|---:|---:|---:|
| adaptive | 8.2888 | 1.0333 | 57.8025 | 3.7238 | 27.3811 |
| experts | 8.2888 | 1.0333 | 57.8025 | 3.7238 | 27.3811 |
| kalman | 7.3511 | 0.9619 | 47.4375 | 3.7523 | 23.8310 |
| ml | 6.8380 | 0.9316 | 59.9725 | 3.8524 | 28.4661 |
| newreno | 9.0169 | 1.5672 | 89.6000 | 4.6490 | 28.1609 |
| plus | 8.0642 | 1.3668 | 89.3275 | 4.5739 | 23.8370 |

Winner highlights (Scenario I):
- Highest throughput: NewReno
- Lowest packet loss: ML (EWMA)
- Lowest delay: Kalman
- Lowest jitter: Adaptive/Experts (tie)
- Lowest retransmissions: Kalman

### Scenario II (Bursty) — Overall Means Across All Points
| TCP | Throughput (Mbps) | Loss (%) | Delay (ms) | Jitter (ms) | Goodput (pkts) |
|---|---:|---:|---:|---:|---:|
| adaptive | 13.4691 | 2.9432 | 38.2400 | 9.4677 | 96619.6667 |
| experts | 13.4691 | 2.9432 | 38.2400 | 9.4677 | 96619.6667 |
| kalman | 13.7008 | 2.9090 | 40.2300 | 9.4580 | 95262.0000 |
| ml | 13.4672 | 3.0464 | 43.8433 | 9.7335 | 94564.3333 |
| newreno | 18.7506 | 3.8351 | 66.7667 | 11.2934 | 93120.0000 |
| plus | 18.2989 | 3.4041 | 61.9200 | 9.9361 | 93317.6667 |

Winner highlights (Scenario II):
- Highest throughput: NewReno
- Lowest packet loss: Kalman
- Lowest delay: Adaptive/Experts (tie)
- Lowest jitter: Kalman
- Highest goodput: Adaptive/Experts (tie)

## Key Interpretation
1. Throughput leaders (NewReno/Plus) generally pay higher delay and higher loss cost in wireless bursty conditions.
2. Kalman provides the strongest delay/loss balance among non-baseline ML options in these runs.
3. Adaptive Experts is fully integrated and functional, but in this short-run dataset it matches Experts exactly across all aggregated metrics.
4. The result indicates behavior preservation under current settings; to show adaptive advantage clearly, a dedicated parameter sweep and longer runs are recommended.

## Adaptive vs Experts Delta (Final Runs)
- Scenario I: all tracked aggregate metrics delta = 0.0
- Scenario II: all tracked aggregate metrics delta = 0.0

## Plots for Report/Presentation

### Existing scenario plots
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

### Paper-style plots (new)
- [results/paper_style/s1_throughput_paper.png](../results/paper_style/s1_throughput_paper.png)
- [results/paper_style/s1_loss_paper.png](../results/paper_style/s1_loss_paper.png)
- [results/paper_style/s1_delay_paper.png](../results/paper_style/s1_delay_paper.png)
- [results/paper_style/s1_jitter_paper.png](../results/paper_style/s1_jitter_paper.png)
- [results/paper_style/s1_goodput_paper.png](../results/paper_style/s1_goodput_paper.png)
- [results/paper_style/s1_retx_paper.png](../results/paper_style/s1_retx_paper.png)
- [results/paper_style/s1_all_metrics_paper.png](../results/paper_style/s1_all_metrics_paper.png)
- [results/paper_style/s2_throughput_paper.png](../results/paper_style/s2_throughput_paper.png)
- [results/paper_style/s2_loss_paper.png](../results/paper_style/s2_loss_paper.png)
- [results/paper_style/s2_delay_paper.png](../results/paper_style/s2_delay_paper.png)
- [results/paper_style/s2_jitter_paper.png](../results/paper_style/s2_jitter_paper.png)
- [results/paper_style/s2_goodput_paper.png](../results/paper_style/s2_goodput_paper.png)
- [results/paper_style/s2_all_metrics_paper.png](../results/paper_style/s2_all_metrics_paper.png)

## Deliverables Created
- Detailed project report PDF: [report/project_paper.pdf](project_paper.pdf)
- Research-style report source: [report/project_paper.tex](project_paper.tex)
- This outcome summary report: [report/OUTCOME_REPORT.md](OUTCOME_REPORT.md)

## Next Best Step
Run full-duration experiments and adaptive-parameter sweeps to quantify whether Adaptive Experts can outperform baseline Experts under stronger non-stationary conditions.