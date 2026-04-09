#!/bin/bash

# Usage:
#   bash tools/run_wpan_static.sh          # light  (30s sim)
#   bash tools/run_wpan_static.sh short    # short  (60s sim)
#   bash tools/run_wpan_static.sh full     # full   (100s sim)

cd "$(dirname "$0")/.."

OUTDIR="results/wpan_static"
mkdir -p "$OUTDIR"

TCPS=("experts" "adaptive" "kalman" "ml" "plus" "newreno")

if [[ "$1" == "full" ]]; then
    SIMTIME=100; TIMEOUT=300; COOLDOWN=5
    echo "[full mode] simTime=${SIMTIME}s  timeout=${TIMEOUT}s  cooldown=${COOLDOWN}s"
elif [[ "$1" == "short" ]]; then
    SIMTIME=60;  TIMEOUT=240; COOLDOWN=4
    echo "[short mode] simTime=${SIMTIME}s  timeout=${TIMEOUT}s  cooldown=${COOLDOWN}s"
else
    SIMTIME=30;  TIMEOUT=180; COOLDOWN=3
    echo "[light mode] simTime=${SIMTIME}s  timeout=${TIMEOUT}s  cooldown=${COOLDOWN}s"
fi

DEF_NODES=20
DEF_FLOWS=10
DEF_PPS=100
DEF_COV=1

NODES=(20 40 60 80 100)
FLOWS=(10 20 30 40 50)
PPS=(100 200 300 400 500)
COVERAGES=(1 2 3 4 5)

CSV="$OUTDIR/wpan_static_results.csv"

parse_summary() {
    local logfile="$1"
    txP=0; rxP=0; lostP=0; pdr=0; drop=0; tput=0; delay=0; energy=0
    [[ ! -f "$logfile" ]] && return
    local blk
    blk=$(sed -n '/=== SUMMARY ===/,/Simulation finished/p' "$logfile" 2>/dev/null)
    [[ -z "$blk" ]] && return
    txP=$(echo "$blk"  | grep "Total Tx Packets:"  | awk '{print $NF}')
    rxP=$(echo "$blk"  | grep "Total Rx Packets:"  | awk '{print $NF}')
    lostP=$(echo "$blk" | grep "Total Lost Packets:" | awk '{print $NF}')
    pdr=$(echo "$blk"  | grep "Packet Delivery:"   | awk '{print $(NF-1)}')
    drop=$(echo "$blk" | grep "Packet Drop Ratio:"  | awk '{print $(NF-1)}')
    tput=$(echo "$blk" | grep "Total Throughput:"   | awk '{print $(NF-1)}')
    delay=$(echo "$blk"| grep "Mean Delay:"         | awk '{print $(NF-1)}')
    energy=$(echo "$blk"|grep "Energy Consumed:"    | awk '{print $(NF-1)}')
    txP=${txP:-0}; rxP=${rxP:-0}; lostP=${lostP:-0}; pdr=${pdr:-0}
    drop=${drop:-0}; tput=${tput:-0}; delay=${delay:-0}; energy=${energy:-0}
}

cleanup_procs() {
    pkill -9 -f "wpan-static-simulation" 2>/dev/null || true
    pkill -9 -f "scratch_wpan-static-simulation" 2>/dev/null || true
    sync 2>/dev/null || true
}

is_done() { [[ -f "$1" ]] && grep -q "Simulation finished" "$1" 2>/dev/null; }

run_one() {
    local logfile="$1"; shift
    if is_done "$logfile"; then
        echo "    ✓ cached"
        parse_summary "$logfile"
        return 0
    fi
    cleanup_procs
    timeout --kill-after=20s "${TIMEOUT}s" \
        ./ns3 run "scratch/wpan-static-simulation $*" \
        > "$logfile" 2>&1
    local rc=$?
    if [[ $rc -eq 124 ]] || [[ $rc -eq 137 ]]; then
        echo "    ✗ TIMEOUT (killed after ${TIMEOUT}s)"
    elif [[ $rc -ne 0 ]]; then
        echo "    ✗ exit code $rc"
    else
        echo "    ✓ done"
    fi
    parse_summary "$logfile"
    cleanup_procs
    sleep "$COOLDOWN"
}

