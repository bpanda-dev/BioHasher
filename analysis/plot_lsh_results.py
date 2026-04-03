#!/usr/bin/env python3
"""
Script to parse LSH Approximate Nearest Neighbour results and plot
Avg_FPR vs Avg_Recall, with separate colors per (experiment, b) combination.
"""

import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
import itertools
from adjustText import adjust_text


def parse_lsh_results(filepath):
    """
    Parse custom CSV format with multiple experiment blocks.
    Each block starts with :1: and has its own metadata and data rows.

    Returns a list of experiments, each being a dict with 'metadata', 'params', and 'rows'.
    """
    experiments = []
    current_exp = None
    headers = []

    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            if ':' not in line:
                continue

            parts = line.split(':', 2)
            if len(parts) < 3:
                continue

            prefix = parts[1]
            content = parts[2]

            if prefix == '1':
                # New experiment block
                current_exp = {'metadata': {}, 'params': {}, 'rows': []}
                experiments.append(current_exp)

            elif prefix == '2' and current_exp is not None:
                current_exp['_meta_keys'] = [item.strip() for item in content.split(',')]

            elif prefix == '3' and current_exp is not None:
                values = [v.strip() for v in content.split(',')]
                for key, val in zip(current_exp.get('_meta_keys', []), values):
                    current_exp['metadata'][key] = val

            elif prefix == '4.1' and current_exp is not None:
                current_exp['_param_keys'] = [item.strip() for item in content.split(',')]

            elif prefix == '4.2' and current_exp is not None:
                current_exp['_param_descs'] = [item.strip() for item in content.split(',')]

            elif prefix == '4.3' and current_exp is not None:
                values = [v.strip() for v in content.split(',')]
                for key, val in zip(current_exp.get('_param_keys', []), values):
                    current_exp['params'][key] = val

            elif prefix == '5' and current_exp is not None:
                headers = [h.strip() for h in content.split(',')]

            elif prefix == '6' and current_exp is not None:
                values = [v.strip() for v in content.split(',')]
                if len(values) == len(headers):
                    row = {}
                    for col, val in zip(headers, values):
                        try:
                            row[col] = float(val)
                        except ValueError:
                            row[col] = val
                    current_exp['rows'].append(row)

    # Clean up internal keys
    for exp in experiments:
        exp.pop('_meta_keys', None)
        exp.pop('_param_keys', None)
        exp.pop('_param_descs', None)

    return experiments


def experiment_label(exp):
    """Build a short label for an experiment from its params."""
    parts = [f"{k}={v}" for k, v in exp['params'].items()]
    return ', '.join(parts) if parts else 'default'


def plot_lsh_results(filepath, output_path=None, use_log=False):
    experiments = parse_lsh_results(filepath)

    if not experiments:
        print("No experiments found in file")
        return

    # Filter out experiments with no data rows
    experiments = [exp for exp in experiments if exp['rows']]
    if not experiments:
        print("No experiments with data found")
        return

    fig, ax = plt.subplots(figsize=(11, 7))
    texts = []  # collect for adjust_text

    # Collect all unique b values across experiments
    all_b = sorted({row['b'] for exp in experiments for row in exp['rows']})

    # Markers cycle for experiments, colors for b values
    markers = ['o','^','*', 's', 'D', 'v', 'P', 'X']
    cmap = plt.cm.tab10
    b_colors = {b: cmap(i / max(len(all_b) - 1, 1)) for i, b in enumerate(all_b)}

    for exp_idx, exp in enumerate(experiments):
        marker = markers[exp_idx % len(markers)]
        label_prefix = experiment_label(exp)

        for b_val in all_b:
            rows = [r for r in exp['rows'] if r['b'] == b_val]
            if not rows:
                continue

            # Sort by recall so the connecting line is meaningful
            rows.sort(key=lambda r: r['Avg_Recall'])

            recalls = [r['Avg_Recall'] for r in rows]
            fprs = [r['Avg_FPR'] for r in rows]

            # Plot line + markers together (ensures proper legend entry)
            ax.plot(
                recalls, fprs,
                color=b_colors[b_val],
                marker=marker,
                markersize=6,
                markeredgecolor='black',
                markeredgewidth=0.5,
                linestyle='--', linewidth=1.2, alpha=0.75,
                label=f'b={int(b_val)} ({label_prefix})',
                zorder=5,
            )

            # Annotate each point with its r value
            for r_row in rows:
                texts.append(ax.text(
                    r_row['Avg_Recall'], r_row['Avg_FPR'],
                    f'{int(r_row["r"])}',
                    fontsize=9, fontweight='bold', alpha=0.85,
                ))

    # Axis labels & title
    scale_label = ' (log scale)' if use_log else ''
    ax.set_xlabel('Average Recall', fontsize=13)
    ax.set_ylabel(f'Average FPR{scale_label}', fontsize=13)

    hash_name = experiments[0]['metadata'].get('Hashname', 'Unknown')
    # ax.set_title(
    #     f'LSH Approx Nearest Neighbour: {hash_name}\nAvg FPR vs Avg Recall',
    #     fontsize=14, fontweight='bold',
    # )

    if use_log:
        ax.set_yscale('log')
    ax.legend(loc='best', fontsize=9, title='b  (experiment params)', title_fontsize=10)
    ax.grid(True, alpha=0.3)
    adjust_text(texts, ax=ax, arrowprops=dict(arrowstyle='-', color='gray', lw=0.5))
    plt.tight_layout()

    if output_path is None:
        suffix = '_log' if use_log else ''
        output_path = Path(filepath).stem + f'_fpr_vs_recall{suffix}.png'

    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"Plot saved to: {output_path}")
    plt.show()


