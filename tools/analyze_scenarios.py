#!/usr/bin/env python3



import argparse
import csv
import os
import sys
from collections import defaultdict

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import matplotlib.ticker as ticker
    HAS_MPL = True
except ImportError:
    HAS_MPL = False

# ── colours / markers for 6 TCP variants ──
COLORS  = {"experts": "#d62728", "adaptive": "#8c564b", "kalman": "#9467bd", "ml": "#1f77b4", "plus": "#ff7f0e", "newreno": "#2ca02c"}
MARKERS = {"experts": "D", "adaptive": "X", "kalman": "P", "ml": "o", "plus": "s", "newreno": "^"}
LABELS  = {"experts": "Experts (Paper)", "adaptive": "Adaptive Experts (Modified)", "kalman": "Kalman Filter (New)", "ml": "EWMA (Simplified)",
           "plus": "Westwood+", "newreno": "NewReno"}


# ════════════════════════════════════════════════════════════════
#  SCENARIO 1 – MANET, x-axis = number of concurrent flows
# ════════════════════════════════════════════════════════════════

def analyze_scenario1():
    csv_path = "results/scenario1/scenario1_results.csv"
    out_dir  = "results/scenario1"
    if not os.path.exists(csv_path):
        print(f"[scenario1] {csv_path} not found – skipping")
        return

    rows = []
    with open(csv_path) as f:
        for r in csv.DictReader(f):
            r["nFlows"] = int(r["nFlows"])
            r["seed"]   = int(r["seed"])
            for k in ("txPackets","rxPackets","lostPackets","retransmissions"):
                r[k] = int(r[k])
            for k in ("lossRate","throughput_Mbps","delay_ms","jitter_ms"):
                r[k] = float(r[k])
            rows.append(r)

    groups = defaultdict(list)
    for r in rows:
        groups[(r["tcp"], r["nFlows"])].append(r)

    agg = []
    for (tcp, nf), items in sorted(groups.items()):
        n = len(items)
        entry = {"tcp": tcp, "nFlows": nf, "n": n}
        for m in ("lossRate","throughput_Mbps","delay_ms","jitter_ms","rxPackets","retransmissions"):
            vals = [it[m] for it in items]
            mean = sum(vals)/n
            std  = (sum((v-mean)**2 for v in vals)/max(n-1,1))**0.5
            entry[f"{m}_mean"] = mean
            entry[f"{m}_std"]  = std
        # retransmission percentage
        tx_sum = sum(it["txPackets"] for it in items)
        retx_sum = sum(it["retransmissions"] for it in items)
        entry["retx_pct"] = (100.0*retx_sum/tx_sum) if tx_sum else 0
        agg.append(entry)

    # Print table
    print("\n" + "="*110)
    print("SCENARIO I – MANET (varied concurrent flows)")
    print("="*110)
    print(f"{'TCP':<10} {'Flows':>5}  {'Throughput':>12}  {'Loss%':>10}  "
          f"{'Delay(ms)':>12}  {'Jitter(ms)':>12}  {'Retx%':>8}  {'Goodput':>10}")
    print("-"*110)
    for a in agg:
        print(f"{a['tcp']:<10} {a['nFlows']:>5}  "
              f"{a['throughput_Mbps_mean']:>6.2f}±{a['throughput_Mbps_std']:>4.2f}  "
              f"{a['lossRate_mean']:>5.2f}±{a['lossRate_std']:>4.2f}  "
              f"{a['delay_ms_mean']:>6.1f}±{a['delay_ms_std']:>5.1f}  "
              f"{a['jitter_ms_mean']:>5.2f}±{a['jitter_ms_std']:>4.2f}  "
              f"{a['retx_pct']:>7.2f}  "
              f"{a['rxPackets_mean']:>8.0f}")

    if not HAS_MPL:
        print("[skip plots] matplotlib not available")
        return

    tcps = sorted(set(a["tcp"] for a in agg))

    # ── Line plots (like paper Figures 1-4) ──
    metrics = [
        ("throughput_Mbps", "Total Throughput (Mbps)",
         "s1_throughput.png"),
        ("lossRate", "Packet Loss Rate (%)",
         "s1_loss.png"),
        ("delay_ms", "Mean Delay (ms)",
         "s1_delay.png"),
        ("jitter_ms", "Mean Jitter (ms)",
         "s1_jitter.png"),
        ("rxPackets", "Total Packets Delivered (Goodput)",
         "s1_goodput.png"),
        ("retransmissions", "Retransmissions",
         "s1_retransmissions.png"),
    ]

    for mk, ylabel, fname in metrics:
        fig, ax = plt.subplots(figsize=(9, 5))
        for tcp in tcps:
            sub = sorted([a for a in agg if a["tcp"] == tcp],
                         key=lambda a: a["nFlows"])
            xs    = [a["nFlows"] for a in sub]
            means = [a[f"{mk}_mean"] for a in sub]
            stds  = [a[f"{mk}_std"]  for a in sub]
            ax.errorbar(xs, means, yerr=stds,
                        label=LABELS.get(tcp, tcp),
                        marker=MARKERS.get(tcp, "o"),
                        color=COLORS.get(tcp),
                        capsize=4, linewidth=1.8)
        ax.set_xlabel("Number of Concurrent TCP Flows")
        ax.set_ylabel(ylabel)
        ax.set_title(f"Scenario I: {ylabel}")
        ax.legend()
        ax.grid(True, alpha=0.3)
        path = os.path.join(out_dir, fname)
        fig.tight_layout()
        fig.savefig(path, dpi=150)
        plt.close(fig)
        print(f"  Saved {path}")

   
    agg_csv = os.path.join(out_dir, "scenario1_aggregated.csv")
    with open(agg_csv, "w") as f:
        w = csv.writer(f)
        w.writerow(["tcp","nFlows","n","throughput_mean","throughput_std",
                     "loss_mean","loss_std","delay_mean","delay_std",
                     "jitter_mean","jitter_std","goodput_mean","retx_pct"])
        for a in agg:
            w.writerow([a["tcp"], a["nFlows"], a["n"],
                        f"{a['throughput_Mbps_mean']:.4f}", f"{a['throughput_Mbps_std']:.4f}",
                        f"{a['lossRate_mean']:.4f}", f"{a['lossRate_std']:.4f}",
                        f"{a['delay_ms_mean']:.2f}", f"{a['delay_ms_std']:.2f}",
                        f"{a['jitter_ms_mean']:.4f}", f"{a['jitter_ms_std']:.4f}",
                        f"{a['rxPackets_mean']:.0f}", f"{a['retx_pct']:.4f}"])
    print(f"  Aggregated CSV: {agg_csv}")