total=$(( ${#TCPS[@]} * (${#NODES[@]} + ${#FLOWS[@]} + ${#PPS[@]} + ${#COVERAGES[@]}) ))
count=0

echo ""
echo "Total runs: $total   |  CSV: $CSV"
echo "────────────────────────────────────────────────"

echo "sweep,tcp,nNodes,nFlows,packetsPerSec,coverageMultiplier,txPackets,rxPackets,lostPackets,pdr,dropRatio,throughput_Mbps,delay_ms,energy_J" > "$CSV"

echo ""; echo "═══ Sweep 1/4: Varying nNodes ═══"
for tcp in "${TCPS[@]}"; do
  for n in "${NODES[@]}"; do
    count=$((count + 1))
    echo "[$count/$total] nodes tcp=$tcp nNodes=$n"
    lf="$OUTDIR/log_nodes_${tcp}_n${n}.txt"
    run_one "$lf" "--tcp=$tcp --nNodes=$n --nFlows=$DEF_FLOWS --packetsPerSec=$DEF_PPS --coverageMultiplier=$DEF_COV --simTime=$SIMTIME --run=1"
    echo "nodes,$tcp,$n,$DEF_FLOWS,$DEF_PPS,$DEF_COV,$txP,$rxP,$lostP,$pdr,$drop,$tput,$delay,$energy" >> "$CSV"
  done
done

echo ""; echo "═══ Sweep 2/4: Varying nFlows ═══"
for tcp in "${TCPS[@]}"; do
  for nf in "${FLOWS[@]}"; do
    count=$((count + 1))
    echo "[$count/$total] flows tcp=$tcp nFlows=$nf"
    lf="$OUTDIR/log_flows_${tcp}_f${nf}.txt"
    run_one "$lf" "--tcp=$tcp --nNodes=$DEF_NODES --nFlows=$nf --packetsPerSec=$DEF_PPS --coverageMultiplier=$DEF_COV --simTime=$SIMTIME --run=1"
    echo "flows,$tcp,$DEF_NODES,$nf,$DEF_PPS,$DEF_COV,$txP,$rxP,$lostP,$pdr,$drop,$tput,$delay,$energy" >> "$CSV"
  done
done

echo ""; echo "═══ Sweep 3/4: Varying packetsPerSec ═══"
for tcp in "${TCPS[@]}"; do
  for pps in "${PPS[@]}"; do
    count=$((count + 1))
    echo "[$count/$total] pps tcp=$tcp packetsPerSec=$pps"
    lf="$OUTDIR/log_pps_${tcp}_p${pps}.txt"
    run_one "$lf" "--tcp=$tcp --nNodes=$DEF_NODES --nFlows=$DEF_FLOWS --packetsPerSec=$pps --coverageMultiplier=$DEF_COV --simTime=$SIMTIME --run=1"
    echo "pps,$tcp,$DEF_NODES,$DEF_FLOWS,$pps,$DEF_COV,$txP,$rxP,$lostP,$pdr,$drop,$tput,$delay,$energy" >> "$CSV"
  done
done

echo ""; echo "═══ Sweep 4/4: Varying coverage ═══"
for tcp in "${TCPS[@]}"; do
  for cov in "${COVERAGES[@]}"; do
    count=$((count + 1))
    echo "[$count/$total] coverage tcp=$tcp coverage=${cov}x"
    lf="$OUTDIR/log_cov_${tcp}_c${cov}.txt"
    run_one "$lf" "--tcp=$tcp --nNodes=$DEF_NODES --nFlows=$DEF_FLOWS --packetsPerSec=$DEF_PPS --coverageMultiplier=$cov --simTime=$SIMTIME --run=1"
    echo "coverage,$tcp,$DEF_NODES,$DEF_FLOWS,$DEF_PPS,$cov,$txP,$rxP,$lostP,$pdr,$drop,$tput,$delay,$energy" >> "$CSV"
  done
done

echo ""
echo "════════════════════════════════════════"
echo "All $count/$total runs completed."
echo "Results → $CSV"
echo "Next: python3 tools/plot_assignment_results.py"
