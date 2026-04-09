#!/bin/bash
# ═══════════════════════════════════════════════════════════════════
# run_all_assignment.sh – Master script for complete assignment
#
# Student ID: 2105155 | Std_id % 8 = 3
# Required topologies:
#   1. Wireless 802.11 (Mobile)
#   2. Wireless 802.15.4 (Static)
# Bonus:
#   A. Cross-Transmission (Wired + Wireless)
#   B. LTE Network
#
# Usage:
#   bash tools/run_all_assignment.sh          # short mode (~1hr)
#   bash tools/run_all_assignment.sh full     # full mode (~3hr)
# ═══════════════════════════════════════════════════════════════════

set -e
cd "$(dirname "$0")/.."

MODE="${1:-short}"
START_TIME=$(date +%s)

echo "═══════════════════════════════════════════════════════════════"
echo "  ASSIGNMENT SIMULATION RUNNER"
echo "  Student: 2105155 | Std_id % 8 = 3"
echo "  Mode: $MODE"
echo "  Start: $(date)"
echo "═══════════════════════════════════════════════════════════════"

# Step 1: Build
echo ""
echo ">>> Step 1: Building ns-3..."
./ns3 build 2>&1 | tail -5
echo "Build complete."

# Step 2: WiFi 802.11 Mobile (required topology #1)
echo ""
echo ">>> Step 2: Running WiFi 802.11 Mobile simulations..."
bash tools/run_wifi_mobile.sh "$MODE"

# Step 3: 802.15.4 Static (required topology #2)
echo ""
echo ">>> Step 3: Running 802.15.4 Static simulations..."
bash tools/run_wpan_static.sh "$MODE"

# Step 4: Bonus simulations
echo ""
echo ">>> Step 4: Running bonus simulations..."
bash tools/run_bonus.sh "$MODE"

# Step 5: Generate all plots
echo ""
echo ">>> Step 5: Generating plots..."
python3 tools/plot_assignment_results.py

# Step 6: Generate report
echo ""
echo ">>> Step 6: Generating PDF report..."
python3 tools/generate_assignment_report.py

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))
MINS=$((ELAPSED / 60))
SECS=$((ELAPSED % 60))

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  ALL DONE! Total time: ${MINS}m ${SECS}s"
echo "  Results:  results/assignment_plots/"
echo "  Report:   report/assignment_report.pdf"
echo "═══════════════════════════════════════════════════════════════"