def plot_best_fpr_per_recall_bin(filepath, output_path=None, num_bins=25, use_log=False):
    """
    For each experiment, bin (b,r) data points by Avg_Recall.
    For each recall bin, pick the minimum FPR across all (b,r) pairs in that bin.
    Plot one curve per experiment.
    """
    experiments = parse_lsh_results(filepath)
    experiments = [exp for exp in experiments if exp['rows']]

    if not experiments:
        print("No experiments with data found")
        return

    fig, ax = plt.subplots(figsize=(11, 7))
    texts = []  # collect for adjust_text

    # Consistent bin edges across all experiments
    all_recalls = [r['Avg_Recall'] for exp in experiments for r in exp['rows']]
    bin_edges = np.linspace(min(all_recalls) - 1e-9, max(all_recalls) + 1e-9, num_bins + 1)

    markers = ['o', 's', 'D', '^', 'v', 'P', '*', 'X']
    cmap = plt.cm.tab10

    for exp_idx, exp in enumerate(experiments):
        label = experiment_label(exp)
        color = cmap(exp_idx / max(len(experiments) - 1, 1))
        marker = markers[exp_idx % len(markers)]

        # Bin rows by recall and find min FPR per bin
        bin_centers = []
        min_fprs = []
        bin_annotations = []  # (b,r) that achieved the min FPR

        for i in range(num_bins):
            lo, hi = bin_edges[i], bin_edges[i + 1]
            rows_in_bin = [r for r in exp['rows'] if lo <= r['Avg_Recall'] < hi]

            if not rows_in_bin:
                continue

            best = min(rows_in_bin, key=lambda r: r['Avg_FPR'])
            bin_centers.append((lo + hi) / 2)
            min_fprs.append(best['Avg_FPR'])
            bin_annotations.append(f"({int(best['b'])},{int(best['r'])})")

        # Sort by bin center for clean line
        order = sorted(range(len(bin_centers)), key=lambda i: bin_centers[i])
        bin_centers = [bin_centers[i] for i in order]
        min_fprs = [min_fprs[i] for i in order]
        bin_annotations = [bin_annotations[i] for i in order]

        # ax.scatter(
        #     bin_centers, min_fprs,
        #     color=color, marker=marker, s=100,
        #     edgecolors='black', linewidth=0.5,
        #     alpha=0.85, label=label, zorder=5,
        # )

        ax.plot(
            bin_centers, min_fprs,
            color=color, marker=marker, markersize=9,
            markeredgecolor='black', markeredgewidth=0.5,
            linestyle='-', linewidth=1.5, alpha=0.85,
            label=label, zorder=5,
        )

        for x, y, ann in zip(bin_centers, min_fprs, bin_annotations):
            texts.append(ax.text(
                x, y, ann,
                fontsize=8, alpha=0.8,
            ))

    hash_name = experiments[0]['metadata'].get('Hashname', 'Unknown')
    scale_label = ' (log scale)' if use_log else ''
    ax.set_xlabel('Recall Bin Center', fontsize=13)
    ax.set_ylabel(f'Best (Min) FPR in Bin{scale_label}', fontsize=13)
    # ax.set_title(
    #     f'LSH {hash_name}: Best FPR per Recall Bin\n(lower is better)',
    #     fontsize=14, fontweight='bold',
    # )

    if use_log:
        ax.set_yscale('log')
    ax.legend(loc='best', fontsize=10, title='Experiment')
    ax.grid(True, alpha=0.3)
    adjust_text(texts, ax=ax, arrowprops=dict(arrowstyle='-', color='gray', lw=0.5))
    plt.tight_layout()

    if output_path is None:
        suffix = '_log' if use_log else ''
        output_path = Path(filepath).stem + f'_best_fpr_per_recall_bin{suffix}.png'

    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"Plot saved to: {output_path}")
    plt.show()