def analyze_scenario2():
    csv_path = "results/scenario2/scenario2_results.csv"
    out_dir  = "results/scenario2"
    if not os.path.exists(csv_path):
        print(f"[scenario2] {csv_path} not found – skipping")
        return

    rows = []
    with open(csv_path) as f:
        for r in csv.DictReader(f):
            r["minSpeed"] = int(r["minSpeed"])
            r["maxSpeed"] = int(r["maxSpeed"])
            r["seed"]     = int(r["seed"])
            for k in ("txPackets","rxPackets","lostPackets","retransmissions","goodput_pkts"):
                r[k] = int(r[k])
            for k in ("lossRate","throughput_Mbps","delay_ms","jitter_ms"):
                r[k] = float(r[k])
            r["speed_label"] = f"[{r['minSpeed']},{r['maxSpeed']}]"
            rows.append(r)

    groups = defaultdict(list)
    for r in rows:
        groups[(r["tcp"], r["speed_label"])].append(r)

    agg = []
    for (tcp, sl), items in sorted(groups.items()):
        n = len(items)
        entry = {"tcp": tcp, "speed_label": sl, "n": n}
        for m in ("lossRate","throughput_Mbps","delay_ms","jitter_ms","rxPackets","retransmissions"):
            vals = [it[m] for it in items]
            mean = sum(vals)/n
            std  = (sum((v-mean)**2 for v in vals)/max(n-1,1))**0.5
            entry[f"{m}_mean"] = mean
            entry[f"{m}_std"]  = std
        agg.append(entry)

    # Print table
    print("\n" + "="*110)
    print("SCENARIO II – BURSTY TRAFFIC (varied mobility speed)")
    print("="*110)
    print(f"{'TCP':<10} {'Speed':>10}  {'Throughput':>12}  {'Loss%':>10}  "
          f"{'Delay(ms)':>12}  {'Jitter(ms)':>12}  {'Goodput':>10}")
    print("-"*110)
    for a in agg:
        print(f"{a['tcp']:<10} {a['speed_label']:>10}  "
              f"{a['throughput_Mbps_mean']:>6.2f}±{a['throughput_Mbps_std']:>4.2f}  "
              f"{a['lossRate_mean']:>5.2f}±{a['lossRate_std']:>4.2f}  "
              f"{a['delay_ms_mean']:>6.1f}±{a['delay_ms_std']:>5.1f}  "
              f"{a['jitter_ms_mean']:>5.2f}±{a['jitter_ms_std']:>4.2f}  "
              f"{a['rxPackets_mean']:>8.0f}")

    if not HAS_MPL:
        print("[skip plots] matplotlib not available")
        return

    tcps = sorted(set(a["tcp"] for a in agg))
    speed_labels = ["[1,10]", "[20,30]", "[40,50]"]
    x_pos = list(range(len(speed_labels)))

   
    bar_metrics = [
        ("rxPackets",       "Total Packets Delivered (Goodput)", "s2_goodput.png"),
        ("throughput_Mbps", "Throughput (Mbps)",                 "s2_throughput.png"),
        ("lossRate",        "Packet Loss Rate (%)",              "s2_loss.png"),
        ("delay_ms",        "Mean Delay (ms)",                   "s2_delay.png"),
        ("jitter_ms",       "Mean Jitter (ms)",                  "s2_jitter.png"),
        ("retransmissions", "Retransmissions",                   "s2_retransmissions.png"),
    ]

    n_tcp = len(tcps)
    bar_width = 0.18

    for mk, ylabel, fname in bar_metrics:
        fig, ax = plt.subplots(figsize=(9, 5))
        for i, tcp in enumerate(tcps):
            sub = [a for a in agg if a["tcp"] == tcp]
            # Ensure same speed_label order
            sub_dict = {a["speed_label"]: a for a in sub}
            means = [sub_dict[sl][f"{mk}_mean"] if sl in sub_dict else 0 for sl in speed_labels]
            stds  = [sub_dict[sl][f"{mk}_std"]  if sl in sub_dict else 0 for sl in speed_labels]
            positions = [x + i * bar_width for x in x_pos]
            ax.bar(positions, means, bar_width, yerr=stds,
                   label=LABELS.get(tcp, tcp),
                   color=COLORS.get(tcp),
                   capsize=3, alpha=0.85)

        ax.set_xlabel("Node Velocity Range (m/s)")
        ax.set_ylabel(ylabel)
        ax.set_title(f"Scenario II (Bursty): {ylabel}")
        ax.set_xticks([x + bar_width * (n_tcp - 1) / 2 for x in x_pos])
        ax.set_xticklabels(speed_labels)
        ax.legend()
        ax.grid(True, axis="y", alpha=0.3)
        path = os.path.join(out_dir, fname)
        fig.tight_layout()
        fig.savefig(path, dpi=150)
        plt.close(fig)
        print(f"  Saved {path}")

    # Save aggregated CSV
    agg_csv = os.path.join(out_dir, "scenario2_aggregated.csv")
    with open(agg_csv, "w") as f:
        w = csv.writer(f)
        w.writerow(["tcp","speed","n","throughput_mean","throughput_std",
                     "loss_mean","loss_std","delay_mean","delay_std",
                     "jitter_mean","jitter_std","goodput_mean","goodput_std"])
        for a in agg:
            w.writerow([a["tcp"], a["speed_label"], a["n"],
                        f"{a['throughput_Mbps_mean']:.4f}", f"{a['throughput_Mbps_std']:.4f}",
                        f"{a['lossRate_mean']:.4f}", f"{a['lossRate_std']:.4f}",
                        f"{a['delay_ms_mean']:.2f}", f"{a['delay_ms_std']:.2f}",
                        f"{a['jitter_ms_mean']:.4f}", f"{a['jitter_ms_std']:.4f}",
                        f"{a['rxPackets_mean']:.0f}", f"{a['rxPackets_std']:.0f}"])
    print(f"  Aggregated CSV: {agg_csv}")



def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--scenario", type=int, choices=[1, 2], default=0,
                        help="Which scenario to analyse (0=both)")
    args = parser.parse_args()

    if args.scenario == 0 or args.scenario == 1:
        analyze_scenario1()
    if args.scenario == 0 or args.scenario == 2:
        analyze_scenario2()

if __name__ == "__main__":
    main()
