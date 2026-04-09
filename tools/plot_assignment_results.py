#!/usr/bin/env python3
"""

Output : results/assignment_plots/*.png
Usage  : python3 tools/plot_assignment_results.py
"""

import os
import sys
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np
from matplotlib.lines import Line2D



BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTDIR   = os.path.join(BASE_DIR, "results", "assignment_plots")
os.makedirs(OUTDIR, exist_ok=True)


TCP_COLORS = {
    "experts":  "#d62728",   # red
    "adaptive": "#1f77b4",   # blue
    "kalman":   "#2ca02c",   # green
    "ml":       "#9467bd",   # purple
    "plus":     "#ff7f0e",   # orange
    "newreno":  "#8c564b",   # brown
}
TCP_MARKERS  = {"experts":"o","adaptive":"s","kalman":"^","ml":"D","plus":"v","newreno":"P"}
TCP_LINES    = {"experts":"-","adaptive":"-","kalman":"-","ml":"-","plus":"-","newreno":"-"}
TCP_LABELS   = {
    "experts":  "Experts (Paper)",
    "adaptive": "Adaptive Experts",
    "kalman":   "Kalman Filter",
    "ml":       "EWMA / ML",
    "plus":     "Westwood+",
    "newreno":  "NewReno",
}

METRICS = {
    "throughput_Mbps": ("Network Throughput",       "Mbps"),
    "delay_ms":        ("End-to-End Delay",         "ms"),
    "pdr":             ("Packet Delivery Ratio",    "%"),
    "dropRatio":       ("Packet Drop Ratio",        "%"),
    "energy_J":        ("Energy Consumption",       "J"),
}

DPI = 200




def setup_style():
    plt.rcParams.update({
      
        "figure.figsize":     (11, 6.5),
        "figure.dpi":         DPI,
        "figure.facecolor":   "white",
        "figure.edgecolor":   "white",
        
        "font.size":          13,
        "font.family":        "sans-serif",
        "font.sans-serif":    ["DejaVu Sans", "Arial", "Helvetica"],
        "axes.labelsize":     15,
        "axes.titlesize":     16,
        "axes.titleweight":   "bold",
        "axes.labelweight":   "bold",
      
        "xtick.labelsize":    12,
        "ytick.labelsize":    12,
        "xtick.direction":    "in",
        "ytick.direction":    "in",
        "xtick.major.size":   5,
        "ytick.major.size":   5,
        
        "axes.grid":          True,
        "grid.color":         "#cccccc",
        "grid.linewidth":     0.6,
        "grid.alpha":         0.5,
        "grid.linestyle":     "--",
        
        "legend.fontsize":    11,
        "legend.framealpha":  0.95,
        "legend.edgecolor":   "#999999",
        "legend.fancybox":    True,
        
        "lines.linewidth":    2.4,
        "lines.markersize":   9,
        
        "axes.spines.top":    False,
        "axes.spines.right":  False,
    })




def _save(fig, fname):
    path = os.path.join(OUTDIR, fname)
    fig.savefig(path, dpi=DPI, bbox_inches="tight", facecolor="white")
    plt.close(fig)
    print(f"    ✓ {fname}")


def _is_degenerate(series) -> bool:
    """True when a numeric series is all-zero or all-NaN."""
    vals = pd.to_numeric(series, errors="coerce")
    return vals.isna().all() or (vals == 0).all()


def _smart_legend(ax):
    """Place a clean legend outside the axes if many items, else inside."""
    handles, labels = ax.get_legend_handles_labels()
    if len(handles) <= 4:
        ax.legend(loc="best")
    else:
        ax.legend(
            loc="upper left",
            bbox_to_anchor=(1.02, 1),
            borderaxespad=0,
        )



