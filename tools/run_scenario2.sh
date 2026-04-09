#!/bin/bash

# Usage:  bash tools/run_scenario2.sh [short]

set -e
cd "$(dirname "$0")/.."

OUTDIR="results/scenario2"
mkdir -p "$OUTDIR"

TCPS=("experts" "adaptive" "kalman" "ml" "plus" "newreno")


SPEED_CONFIGS=("1,10" "20,30" "40,50")

if [[ "$1" == "short" ]]; then
    SEEDS=(1 2 3)
    SIMTIME=600    # 10 min 
    echo "[short mode] simTime=${SIMTIME}s"
else
    SEEDS=(1 2 3 4 5)
    SIMTIME=5400   # 90 min (paper)
    echo "[full mode] simTime=${SIMTIME}s"
fi

CSV="$OUTDIR/scenario2_results.csv"
echo "tcp,minSpeed,maxSpeed,seed,txPackets,rxPackets,lostPackets,retransmissions,lossRate,throughput_Mbps,goodput_pkts,delay_ms,jitter_ms" > "$CSV"

total=$((${#TCPS[@]} * ${#SPEED_CONFIGS[@]} * ${#SEEDS[@]}))
count=0

for tcp in "${TCPS[@]}"; do
  for spd in "${SPEED_CONFIGS[@]}"; do
    IFS=',' read -r minS maxS <<< "$spd"
    for seed in "${SEEDS[@]}"; do
      count=$((count + 1))
      echo "[$count/$total] tcp=$tcp speed=[$minS,$maxS] seed=$seed ..."

      logfile="$OUTDIR/log_${tcp}_v${minS}-${maxS}_s${seed}.txt"

      ./ns3 run "scratch/my-bursty-simulation \
        --tcp=$tcp --minSpeed=$minS --maxSpeed=$maxS \
        --simTime=$SIMTIME --run=$seed" \
        > "$logfile" 2>&1 || true

      summary=$(sed -n '/=== SUMMARY ===/,/Simulation finished/p' "$logfile")
      txP=$(echo "$summary" | grep "Total Tx Packets:" | awk '{print $NF}')
      rxP=$(echo "$summary" | grep "Total Rx Packets:" | awk '{print $NF}')
      lostP=$(echo "$summary" | grep "Total Lost Packets:" | awk '{print $NF}')
      retx=$(echo "$summary" | grep "Retransmissions:" | awk '{print $NF}')
      loss=$(echo "$summary" | grep "Packet Loss Rate:" | awk '{print $(NF-1)}')
      tput=$(echo "$summary" | grep "Total Throughput:" | awk '{print $(NF-1)}')
      goodput=$(echo "$summary" | grep "Goodput (Rx pkts):" | awk '{print $NF}')
      delay=$(echo "$summary" | grep "Mean Delay:" | awk '{print $(NF-1)}')
      jitter=$(echo "$summary" | grep "Mean Jitter:" | awk '{print $(NF-1)}')

      echo "$tcp,$minS,$maxS,$seed,${txP:-0},${rxP:-0},${lostP:-0},${retx:-0},${loss:-0},${tput:-0},${goodput:-0},${delay:-0},${jitter:-0}" >> "$CSV"
    done
  done
done

echo ""
echo "Done! Results → $CSV"
echo "Analyse: python3 tools/analyze_scenarios.py --scenario 2"
