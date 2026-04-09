#!/usr/bin/env python3


import csv
import os
import sys
from collections import defaultdict

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    HAS_MPL = True
except ImportError:
    HAS_MPL = False

CSV_PATH = "results/paper_topology/paper_topology_results.csv"
OUT_DIR  = "results/paper_topology"

def load_data(csv_path):
    rows = []
    with open(csv_path) as f:
        reader = csv.DictReader(f)
        for r in reader:
            r["nFlows"]   = int(r["nFlows"])
            r["seed"]     = int(r["seed"])
            for k in ("txPackets","rxPackets","lostPackets","retransmissions"):
                r[k] = int(r[k])
            for k in ("lossRate","throughput_Mbps","delay_ms","jitter_ms"):
                r[k] = float(r[k])
            rows.append(r)
    return rows

def aggregate(rows):
    """Group by (tcp, nFlows) and compute mean ± std for each metric."""
    groups = defaultdict(list)
    for r in rows:
        groups[(r["tcp"], r["nFlows"])].append(r)

    agg = []
    for (tcp, nf), items in sorted(groups.items(), key=lambda x: (x[0][0], x[0][1])):
        n = len(items)
        entry = {"tcp": tcp, "nFlows": nf, "n": n}
        for metric in ("lossRate","throughput_Mbps","delay_ms","jitter_ms","txPackets","rxPackets"):
            vals = [it[metric] for it in items]
            mean = sum(vals)/n
            std  = (sum((v-mean)**2 for v in vals)/max(n-1,1))**0.5
            entry[f"{metric}_mean"] = mean
            entry[f"{metric}_std"]  = std
        agg.append(entry)
    return agg

def print_table(agg):
    print(f"\n{'TCP':<10} {'Flows':>5}  {'Throughput (Mbps)':>20}  {'Loss (%)':>12}  {'Delay (ms)':>14}  {'Jitter (ms)':>14}")
    print("-"*85)
    for a in agg:
        print(f"{a['tcp']:<10} {a['nFlows']:>5}  "
              f"{a['throughput_Mbps_mean']:>8.3f} ± {a['throughput_Mbps_std']:>6.3f}  "
              f"{a['lossRate_mean']:>5.2f} ± {a['lossRate_std']:>4.2f}  "
              f"{a['delay_ms_mean']:>6.1f} ± {a['delay_ms_std']:>5.1f}  "
              f"{a['jitter_ms_mean']:>5.2f} ± {a['jitter_ms_std']:>4.2f}")

def plot_by_flows(agg, out_dir):
    if not HAS_MPL:
        print("[skip plots] matplotlib not available")
        return

    tcps = sorted(set(a["tcp"] for a in agg))
    colors  = {"ml": "C0", "plus": "C1", "newreno": "C2"}
    markers = {"ml": "o",  "plus": "s",  "newreno": "^"}
    labels  = {"ml": "WestwoodMl", "plus": "Westwood+", "newreno": "NewReno"}

    metrics = [
        ("throughput_Mbps", "Total Throughput (Mbps)", "throughput_vs_flows.png"),
        ("lossRate",        "Packet Loss Rate (%)",    "loss_vs_flows.png"),
        ("delay_ms",        "Mean Delay (ms)",         "delay_vs_flows.png"),
        ("jitter_ms",       "Mean Jitter (ms)",        "jitter_vs_flows.png"),
    ]

    for metric_key, ylabel, fname in metrics:
        fig, ax = plt.subplots(figsize=(8, 5))
        for tcp in tcps:
            sub = [a for a in agg if a["tcp"] == tcp]
            sub.sort(key=lambda a: a["nFlows"])
            xs    = [a["nFlows"] for a in sub]
            means = [a[f"{metric_key}_mean"] for a in sub]
            stds  = [a[f"{metric_key}_std"] for a in sub]
            ax.errorbar(xs, means, yerr=stds, label=labels.get(tcp, tcp),
                        marker=markers.get(tcp, "o"), color=colors.get(tcp),
                        capsize=4, linewidth=1.5)
        ax.set_xlabel("Number of Concurrent TCP Flows")
        ax.set_ylabel(ylabel)
        ax.set_title(f"{ylabel} vs Concurrent Flows (Paper Topology – MANET)")
        ax.legend()
        ax.grid(True, alpha=0.3)
        path = os.path.join(out_dir, fname)
        fig.tight_layout()
        fig.savefig(path, dpi=150)
        plt.close(fig)
        print(f"  Saved {path}")

def main():
    csv_path = CSV_PATH
    if len(sys.argv) > 1:
        csv_path = sys.argv[1]
    if not os.path.exists(csv_path):
        print(f"ERROR: {csv_path} not found. Run the simulation first.")
        sys.exit(1)

    rows = load_data(csv_path)
    agg  = aggregate(rows)

    print_table(agg)

    os.makedirs(OUT_DIR, exist_ok=True)
    plot_by_flows(agg, OUT_DIR)

    # Save aggregated CSV
    agg_csv = os.path.join(OUT_DIR, "paper_topology_aggregated.csv")
    with open(agg_csv, "w") as f:
        w = csv.writer(f)
        w.writerow(["tcp","nFlows","n","throughput_mean","throughput_std",
                     "loss_mean","loss_std","delay_mean","delay_std",
                     "jitter_mean","jitter_std"])
        for a in agg:
            w.writerow([a["tcp"], a["nFlows"], a["n"],
                        f"{a['throughput_Mbps_mean']:.4f}", f"{a['throughput_Mbps_std']:.4f}",
                        f"{a['lossRate_mean']:.4f}", f"{a['lossRate_std']:.4f}",
                        f"{a['delay_ms_mean']:.2f}", f"{a['delay_ms_std']:.2f}",
                        f"{a['jitter_ms_mean']:.4f}", f"{a['jitter_ms_std']:.4f}"])
    print(f"\n  Aggregated CSV: {agg_csv}")

if __name__ == "__main__":
    main()