def plot_all_points(filepath, output_path=None, use_log=False):
    """
    Simple scatter: all (Recall, FPR) points per experiment in one color.
    No b/r separation, no annotations — just one cloud per experiment.
    """
    experiments = parse_lsh_results(filepath)
    experiments = [exp for exp in experiments if exp['rows']]

    if not experiments:
        print("No experiments with data found")
        return

    fig, ax = plt.subplots(figsize=(11, 7))
    texts = []  # collect for adjust_text

    markers = ['o', 's', 'D', '^', 'v', 'P', '*', 'X']
    cmap = plt.cm.tab10

    for exp_idx, exp in enumerate(experiments):
        color = cmap(exp_idx / max(len(experiments) - 1, 1))
        marker = markers[exp_idx % len(markers)]
        label = experiment_label(exp)

        recalls = [r['Avg_Recall'] for r in exp['rows']]
        fprs = [r['Avg_FPR'] for r in exp['rows']]

        ax.scatter(
            recalls, fprs,
            color=color, marker=marker, s=60, alpha=0.7,
            edgecolors='black', linewidth=0.4,
            label=label, zorder=5,
        )

        for row in exp['rows']:
            texts.append(ax.text(
                row['Avg_Recall'], row['Avg_FPR'],
                f"({int(row['b'])},{int(row['r'])})",
                fontsize=8, alpha=0.75,
            ))

    hash_name = experiments[0]['metadata'].get('Hashname', 'Unknown')
    scale_label = ' (log scale)' if use_log else ''
    ax.set_xlabel('Average Recall', fontsize=13)
    ax.set_ylabel(f'Average FPR{scale_label}', fontsize=13)
    # ax.set_title(
    #     f'LSH {hash_name}: All (b,r) configurations\nFPR vs Recall',
    #     fontsize=14, fontweight='bold',
    # )

    if use_log:
        ax.set_yscale('log')
    ax.legend(loc='best', fontsize=10, title='Experiment')
    ax.grid(True, alpha=0.3)
    adjust_text(texts, ax=ax, arrowprops=dict(arrowstyle='-', color='gray', lw=0.5))
    plt.tight_layout()

    if output_path is None:
        suffix = '_log' if use_log else ''
        output_path = Path(filepath).stem + f'_all_points{suffix}.png'

    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"Plot saved to: {output_path}")
    plt.show()


if __name__ == "__main__":
    import sys

    if len(sys.argv) > 1:
        filepath = sys.argv[1]
        output_path = sys.argv[2] if len(sys.argv) > 2 else None
    else:
        filepath = str(
            Path(__file__).parent.parent / "results" / "ApproxNearestNeighbourResults_SubseqHash-64.csv"
        )
        output_path = None

    # Linear versions
    plot_lsh_results(filepath, output_path)
    plot_best_fpr_per_recall_bin(filepath)
    plot_all_points(filepath)

    # Log-scale versions
    plot_lsh_results(filepath, use_log=True)
    plot_best_fpr_per_recall_bin(filepath, use_log=True)
    plot_all_points(filepath, use_log=True)