def plot_sweep(df, sweep_name, x_col, x_label, topology_name, out_prefix):
    sub = df[df["sweep"] == sweep_name].copy()
    if sub.empty:
        print(f"    [skip] no data for sweep={sweep_name}")
        return

    for mkey, (mname, munit) in METRICS.items():
        if mkey not in sub.columns or _is_degenerate(sub[mkey]):
            continue

        fig, ax = plt.subplots()

        for tcp in TCP_COLORS:
            td = sub[sub["tcp"] == tcp].sort_values(x_col)
            if td.empty:
                continue
            x = td[x_col].values
            y = pd.to_numeric(td[mkey], errors="coerce").values
            ax.plot(
                x, y,
                color=TCP_COLORS[tcp],
                marker=TCP_MARKERS[tcp],
                linestyle=TCP_LINES[tcp],
                label=TCP_LABELS[tcp],
                markeredgecolor="white",
                markeredgewidth=0.8,
            )

        ax.set_xlabel(x_label)
        ax.set_ylabel(f"{mname} ({munit})")
        ax.set_title(f"{topology_name}:  {mname}  vs  {x_label}")
        ax.set_xticks(sorted(sub[x_col].unique()))
        _smart_legend(ax)

        _save(fig, f"{out_prefix}_{sweep_name}_{mkey}.png")



def plot_bar(df, topology_name, out_prefix, skip_metrics=None):
    """
    Grouped bar chart at default (smallest sweep value).
    `skip_metrics` – set of metric keys to skip (e.g. degenerate data).
    """
    skip_metrics = skip_metrics or set()
    sample = df.groupby("tcp").first().reset_index()
    if sample.empty:
        return

    for mkey, (mname, munit) in METRICS.items():
        if mkey in skip_metrics or mkey not in sample.columns:
            continue
        vals_raw = pd.to_numeric(sample[mkey], errors="coerce")
        if vals_raw.isna().all() or (vals_raw == 0).all():
            continue

        fig, ax = plt.subplots(figsize=(10, 6))
        tcps, vals, colors = [], [], []
        for tcp in TCP_COLORS:
            row = sample[sample["tcp"] == tcp]
            if row.empty:
                continue
            v = pd.to_numeric(row[mkey], errors="coerce").values[0]
            tcps.append(TCP_LABELS[tcp])
            vals.append(v)
            colors.append(TCP_COLORS[tcp])

        if not tcps:
            continue

        x_pos = np.arange(len(tcps))
        bars = ax.bar(x_pos, vals, width=0.55, color=colors,
                       edgecolor="black", linewidth=0.7, alpha=0.88)

       
        for bar, v in zip(bars, vals):
            ax.text(
                bar.get_x() + bar.get_width() / 2.0,
                bar.get_height() + max(vals) * 0.015,
                f"{v:.2f}",
                ha="center", va="bottom", fontsize=10, fontweight="bold",
            )

        ax.set_xticks(x_pos)
        ax.set_xticklabels(tcps, rotation=20, ha="right")
        ax.set_ylabel(f"{mname} ({munit})")
        ax.set_title(f"{topology_name}:  {mname}  Comparison")
        ax.set_ylim(bottom=0, top=max(vals) * 1.18 if max(vals) > 0 else 1)

        _save(fig, f"{out_prefix}_bar_{mkey}.png")




def process_wifi_mobile():
    csv = os.path.join(BASE_DIR, "results", "wifi_mobile", "wifi_mobile_results.csv")
    if not os.path.exists(csv):
        print("  [WARN] WiFi CSV missing"); return
    print("\n── WiFi 802.11 (Mobile) ──")
    df = pd.read_csv(csv)
   
    before = len(df)
    df = df[~((pd.to_numeric(df.get("txPackets", 0), errors="coerce") == 0) &
              (pd.to_numeric(df.get("rxPackets", 0), errors="coerce") == 0))].copy()
    dropped = before - len(df)
    if dropped:
        print(f"    [filter] removed {dropped} all-zero timeout rows")
    for sw, xc, xl in [
        ("nodes", "nNodes",       "Number of Nodes"),
        ("flows", "nFlows",       "Number of Flows"),
        ("pps",   "packetsPerSec","Packets per Second"),
        ("speed", "speed",        "Node Speed (m/s)"),
    ]:
        plot_sweep(df, sw, xc, xl, "WiFi 802.11 (Mobile)", "wifi_mobile")
    plot_bar(df, "WiFi 802.11 (Mobile)", "wifi_mobile")


