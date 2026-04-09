#!/bin/bash
# -------------------------------------------------------------------
#
# Usage:
#   bash tools/run_bonus.sh          # light  (30s sim)
#   bash tools/run_bonus.sh short    # short  (60s sim)
#   bash tools/run_bonus.sh full     # full   (100s sim)
# -------------------------------------------------------------------

cd "$(dirname "$0")/.."

TCPS=("experts" "adaptive" "kalman" "ml" "plus" "newreno")

if [[ "$1" == "full" ]]; then
    SIMTIME=100; TIMEOUT=300; COOLDOWN=5
    echo "[full mode] simTime=${SIMTIME}s  timeout=${TIMEOUT}s"
elif [[ "$1" == "short" ]]; then
    SIMTIME=60;  TIMEOUT=240; COOLDOWN=4
    echo "[short mode] simTime=${SIMTIME}s  timeout=${TIMEOUT}s"
else
    SIMTIME=30;  TIMEOUT=180; COOLDOWN=3
    echo "[light mode] simTime=${SIMTIME}s  timeout=${TIMEOUT}s"
fi

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
    pkill -9 -f "cross-transmission-simulation" 2>/dev/null || true
    pkill -9 -f "lte-bonus-simulation" 2>/dev/null || true
    pkill -9 -f "scratch_cross" 2>/dev/null || true
    pkill -9 -f "scratch_lte" 2>/dev/null || true
    sync 2>/dev/null || true
}

is_done() { [[ -f "$1" ]] && grep -q "Simulation finished" "$1" 2>/dev/null; }

run_one() {
    local sim_binary="$1"; local logfile="$2"; shift 2
    if is_done "$logfile"; then
        echo "    ✓ cached"
        parse_summary "$logfile"
        return 0
    fi
    cleanup_procs
    timeout --kill-after=20s "${TIMEOUT}s" \
        ./ns3 run "scratch/$sim_binary $*" \
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


OUTDIR="results/cross_transmission"
mkdir -p "$OUTDIR"

CSV_CROSS="$OUTDIR/cross_results.csv"
echo "sweep,tcp,nWired,nWireless,nFlows,packetsPerSec,speed,txPackets,rxPackets,lostPackets,pdr,dropRatio,throughput_Mbps,delay_ms,energy_J" > "$CSV_CROSS"

echo "===== BONUS A: Cross-Transmission (Wired + Wireless) ====="
NODES_TOTAL=(20 40 60 80 100)
count=0
total_cross=$(( ${#TCPS[@]} * ${#NODES_TOTAL[@]} ))

for tcp in "${TCPS[@]}"; do
  for ntotal in "${NODES_TOTAL[@]}"; do
    count=$((count + 1))
    nWired=$((ntotal / 4))
    nWireless=$((ntotal - nWired))
    nFlows=$((ntotal / 2))
    echo "[$count/$total_cross] cross tcp=$tcp nWired=$nWired nWireless=$nWireless"
    lf="$OUTDIR/log_${tcp}_n${ntotal}.txt"
    run_one "cross-transmission-simulation" "$lf" "--tcp=$tcp --nWired=$nWired --nWireless=$nWireless --nFlows=$nFlows --packetsPerSec=100 --speed=5 --simTime=$SIMTIME --run=1"
    echo "nodes,$tcp,$nWired,$nWireless,$nFlows,100,5,$txP,$rxP,$lostP,$pdr,$drop,$tput,$delay,$energy" >> "$CSV_CROSS"
  done
done

echo "Cross-transmission results → $CSV_CROSS"


OUTDIR_LTE="results/lte_bonus"
mkdir -p "$OUTDIR_LTE"

CSV_LTE="$OUTDIR_LTE/lte_results.csv"
echo "sweep,tcp,nNodes,nFlows,packetsPerSec,speed,txPackets,rxPackets,lostPackets,pdr,dropRatio,throughput_Mbps,delay_ms,energy_J" > "$CSV_LTE"

echo ""
echo "===== BONUS B: LTE Network ====="
LTE_NODES=(20 40 60 80 100)
count=0
total_lte=$(( ${#TCPS[@]} * ${#LTE_NODES[@]} ))

for tcp in "${TCPS[@]}"; do
  for n in "${LTE_NODES[@]}"; do
    count=$((count + 1))
    nF=$((n / 2))
    echo "[$count/$total_lte] lte tcp=$tcp nNodes=$n"
    lf="$OUTDIR_LTE/log_${tcp}_n${n}.txt"
    run_one "lte-bonus-simulation" "$lf" "--tcp=$tcp --nNodes=$n --nFlows=$nF --packetsPerSec=100 --speed=5 --simTime=$SIMTIME --run=1"
    echo "nodes,$tcp,$n,$nF,100,5,$txP,$rxP,$lostP,$pdr,$drop,$tput,$delay,$energy" >> "$CSV_LTE"
  done
done

echo ""
echo "════════════════════════════════════════"
echo "LTE results → $CSV_LTE"
echo "All bonus simulations done!"
echo "Next: python3 tools/plot_assignment_results.py"
