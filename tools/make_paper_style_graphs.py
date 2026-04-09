#!/usr/bin/env python3

import os
from pathlib import Path

import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
S1 = ROOT / "results" / "scenario1" / "scenario1_aggregated.csv"
S2 = ROOT / "results" / "scenario2" / "scenario2_aggregated.csv"
OUT = ROOT / "results" / "paper_style"

ORDER = ["experts", "adaptive", "kalman", "ml", "plus", "newreno"]
LABEL = {
    "experts": "Experts (Paper)",
    "adaptive": "Adaptive Experts",
    "kalman": "Kalman",
    "ml": "EWMA",
    "plus": "Westwood+",
    "newreno": "NewReno",
}
COLOR = {
    "experts": "#d62728",
    "adaptive": "#8c564b",
    "kalman": "#9467bd",
    "ml": "#1f77b4",
    "plus": "#ff7f0e",
    "newreno": "#2ca02c",
}
MARK = {
    "experts": "D",
    "adaptive": "X",
    "kalman": "P",
    "ml": "o",
    "plus": "s",
    "newreno": "^",
}


def style_axes(ax, xlabel, ylabel, title):
    ax.set_xlabel(xlabel, fontsize=10)
    ax.set_ylabel(ylabel, fontsize=10)
    ax.set_title(title, fontsize=11, fontweight="bold")
    ax.grid(True, linestyle="--", linewidth=0.5, alpha=0.45)
    ax.tick_params(labelsize=9)


def plot_s1_metric(df, metric, metric_std, ylabel, out_name):
    fig, ax = plt.subplots(figsize=(7.2, 4.5))
    for tcp in ORDER:
        sub = df[df["tcp"] == tcp].sort_values("nFlows")
        if sub.empty:
            continue
        ax.errorbar(
            sub["nFlows"],
            sub[metric],
            yerr=sub[metric_std],
            marker=MARK[tcp],
            markersize=5,
            linewidth=1.8,
            capsize=3,
            color=COLOR[tcp],
            label=LABEL[tcp],
        )
    style_axes(ax, "Concurrent TCP Flows", ylabel, f"Scenario I: {ylabel}")
    ax.legend(fontsize=8, ncol=2, frameon=True)
    fig.tight_layout()
    fig.savefig(OUT / out_name, dpi=220)
    plt.close(fig)


def plot_s2_metric(df, metric, metric_std, ylabel, out_name):
    speed_order = ["[1,10]", "[20,30]", "[40,50]"]
    x = [1, 2, 3]

    fig, ax = plt.subplots(figsize=(7.2, 4.5))
    for tcp in ORDER:
        sub = df[df["tcp"] == tcp].copy()
        if sub.empty:
            continue
        sub["speed"] = pd.Categorical(sub["speed"], speed_order, ordered=True)
        sub = sub.sort_values("speed")
        ax.errorbar(
            x,
            sub[metric],
            yerr=sub[metric_std],
            marker=MARK[tcp],
            markersize=5,
            linewidth=1.8,
            capsize=3,
            color=COLOR[tcp],
            label=LABEL[tcp],
        )

    ax.set_xticks(x)
    ax.set_xticklabels(speed_order)
    style_axes(ax, "Node Speed Range (m/s)", ylabel, f"Scenario II: {ylabel}")
    ax.legend(fontsize=8, ncol=2, frameon=True)
    fig.tight_layout()
    fig.savefig(OUT / out_name, dpi=220)
    plt.close(fig)


def panel_s1(df):
    fig, axs = plt.subplots(2, 2, figsize=(10, 7.2))
    specs = [
        ("throughput_mean", "throughput_std", "Throughput (Mbps)"),
        ("loss_mean", "loss_std", "Packet Loss (%)"),
        ("delay_mean", "delay_std", "Mean Delay (ms)"),
        ("jitter_mean", "jitter_std", "Mean Jitter (ms)"),
    ]

    for ax, (m, s, ylab) in zip(axs.flat, specs):
        for tcp in ORDER:
            sub = df[df["tcp"] == tcp].sort_values("nFlows")
            if sub.empty:
                continue
            ax.errorbar(
                sub["nFlows"],
                sub[m],
                yerr=sub[s],
                marker=MARK[tcp],
                markersize=4,
                linewidth=1.3,
                capsize=2,
                color=COLOR[tcp],
                label=LABEL[tcp],
            )
        style_axes(ax, "Flows", ylab, ylab)

    handles, labels = axs[0, 0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="lower center", ncol=3, fontsize=8, frameon=True)
    fig.suptitle("Scenario I (MANET) – Paper-Style Multi-Metric Comparison", fontsize=13, fontweight="bold")
    fig.tight_layout(rect=[0, 0.08, 1, 0.96])
    fig.savefig(OUT / "s1_all_metrics_paper.png", dpi=240)
    plt.close(fig)