def process_wpan_static():
    csv = os.path.join(BASE_DIR, "results", "wpan_static", "wpan_static_results.csv")
    if not os.path.exists(csv):
        print("  [WARN] WPAN CSV missing"); return
    print("\n── 802.15.4 (Static) ──")
    df = pd.read_csv(csv)
    for sw, xc, xl in [
        ("nodes",    "nNodes",              "Number of Nodes"),
        ("flows",    "nFlows",              "Number of Flows"),
        ("pps",      "packetsPerSec",       "Packets per Second"),
        ("coverage", "coverageMultiplier",  "Coverage Multiplier (×Tx range)"),
    ]:
        plot_sweep(df, sw, xc, xl, "802.15.4 (Static)", "wpan_static")
    plot_bar(df, "802.15.4 (Static)", "wpan_static")


def process_cross_transmission():
    csv = os.path.join(BASE_DIR, "results", "cross_transmission", "cross_results.csv")
    if not os.path.exists(csv):
        print("  [WARN] Cross CSV missing"); return
    print("\n── Cross-Transmission (Bonus) ──")
    df = pd.read_csv(csv)
    df["totalNodes"] = pd.to_numeric(df["nWired"], errors="coerce") + \
                       pd.to_numeric(df["nWireless"], errors="coerce")

    # Only plot metrics that actually have meaningful data
    good_metrics = {k for k in METRICS if k in df.columns and not _is_degenerate(df[k])}
    print(f"    Usable metrics: {good_metrics or '{none}'}")

    for mkey in sorted(good_metrics):
        mname, munit = METRICS[mkey]
        fig, ax = plt.subplots()
        for tcp in TCP_COLORS:
            td = df[df["tcp"] == tcp].sort_values("totalNodes")
            if td.empty:
                continue
            x = td["totalNodes"].values
            y = pd.to_numeric(td[mkey], errors="coerce").values
            ax.plot(
                x, y, color=TCP_COLORS[tcp], marker=TCP_MARKERS[tcp],
                linestyle=TCP_LINES[tcp], label=TCP_LABELS[tcp],
                markeredgecolor="white", markeredgewidth=0.8,
            )
        ax.set_xlabel("Total Nodes (Wired + Wireless)")
        ax.set_ylabel(f"{mname} ({munit})")
        ax.set_title(f"Cross-Transmission:  {mname}  vs  Network Size")
        _smart_legend(ax)
        _save(fig, f"cross_transmission_{mkey}.png")

    # Bar only for good metrics
    skip = set(METRICS.keys()) - good_metrics
    plot_bar(df, "Cross-Transmission", "cross", skip_metrics=skip)


def process_lte_bonus():
    csv = os.path.join(BASE_DIR, "results", "lte_bonus", "lte_results.csv")
    if not os.path.exists(csv):
        print("  [WARN] LTE CSV missing"); return
    print("\n── LTE Network (Bonus) ──")
    df = pd.read_csv(csv)

    good = {k for k in METRICS if k in df.columns and not _is_degenerate(df[k])}
    # Also skip dropRatio if range is negligible (< 0.5 %)
    dr = pd.to_numeric(df.get("dropRatio", pd.Series(dtype=float)), errors="coerce")
    if dr.max() - dr.min() < 0.5:
        good.discard("dropRatio")

    for mkey in sorted(good):
        mname, munit = METRICS[mkey]
        fig, ax = plt.subplots()
        for tcp in TCP_COLORS:
            td = df[df["tcp"] == tcp].sort_values("nNodes")
            if td.empty:
                continue
            x = td["nNodes"].values
            y = pd.to_numeric(td[mkey], errors="coerce").values
            ax.plot(
                x, y, color=TCP_COLORS[tcp], marker=TCP_MARKERS[tcp],
                linestyle=TCP_LINES[tcp], label=TCP_LABELS[tcp],
                markeredgecolor="white", markeredgewidth=0.8,
            )
        ax.set_xlabel("Number of UEs")
        ax.set_ylabel(f"{mname} ({munit})")
        ax.set_title(f"LTE Network:  {mname}  vs  Number of UEs")
        _smart_legend(ax)
        _save(fig, f"lte_bonus_{mkey}.png")

    skip = set(METRICS.keys()) - good
    plot_bar(df, "LTE Network", "lte", skip_metrics=skip)


