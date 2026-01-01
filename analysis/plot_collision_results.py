#!/usr/bin/env python3
"""
LSH Collision Analysis Plotting Script

This script reads LSH collision test results from a CSV file and generates three types of plots:
1. Line plots showing average collisions vs Dissimilarity rate
2. Box plots with categorical x-axis showing collision distribution
3. Box plots with numerical x-axis (to-scale) showing collision distribution

Usage:
    python plot_collision_results.py <input_csv_file>

Output:
    Three PNG files with the input filename as prefix:
    - <input_filename>_plot1.png: Line plots of average collisions
    - <input_filename>_plot2.png: Box plots with categorical Dissimilarity rates
    - <input_filename>_plot3.png: Box plots with numerical Dissimilarity rate axis
"""

import sys
import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np


def read_collision_data_complete(filename):
    """
    Read collision results CSV file with proper metadata parsing.
    
    The CSV file format contains metadata lines starting with ':' followed by data lines.
    Metadata lines define:
        :1: Test name
        :2: Column headers (ignored)
        :3: Hash name, keybits, token length
        :4: Dissimilarity rate bins (comma-separated)
        :5: Epsilon value (legend label) followed by bin numbers
        :6: Bin number: collision rate value
    
    Args:
        filename (str): Path to the CSV file
        
    Returns:
        tuple: (sections dict, DataFrame with parsed data)
            - sections: Dictionary containing metadata
            - DataFrame: Contains columns for test_name, hashname, keybits, 
                         tokenlength, epsilon, dissimilarity_rate, and collision_rate
    """
    with open(filename, 'r') as file:
        lines = file.readlines()

    sections = {}
    data_rows = []
    current_epsilon = None
    dissimilarity_bins = []

    for line in lines:
        line = line.strip()
        if not line:
            continue
            
        if line.startswith(':1:'):
            sections['test_name'] = line[3:].strip()
        elif line.startswith(':2:'):
            continue  # Skip column headers
        elif line.startswith(':3:'):
            line_content = line[3:].strip()
            line_parts = line_content.split(',')
            sections['Hashname'] = line_parts[0].strip()
            sections['Keybits'] = int(line_parts[1].strip())
            sections['Tokenlength'] = int(line_parts[2].strip())
        elif line.startswith(':4:'):
            line_content = line[3:].strip()
            dissimilarity_bins = [float(x.strip()) for x in line_content.split(',')]
            sections['dissimilarity_bins'] = dissimilarity_bins
        elif line.startswith(':5:'):
            # New epsilon value (legend label)
            line_content = line[3:].strip()
            parts = line_content.split(',')
            current_epsilon = float(parts[0].strip())
        elif line.startswith(':6:'):
            # Data point: bin_number: collision_rate
            line_content = line[3:].strip()
            parts = line_content.split(':')
            bin_idx = int(parts[0].strip())
            collision_rate = float(parts[1].strip())
            
            # Create data row
            row_data = {
                'test_name': sections.get('test_name', ''),
                'hashname': sections.get('Hashname', ''),
                'keybits': sections.get('Keybits', 0),
                'tokenlength': sections.get('Tokenlength', 0),
                'epsilon': current_epsilon,
                'dissimilarity_rate': dissimilarity_bins[bin_idx],
                'collision_rate': collision_rate
            }
            data_rows.append(row_data)
    
    return sections, pd.DataFrame(data_rows)


