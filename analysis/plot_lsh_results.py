#!/usr/bin/env python3
"""
Script to parse LSH Approximate Nearest Neighbour results and plot
Avg_FPR vs Avg_Recall, with separate colors per (experiment, b) combination.
"""

import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
import itertools


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


def plot_lsh_results(filepath, output_path=None):
    experiments = parse_lsh_results(filepath)

    if not experiments:
        print("No experiments found in file")
        return

    fig, ax = plt.subplots(figsize=(10, 7))

    # Collect all unique b values across experiments
    all_b = sorted({row['b'] for exp in experiments for row in exp['rows']})

    # Markers cycle for experiments, colors for b values
    markers = ['o', '*', 'D', '^', 'v', 'P', 's', 'X']
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

            # Plot line connecting points with the same b
            ax.plot(
                recalls, fprs,
                color=b_colors[b_val],
                linestyle='--', linewidth=1, alpha=0.5,
            )

            # Plot scatter points
            ax.scatter(
                recalls, fprs,
                label=f'b={int(b_val)} ({label_prefix})',
                s=120, alpha=0.8,
                color=b_colors[b_val],
                marker=marker,
                edgecolors='black', linewidth=0.5,
                zorder=5,
            )

            # Annotate each point with its r value
            for r_row in rows:
                ax.annotate(
                    f'r={int(r_row["r"])}',
                    (r_row['Avg_Recall'], r_row['Avg_FPR']),
                    textcoords='offset points', xytext=(6, 6),
                    fontsize=7, alpha=0.85,
                )

    # Axis labels & title
    ax.set_xlabel('Average Recall', fontsize=13)
    ax.set_ylabel('Average FPR (log scale)', fontsize=13)

    hash_name = experiments[0]['metadata'].get('Hashname', 'Unknown')
    ax.set_title(
        f'LSH Approx Nearest Neighbour: {hash_name}\nAvg FPR vs Avg Recall',
        fontsize=14, fontweight='bold',
    )

    ax.set_yscale('log')
    ax.legend(loc='best', fontsize=9, title='b  (experiment params)', title_fontsize=10)
    ax.grid(True, alpha=0.3)
    plt.tight_layout()

    if output_path is None:
        output_path = Path(filepath).stem + '_fpr_vs_recall.png'

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

    plot_lsh_results(filepath, output_path)
