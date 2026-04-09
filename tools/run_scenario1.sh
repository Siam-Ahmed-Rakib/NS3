#!/bin/bash
# -------------------------------------------------------------------

# Usage:  bash tools/run_scenario1.sh [short]
# -------------------------------------------------------------------

set -e
cd "$(dirname "$0")/.."

OUTDIR="results/scenario1"
mkdir -p "$OUTDIR"

TCPS=("experts" "adaptive" "kalman" "ml" "plus" "newreno")

if [[ "$1" == "short" ]]; then
    FLOWS=(3 7 17 34)
    SEEDS=(1 2 3)
    SIMTIME=300   # 5 min
    echo "[short mode] simTime=${SIMTIME}s"
else
    FLOWS=(3 7 17 34 68 100 130)
    SEEDS=(1 2 3 4 5)
    SIMTIME=1500  # 25 min (paper)
    echo "[full mode] simTime=${SIMTIME}s"
fi

CSV="$OUTDIR/scenario1_results.csv"
echo "tcp,nFlows,seed,txPackets,rxPackets,lostPackets,retransmissions,lossRate,throughput_Mbps,delay_ms,jitter_ms" > "$CSV"

total=$((${#TCPS[@]} * ${#FLOWS[@]} * ${#SEEDS[@]}))
count=0

for tcp in "${TCPS[@]}"; do
  for nf in "${FLOWS[@]}"; do
    for seed in "${SEEDS[@]}"; do
      count=$((count + 1))
      echo "[$count/$total] tcp=$tcp nFlows=$nf seed=$seed ..."

      logfile="$OUTDIR/log_${tcp}_f${nf}_s${seed}.txt"

      ./ns3 run "scratch/my-experts-simulation \
        --tcp=$tcp --nFlows=$nf --simTime=$SIMTIME --run=$seed" \
        > "$logfile" 2>&1 || true

      summary=$(sed -n '/=== SUMMARY ===/,/Simulation finished/p' "$logfile")
      txP=$(echo "$summary" | grep "Total Tx Packets:" | awk '{print $NF}')
      rxP=$(echo "$summary" | grep "Total Rx Packets:" | awk '{print $NF}')
      lostP=$(echo "$summary" | grep "Total Lost Packets:" | awk '{print $NF}')
      retx=$(echo "$summary" | grep "Retransmissions:" | awk '{print $NF}')
      loss=$(echo "$summary" | grep "Packet Loss Rate:" | awk '{print $(NF-1)}')
      tput=$(echo "$summary" | grep "Total Throughput:" | awk '{print $(NF-1)}')
      delay=$(echo "$summary" | grep "Mean Delay:" | awk '{print $(NF-1)}')
      jitter=$(echo "$summary" | grep "Mean Jitter:" | awk '{print $(NF-1)}')

      echo "$tcp,$nf,$seed,${txP:-0},${rxP:-0},${lostP:-0},${retx:-0},${loss:-0},${tput:-0},${delay:-0},${jitter:-0}" >> "$CSV"
    done
  done
done

echo ""
echo "Done! Results → $CSV"
echo "Analyse: python3 tools/analyze_scenarios.py --scenario 1"