def generate_line_plots(data_lines, output_filename):
    """
    Generate line plots showing collision rate vs Dissimilarity rate for different epsilon values.
    
    Creates a single plot with separate lines for each epsilon value,
    showing the relationship between dissimilarity rate and collision rate.
    
    Args:
        data_lines (DataFrame): Parsed collision data
        output_filename (str): Path to save the output PNG file
    """
    print("Generating Plot 1: Line plots (Collision Rate vs Dissimilarity Rate)...")
    
    # Get unique epsilon values
    unique_epsilons = sorted(data_lines['epsilon'].unique())
    
    print(f"  Creating plot with {len(unique_epsilons)} epsilon values")
    
    fig, ax = plt.subplots(1, 1, figsize=(12, 8))
    
    # Define colors for different epsilon values
    colors = plt.cm.viridis(np.linspace(0, 1, len(unique_epsilons)))
    
    # Plot data for each epsilon
    for i, epsilon in enumerate(unique_epsilons):
        subset = data_lines[data_lines['epsilon'] == epsilon]
        
        if not subset.empty:
            # Sort by dissimilarity_rate for proper line plotting
            subset = subset.sort_values('dissimilarity_rate')
            
            # Plot dissimilarity_rate vs collision_rate
            ax.plot(subset['dissimilarity_rate'], subset['collision_rate'], 
                   marker='o', color=colors[i], linewidth=2, markersize=6,
                   label=f'ε = {epsilon:.2f}')
    
    # Customize plot
    ax.set_xlabel('Dissimilarity Rate', fontsize=12)
    ax.set_ylabel('Collision Rate', fontsize=12)
    ax.set_title(f'LSH Collision Analysis: {data_lines["hashname"].iloc[0]}\n'
                f'Key bits: {data_lines["keybits"].iloc[0]}, '
                f'Token length: {data_lines["tokenlength"].iloc[0]}', 
                fontsize=14, fontweight='bold')
    ax.grid(True, alpha=0.3)
    ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', fontsize=10)
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)
    
    plt.tight_layout()
    
    # Save figure
    plt.savefig(output_filename, dpi=300, bbox_inches='tight', 
                facecolor='white', edgecolor='black', pad_inches=0.1)
    plt.close()
    print(f"  Saved: {output_filename}")


def generate_boxplot_categorical(data_lines, output_filename):
    """
    Generate box plots - not applicable for this data format.
    This function is kept for compatibility but will skip generation.
    """
    print("Skipping Plot 2: Box plots not applicable for collision rate data...")


def generate_boxplot_numerical(data_lines, output_filename):
    """
    Generate box plots - not applicable for this data format.
    This function is kept for compatibility but will skip generation.
    """
    print("Skipping Plot 3: Box plots not applicable for collision rate data...")


def main():
    """
    Main function to orchestrate the plotting process.
    
    Reads command-line arguments, loads collision data, and generates
    three types of visualization plots saved as PNG files.
    """
    # Check command-line arguments
    if len(sys.argv) != 2:
        print("Usage: python plot_collision_results.py <input_csv_file>")
        print("\nExample:")
        print("  python plot_collision_results.py collisionResults_Hamming-32-encoded-seq.csv")
        sys.exit(1)
    
    input_file = sys.argv[1]
    
    # Check if input file exists
    if not os.path.exists(input_file):
        print(f"Error: Input file '{input_file}' not found!")
        sys.exit(1)
    
    # Extract base filename without extension for output naming
    base_name = os.path.splitext(os.path.basename(input_file))[0]
    output_dir = os.path.dirname(input_file) if os.path.dirname(input_file) else '.'
    
    print(f"\n{'='*70}")
    print(f"LSH Collision Analysis Plotting Script")
    print(f"{'='*70}")
    print(f"Input file: {input_file}")
    print(f"Output directory: {output_dir}")
    print(f"Base name: {base_name}")
    print(f"{'='*70}\n")
    
    # Read collision data
    print("Reading collision data...")
    sections, data_lines = read_collision_data_complete(input_file)
    print(f"  Loaded {len(data_lines)} data points")
    
    if len(data_lines) == 0:
        print("Error: No data loaded from file!")
        sys.exit(1)
    
    print(f"  Hash function: {data_lines['hashname'].iloc[0]}")
    print(f"  Keybits: {data_lines['keybits'].iloc[0]}")
    print(f"  Token length: {data_lines['tokenlength'].iloc[0]}")
    print(f"  Unique epsilon values: {sorted(data_lines['epsilon'].unique())}")
    print()
    
    # Generate output filenames
    output_file_1 = os.path.join(output_dir, f"{base_name}_plot1.png")
    
    # Generate plot
    generate_line_plots(data_lines, output_file_1)
    
    print(f"\n{'='*70}")
    print("Plot generated successfully!")
    print(f"{'='*70}")
    print(f"Output file:")
    print(f"  1. {output_file_1}")
    print(f"{'='*70}\n")


if __name__ == "__main__":
    main()