# ═══════════════════════  COMBINED  ═══════════════════════

def create_combined_summary():
    topo_files = {
        "WiFi 802.11\n(Mobile)":     os.path.join(BASE_DIR, "results", "wifi_mobile", "wifi_mobile_results.csv"),
        "802.15.4\n(Static)":        os.path.join(BASE_DIR, "results", "wpan_static", "wpan_static_results.csv"),
        "Cross-Trans.":              os.path.join(BASE_DIR, "results", "cross_transmission", "cross_results.csv"),
        "LTE":                       os.path.join(BASE_DIR, "results", "lte_bonus", "lte_results.csv"),
    }

    rows = []
    for tname, csv_path in topo_files.items():
        if not os.path.exists(csv_path):
            continue
        df = pd.read_csv(csv_path)
        for tcp in TCP_COLORS:
            sub = df[df["tcp"] == tcp]
            if sub.empty:
                continue
            rows.append({
                "Topology":              tname,
                "TCP Variant":           TCP_LABELS[tcp],
                "Avg Throughput (Mbps)": pd.to_numeric(sub["throughput_Mbps"], errors="coerce").mean(),
                "Avg Delay (ms)":        pd.to_numeric(sub["delay_ms"],        errors="coerce").mean(),
                "Avg PDR (%)":           pd.to_numeric(sub["pdr"],             errors="coerce").mean(),
                "Avg Drop (%)":          pd.to_numeric(sub["dropRatio"],       errors="coerce").mean(),
                "Avg Energy (J)":        pd.to_numeric(sub["energy_J"],        errors="coerce").mean(),
            })

    if not rows:
        return
    sdf = pd.DataFrame(rows)
    sdf.to_csv(os.path.join(OUTDIR, "summary_all_topologies.csv"),
               index=False, float_format="%.4f")
    print("\n    ✓ summary_all_topologies.csv")

    # ── Multi-panel comparison figure ──
    metric_cols = [
        "Avg Throughput (Mbps)", "Avg Delay (ms)", "Avg PDR (%)",
        "Avg Drop (%)", "Avg Energy (J)",
    ]
    fig, axes = plt.subplots(2, 3, figsize=(20, 11))
    axes = axes.flatten()

    for idx, mcol in enumerate(metric_cols):
        ax = axes[idx]
        pivot = sdf.pivot_table(values=mcol, index="TCP Variant",
                                columns="Topology", aggfunc="mean")
        bars = pivot.plot(kind="bar", ax=ax, edgecolor="black", linewidth=0.5,
                          alpha=0.85, width=0.75)
        unit = mcol.split("(")[1].rstrip(")") if "(" in mcol else ""
        ax.set_title(mcol, fontweight="bold", fontsize=12)
        ax.set_ylabel(unit, fontsize=10)
        ax.legend(fontsize=7.5, loc="best", framealpha=0.9)
        ax.tick_params(axis="x", rotation=30, labelsize=9)
        ax.grid(axis="y", alpha=0.4, linestyle="--")

    axes[-1].axis("off")
    fig.suptitle("TCP Variant Performance Across All Topologies",
                 fontweight="bold", fontsize=15, y=0.99)
    fig.tight_layout(rect=[0, 0, 1, 0.97])
    _save(fig, "combined_comparison.png")


# ═══════════════════════  MAIN  ═══════════════════════

def main():
    setup_style()
    print("=" * 60)
    print("  Assignment Results Plotter  v2")
    print("  Student ID: 2105155  |  Std_id % 8 = 3")
    print("=" * 60)

    # Clear old PNGs so deleted plots don't linger
    for f in os.listdir(OUTDIR):
        if f.endswith(".png"):
            os.remove(os.path.join(OUTDIR, f))

    process_wifi_mobile()
    process_wpan_static()
    process_cross_transmission()
    process_lte_bonus()
    create_combined_summary()

    n = len([f for f in os.listdir(OUTDIR) if f.endswith(".png")])
    print(f"\n{'=' * 60}")
    print(f"  Total plots: {n}   →  {OUTDIR}")
    print(f"{'=' * 60}")


if __name__ == "__main__":
    main()
