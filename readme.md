# Project README (What Changed in ns-3 and Why)

## 1) Project Goal
This project extends ns-3 TCP congestion control to evaluate machine-learning-based RTT prediction in wireless/mobile scenarios, and compares 6 TCP variants:
- Experts (paper baseline)
- Adaptive Experts (modified paper algorithm)
- Kalman
- EWMA (ML)
- Westwood+
- NewReno

The goal is to show throughput/loss/delay/jitter trade-offs and explain exactly what was changed in base ns-3.

---

## 2) Exactly Where Base ns-3 Was Changed

## A) Core TCP model changes (inside ns-3 internet module)

### 1. New/updated ML TCP implementations
- `src/internet/model/tcp-westwood-ml.h`
- `src/internet/model/tcp-westwood-ml.cc`
- `src/internet/model/tcp-westwood-experts.h`
- `src/internet/model/tcp-westwood-experts.cc`
- `src/internet/model/tcp-westwood-kalman.h`
- `src/internet/model/tcp-westwood-kalman.cc`
- `src/internet/model/tcp-westwood-experts-adaptive.h`
- `src/internet/model/tcp-westwood-experts-adaptive.cc`

### 2. Build registration (required for ns-3 to compile new TCP types)
- `src/internet/CMakeLists.txt`

### Meaning of these changes
- Added RTT prediction logic in TCP ACK processing (`PktsAcked`).
- Added RTT-aware cwnd behavior in congestion avoidance (`IncreaseWindow`).
- Added proper socket cloning (`Fork`) so chosen TCP type is preserved per flow.
- Added tunable TypeId attributes for experiments (learning rates, thresholds, etc.).

---

## B) Scenario simulation changes (scratch)

### Scenario I (MANET, varied flows)
- `scratch/my-experts-simulation.cc`

### Scenario II (bursty traffic, varied speed)
- `scratch/my-bursty-simulation.cc`

### Meaning of these changes
- Added TCP selection for all 6 variants.
- Unified scenario parameters for fair comparison.
- Printed run summaries for metric extraction.

---

## C) Assignment topology simulations (Student ID 2105155, mod 8 = 3)

### Simulation files
- `scratch/wifi-mobile-simulation.cc` — WiFi 802.11b ad-hoc, AODV, RandomWaypoint mobility
- `scratch/wpan-static-simulation.cc` — IEEE 802.15.4 + 6LoWPAN, static grid
- `scratch/cross-transmission-simulation.cc` — Bonus: hybrid wired CSMA + WiFi (AODV cross-domain)
- `scratch/lte-bonus-simulation.cc` — Bonus: LTE/EPC cellular

### Runner scripts (laptop-safe: timeout, cooldown, resume)
- `tools/run_wifi_mobile.sh`
- `tools/run_wpan_static.sh`
- `tools/run_bonus.sh` (cross-transmission + LTE)

### Analysis and plotting
- `tools/plot_assignment_results.py` — Publication-quality graphs (auto-skips degenerate data)
- `tools/generate_assignment_report.py` — PDF report generator

### Meaning of these changes
- 4 parameter sweeps per topology (nodes, flows, packets/s, speed/coverage)
- 5 metrics: throughput, delay, PDR, drop ratio, energy
- 6 TCP variants compared across all sweeps

---

## D) Original paper experiment automation
- `tools/run_scenario1.sh`
- `tools/run_scenario2.sh`
- `tools/analyze_scenarios.py`
- `tools/make_paper_style_graphs.py`
- `tools/generate_project_paper_pdf.py`

### Meaning of these changes
- Batch execution across TCP variants, seeds, and scenario settings.
- Aggregation into CSV with mean/std.
- Plot generation (normal + paper-style).
- PDF generation for report submission.

---

## E) Documentation/report files
- `REPORT.md` (detailed technical report in repository root)
- `report/project_paper.pdf` (full paper-style report)
- `report/project_paper.tex` (LaTeX paper source)
- `report/assignment_report.pdf` (assignment topology report)
- `report/OUTCOME_REPORT.md` (final outcomes summary)
- `report/FINAL_REPORT.md` (final presentation-ready report)

---

## 3) What Each Algorithm Means

- **Experts (Paper):** Fixed-share experts model from reference paper.
- **Adaptive Experts:** Modified paper model with adaptive learning/share rates, momentum prediction, and expert revival.
- **Kalman:** RTT prediction as state estimation with adaptive noise.
- **EWMA (ML):** Lightweight smoothing predictor for RTT.
- **Westwood+:** Baseline bandwidth-estimation TCP.
- **NewReno:** Standard congestion-control baseline.

---

## 4) How to Reproduce the Full Pipeline

## Step 1: Build
```bash
./ns3 build
```

## Step 2: Run scenarios
```bash
bash tools/run_scenario1.sh short
bash tools/run_scenario2.sh short
```

## Step 3: Aggregate + plots
```bash
python3 tools/analyze_scenarios.py
python3 tools/make_paper_style_graphs.py
```

## Step 4: Generate PDF report
```bash
python3 tools/generate_project_paper_pdf.py
```

---

## 5) Where Final Outputs Are

### Assignment results (Student ID 2105155)
- `results/wifi_mobile/wifi_mobile_results.csv`
- `results/wpan_static/wpan_static_results.csv`
- `results/cross_transmission/cross_results.csv`
- `results/lte_bonus/lte_results.csv`
- `results/assignment_plots/*.png` (63 publication-quality graphs)
- `report/assignment_report.pdf`

### Original paper results
- `results/scenario1/scenario1_results.csv`
- `results/scenario1/scenario1_aggregated.csv`
- `results/scenario2/scenario2_results.csv`
- `results/scenario2/scenario2_aggregated.csv`

### Standard comparison plots
- `results/scenario1/*.png`
- `results/scenario2/*.png`

### Paper-style plots
- `results/paper_style/*.png`

### Reports
- `report/project_paper.pdf`
- `report/assignment_report.pdf`
- `report/OUTCOME_REPORT.md`
- `report/FINAL_REPORT.md`

---

## 6) Important Note (Data Preservation)
Scenario-generated data, logs, CSVs, and plots are intentionally kept for verification/proof during review with your sir.
Only temporary cache files were removed during cleanup.