def panel_s2(df):
    speed_order = ["[1,10]", "[20,30]", "[40,50]"]
    x = [1, 2, 3]
    fig, axs = plt.subplots(2, 2, figsize=(10, 7.2))
    specs = [
        ("throughput_mean", "throughput_std", "Throughput (Mbps)"),
        ("loss_mean", "loss_std", "Packet Loss (%)"),
        ("delay_mean", "delay_std", "Mean Delay (ms)"),
        ("jitter_mean", "jitter_std", "Mean Jitter (ms)"),
    ]

    for ax, (m, s, ylab) in zip(axs.flat, specs):
        for tcp in ORDER:
            sub = df[df["tcp"] == tcp].copy()
            if sub.empty:
                continue
            sub["speed"] = pd.Categorical(sub["speed"], speed_order, ordered=True)
            sub = sub.sort_values("speed")
            ax.errorbar(
                x,
                sub[m],
                yerr=sub[s],
                marker=MARK[tcp],
                markersize=4,
                linewidth=1.3,
                capsize=2,
                color=COLOR[tcp],
                label=LABEL[tcp],
            )
        ax.set_xticks(x)
        ax.set_xticklabels(speed_order)
        style_axes(ax, "Speed Range", ylab, ylab)

    handles, labels = axs[0, 0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="lower center", ncol=3, fontsize=8, frameon=True)
    fig.suptitle("Scenario II (Bursty) – Paper-Style Multi-Metric Comparison", fontsize=13, fontweight="bold")
    fig.tight_layout(rect=[0, 0.08, 1, 0.96])
    fig.savefig(OUT / "s2_all_metrics_paper.png", dpi=240)
    plt.close(fig)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    if not S1.exists() or not S2.exists():
        raise FileNotFoundError("Aggregated CSVs not found. Run tools/analyze_scenarios.py first.")

    s1 = pd.read_csv(S1)
    s2 = pd.read_csv(S2)

    plot_s1_metric(s1, "throughput_mean", "throughput_std", "Throughput (Mbps)", "s1_throughput_paper.png")
    plot_s1_metric(s1, "loss_mean", "loss_std", "Packet Loss (%)", "s1_loss_paper.png")
    plot_s1_metric(s1, "delay_mean", "delay_std", "Mean Delay (ms)", "s1_delay_paper.png")
    plot_s1_metric(s1, "jitter_mean", "jitter_std", "Mean Jitter (ms)", "s1_jitter_paper.png")
    plot_s1_metric(s1, "goodput_mean", "throughput_std", "Goodput (packets)", "s1_goodput_paper.png")
    plot_s1_metric(s1, "retx_pct", "loss_std", "Retransmissions (%)", "s1_retx_paper.png")

    plot_s2_metric(s2, "throughput_mean", "throughput_std", "Throughput (Mbps)", "s2_throughput_paper.png")
    plot_s2_metric(s2, "loss_mean", "loss_std", "Packet Loss (%)", "s2_loss_paper.png")
    plot_s2_metric(s2, "delay_mean", "delay_std", "Mean Delay (ms)", "s2_delay_paper.png")
    plot_s2_metric(s2, "jitter_mean", "jitter_std", "Mean Jitter (ms)", "s2_jitter_paper.png")
    plot_s2_metric(s2, "goodput_mean", "goodput_std", "Goodput (packets)", "s2_goodput_paper.png")

    panel_s1(s1)
    panel_s2(s2)

    print(f"Saved paper-style graphs in: {OUT}")


if __name__ == "__main__":
    main()
