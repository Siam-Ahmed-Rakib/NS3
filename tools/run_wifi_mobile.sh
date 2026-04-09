#!/bin/bash

# Usage:
#   bash tools/run_wifi_mobile.sh          # light  (30s sim, safest)
#   bash tools/run_wifi_mobile.sh short    # short  (60s sim)
#   bash tools/run_wifi_mobile.sh full     # full   (100s sim)

cd "$(dirname "$0")/.."

OUTDIR="results/wifi_mobile"
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
DEF_SPEED=5

# Values to sweep
NODES=(20 40 60 80 100)
FLOWS=(10 20 30 40 50)
PPS=(100 200 300 400 500)
SPEEDS=(5 10 15 20 25)

CSV="$OUTDIR/wifi_mobile_results.csv"

# ── Helper: parse summary block from a log file ──
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

# ── Helper: kill leftover simulation processes ──
cleanup_procs() {
    pkill -9 -f "wifi-mobile-simulation" 2>/dev/null || true
    pkill -9 -f "scratch_wifi-mobile-simulation" 2>/dev/null || true
    sync 2>/dev/null || true
}

# ── Helper: check if run already succeeded ──
is_done() { [[ -f "$1" ]] && grep -q "Simulation finished" "$1" 2>/dev/null; }

# ── Run a single simulation safely ──
run_one() {
    local logfile="$1"; shift
    # Resume: skip if already completed
    if is_done "$logfile"; then
        echo "    ✓ cached"
        parse_summary "$logfile"
        return 0
    fi
    cleanup_procs
    # Run with hard timeout
    timeout --kill-after=20s "${TIMEOUT}s" \
        ./ns3 run "scratch/wifi-mobile-simulation $*" \
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

total=$(( ${#TCPS[@]} * (${#NODES[@]} + ${#FLOWS[@]} + ${#PPS[@]} + ${#SPEEDS[@]}) ))
count=0

echo ""
echo "Total runs: $total   |  CSV: $CSV"
echo "────────────────────────────────────────────────"

# Fresh CSV header
echo "sweep,tcp,nNodes,nFlows,packetsPerSec,speed,txPackets,rxPackets,lostPackets,pdr,dropRatio,throughput_Mbps,delay_ms,energy_J" > "$CSV"

# ── Sweep 1: Vary nNodes ──
echo ""; echo "═══ Sweep 1/4: Varying nNodes ═══"
for tcp in "${TCPS[@]}"; do
  for n in "${NODES[@]}"; do
    count=$((count + 1))
    echo "[$count/$total] nodes tcp=$tcp nNodes=$n"
    lf="$OUTDIR/log_nodes_${tcp}_n${n}.txt"
    run_one "$lf" "--tcp=$tcp --nNodes=$n --nFlows=$DEF_FLOWS --packetsPerSec=$DEF_PPS --speed=$DEF_SPEED --simTime=$SIMTIME --run=1"
    echo "nodes,$tcp,$n,$DEF_FLOWS,$DEF_PPS,$DEF_SPEED,$txP,$rxP,$lostP,$pdr,$drop,$tput,$delay,$energy" >> "$CSV"
  done
done

# ── Sweep 2: Vary nFlows ──
echo ""; echo "═══ Sweep 2/4: Varying nFlows ═══"
for tcp in "${TCPS[@]}"; do
  for nf in "${FLOWS[@]}"; do
    count=$((count + 1))
    echo "[$count/$total] flows tcp=$tcp nFlows=$nf"
    lf="$OUTDIR/log_flows_${tcp}_f${nf}.txt"
    run_one "$lf" "--tcp=$tcp --nNodes=$DEF_NODES --nFlows=$nf --packetsPerSec=$DEF_PPS --speed=$DEF_SPEED --simTime=$SIMTIME --run=1"
    echo "flows,$tcp,$DEF_NODES,$nf,$DEF_PPS,$DEF_SPEED,$txP,$rxP,$lostP,$pdr,$drop,$tput,$delay,$energy" >> "$CSV"
  done
done

# ── Sweep 3: Vary packetsPerSec ──
echo ""; echo "═══ Sweep 3/4: Varying packetsPerSec ═══"
for tcp in "${TCPS[@]}"; do
  for pps in "${PPS[@]}"; do
    count=$((count + 1))
    echo "[$count/$total] pps tcp=$tcp packetsPerSec=$pps"
    lf="$OUTDIR/log_pps_${tcp}_p${pps}.txt"
    run_one "$lf" "--tcp=$tcp --nNodes=$DEF_NODES --nFlows=$DEF_FLOWS --packetsPerSec=$pps --speed=$DEF_SPEED --simTime=$SIMTIME --run=1"
    echo "pps,$tcp,$DEF_NODES,$DEF_FLOWS,$pps,$DEF_SPEED,$txP,$rxP,$lostP,$pdr,$drop,$tput,$delay,$energy" >> "$CSV"
  done
done

# ── Sweep 4: Vary speed ──
echo ""; echo "═══ Sweep 4/4: Varying speed ═══"
for tcp in "${TCPS[@]}"; do
  for spd in "${SPEEDS[@]}"; do
    count=$((count + 1))
    echo "[$count/$total] speed tcp=$tcp speed=$spd"
    lf="$OUTDIR/log_speed_${tcp}_s${spd}.txt"
    run_one "$lf" "--tcp=$tcp --nNodes=$DEF_NODES --nFlows=$DEF_FLOWS --packetsPerSec=$DEF_PPS --speed=$spd --simTime=$SIMTIME --run=1"
    echo "speed,$tcp,$DEF_NODES,$DEF_FLOWS,$DEF_PPS,$spd,$txP,$rxP,$lostP,$pdr,$drop,$tput,$delay,$energy" >> "$CSV"
  done
done

echo ""
echo "════════════════════════════════════════"
echo "All $count/$total runs completed."
echo "Results → $CSV"
echo "Next: python3 tools/plot_assignment_results.py"
