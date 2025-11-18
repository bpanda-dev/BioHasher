#!/usr/bin/env python3
"""
LSH Collision Analysis Plotting Script

This script reads LSH collision test results from a CSV file and generates three types of plots:
1. Line plots showing average collisions vs similarity rate
2. Box plots with categorical x-axis showing collision distribution
3. Box plots with numerical x-axis (to-scale) showing collision distribution

Usage:
    python plot_collision_results.py <input_csv_file>

Output:
    Three PNG files with the input filename as prefix:
    - <input_filename>_plot1.png: Line plots of average collisions
    - <input_filename>_plot2.png: Box plots with categorical similarity rates
    - <input_filename>_plot3.png: Box plots with numerical similarity rate axis
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
        :2: (reserved)
        :3: Hash name, keybits, token length
        :4: Similarity rates (comma-separated)
        :5: (reserved)
        :6: (reserved)
    
    Data lines contain comma-separated collision counts for each hash computation.
    
    Args:
        filename (str): Path to the CSV file
        
    Returns:
        tuple: (sections dict, DataFrame with parsed data)
            - sections: Dictionary containing metadata
            - DataFrame: Contains columns for test_name, hashname, keybits, 
                         tokenlength, similarity_rate, and collision_counts
    """
    with open(filename, 'r') as file:
        lines = file.readlines()

        sections = {}
        data_lines = []
        subdata_i = 0

        for i, line in enumerate(lines):
            if line.startswith(':'):
                subdata_i = 0
                # This is a metadata line
                if line.startswith(':1:'):
                    sections['test_name'] = line[3:].strip()
                elif line.startswith(':2:'):
                    continue
                elif line.startswith(':3:'):
                    line_content = line[3:].strip()
                    line_parts = line_content.split(',')
                    sections['Hashname'] = line_parts[0].strip()
                    sections['Keybits'] = int(line_parts[1].strip())
                    sections['Tokenlength'] = int(line_parts[2].strip())
                elif line.startswith(':4:'):
                    line_content = line[3:].strip()
                    similarity_rates = [float(x.strip()) for x in line_content.split(',')]
                    sections['similarity_rates'] = similarity_rates
                elif line.startswith(':5:'):
                    continue
                elif line.startswith(':6:'):
                    continue
            else:
                if line.strip():
                    collision_counts = [float(x) for x in line.split(',')]
                    # Create a row with metadata + data
                    row_data = {
                        'test_name': sections.get('test_name', ''),
                        'hashname': sections.get('Hashname', ''),
                        'keybits': sections.get('Keybits', 0),
                        'tokenlength': sections.get('Tokenlength', 0),
                        'similarity_rate': sections.get('similarity_rates', [])[subdata_i],
                        'collision_counts': collision_counts
                    }
                    subdata_i += 1
                    data_lines.append(row_data)
    
    return sections, pd.DataFrame(data_lines)


def generate_line_plots(data_lines, output_filename):
    """
    Generate line plots showing average collisions vs similarity rate.
    
    Creates subplots for each unique keybit value, with separate lines for
    each token length. Shows the relationship between similarity rate and
    average collision counts.
    
    Args:
        data_lines (DataFrame): Parsed collision data
        output_filename (str): Path to save the output PNG file
    """
    print("Generating Plot 1: Line plots (Average Collisions vs Similarity Rate)...")
    
    # Calculate average and standard deviation for collision counts
    data_lines['average_collisions'] = data_lines['collision_counts'].apply(np.mean)
    data_lines['std_dev_collisions'] = data_lines['collision_counts'].apply(np.std)
    
    # Get unique values for subplot organization
    unique_keybits = sorted(data_lines['keybits'].unique())
    unique_tokenlengths = sorted(data_lines['tokenlength'].unique())
    
    n_keybits = len(unique_keybits)
    n_tokenlengths = len(unique_tokenlengths)
    
    print(f"  Creating {n_keybits} subplots, each with {n_tokenlengths} lines")
    
    # Calculate rows and columns for 2 subplots per row
    n_cols = 2
    n_rows = (n_keybits + n_cols - 1) // n_cols  # Ceiling division
    
    fig, axes = plt.subplots(n_rows, n_cols, figsize=(12, 6*n_rows))
    
    # Handle different subplot configurations
    if n_keybits == 1:
        axes = [axes] if n_rows == 1 else axes.flatten()
    elif n_rows == 1:
        axes = axes if n_keybits > 1 else [axes]
    else:
        axes = axes.flatten()
    
    # Define colors for different token lengths
    colors = plt.cm.Set1(np.linspace(0, 1, n_tokenlengths))
    
    # Plot data
    for i, keybit in enumerate(unique_keybits):
        ax = axes[i]
        
        for j, tokenlength in enumerate(unique_tokenlengths):
            # Filter data for this specific keybit and tokenlength combination
            subset = data_lines[
                (data_lines['keybits'] == keybit) & 
                (data_lines['tokenlength'] == tokenlength)
            ]
            
            if not subset.empty:
                # Plot similarity_rate vs average_collisions
                ax.plot(subset['similarity_rate'], subset['average_collisions'], 
                       marker='o', color=colors[j], linewidth=2, markersize=6,
                       label=f'Token Length {tokenlength}')
        
        # Customize subplot
        ax.set_xlabel('Similarity Rate', fontsize=12)
        ax.set_ylabel('Average Collisions', fontsize=12)
        ax.set_title(f'Keybits = {keybit}', fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        ax.legend()
        ax.set_xlim(0, 1)
    
    # Hide unused subplots if n_keybits is odd
    if n_keybits < len(axes):
        for i in range(n_keybits, len(axes)):
            axes[i].set_visible(False)
    
    # Overall plot formatting
    fig.suptitle(f'LSH Collision Analysis: {data_lines["hashname"].iloc[0]}', 
                 fontsize=18, fontweight='bold')
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    
    # Save figure with border
    plt.savefig(output_filename, dpi=300, bbox_inches='tight', 
                facecolor='white', edgecolor='black', linewidth=2)
    plt.close()
    print(f"  Saved: {output_filename}")


def generate_boxplot_categorical(data_lines, output_filename):
    """
    Generate box plots with categorical x-axis showing collision count distribution.
    
    Creates box plots grouped by similarity rate (as categorical labels) with
    separate boxes for each token length. Shows the distribution of collision
    counts at each similarity rate.
    
    Args:
        data_lines (DataFrame): Parsed collision data
        output_filename (str): Path to save the output PNG file
    """
    print("Generating Plot 2: Box plots with categorical x-axis...")
    
    # Get unique values for subplot organization
    unique_keybits = sorted(data_lines['keybits'].unique())
    unique_tokenlengths = sorted(data_lines['tokenlength'].unique())
    
    n_keybits = len(unique_keybits)
    n_tokenlengths = len(unique_tokenlengths)
    
    # Calculate rows and columns for 2 subplots per row
    n_cols = 2
    n_rows = (n_keybits + n_cols - 1) // n_cols
    
    fig, axes = plt.subplots(n_rows, n_cols, figsize=(40, 6*n_rows))
    
    # Handle subplot configurations
    if n_keybits == 1:
        axes = [axes] if n_rows == 1 else axes.flatten()
    elif n_rows == 1:
        axes = axes if n_keybits > 1 else [axes]
    else:
        axes = axes.flatten()
    
    # Define colors
    colors = plt.cm.Set1(np.linspace(0, 1, n_tokenlengths))
    
    # Plot data
    for i, keybit in enumerate(unique_keybits):
        ax = axes[i]
        
        # Get all unique similarity rates for this keybit
        all_sim_rates = set()
        for tokenlength in unique_tokenlengths:
            subset = data_lines[
                (data_lines['keybits'] == keybit) & 
                (data_lines['tokenlength'] == tokenlength)
            ]
            if not subset.empty:
                all_sim_rates.update(subset['similarity_rate'].unique())
        
        unique_sim_rates = sorted(all_sim_rates)
        
        # For each similarity rate, create grouped box plots
        for sim_idx, sim_rate in enumerate(unique_sim_rates):
            box_data = []
            colors_for_this_group = []
            
            for j, tokenlength in enumerate(unique_tokenlengths):
                subset = data_lines[
                    (data_lines['keybits'] == keybit) & 
                    (data_lines['tokenlength'] == tokenlength) &
                    (data_lines['similarity_rate'] == sim_rate)
                ]
                
                if not subset.empty:
                    # Get collision counts for this combination
                    collision_counts = subset['collision_counts'].iloc[0]
                    box_data.append(collision_counts)
                    colors_for_this_group.append(colors[j])
                else:
                    # Add empty data to maintain alignment
                    box_data.append([])
                    colors_for_this_group.append(colors[j])
            
            # Create positions for this group of box plots
            if box_data:
                # Calculate positions: center around sim_idx, with small offsets for each token length
                group_width = 0.8
                box_width = group_width / n_tokenlengths
                positions = []
                
                for j in range(n_tokenlengths):
                    offset = (j - (n_tokenlengths - 1) / 2) * box_width
                    positions.append(sim_idx + offset)
                
                # Filter out empty data
                valid_data = [(data, pos, color) for data, pos, color in 
                             zip(box_data, positions, colors_for_this_group) if data]
                
                if valid_data:
                    valid_box_data, valid_positions, valid_colors = zip(*valid_data)
                    
                    # Create box plot
                    bp = ax.boxplot(valid_box_data, positions=valid_positions, 
                                   widths=box_width*0.7, patch_artist=True,
                                   boxprops=dict(linewidth=1),
                                   medianprops=dict(color='black', linewidth=1.5),
                                   whiskerprops=dict(linewidth=1),
                                   capprops=dict(linewidth=1))
                    
                    # Color the boxes
                    for patch, color in zip(bp['boxes'], valid_colors):
                        patch.set_facecolor(color)
                        patch.set_alpha(0.7)
        
        # Customize subplot
        ax.set_xlabel('Similarity Rate', fontsize=12)
        ax.set_ylabel('Collision Counts Distribution', fontsize=12)
        ax.set_title(f'Keybits = {keybit}', fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        
        # Set x-ticks and labels (categorical)
        ax.set_xticks(range(len(unique_sim_rates)))
        ax.set_xticklabels([f'{rate:.2f}' for rate in unique_sim_rates], rotation=45)
        ax.set_xlim(-0.5, len(unique_sim_rates) - 0.5)
    
    # Create legend
    legend_elements = [plt.Rectangle((0,0),1,1, facecolor=colors[j], alpha=0.7, 
                                    label=f'Token Length {tokenlength}') 
                      for j, tokenlength in enumerate(unique_tokenlengths)]
    fig.legend(handles=legend_elements, loc='upper right', bbox_to_anchor=(0.98, 0.98))
    
    # Hide unused subplots
    if n_keybits < len(axes):
        for i in range(n_keybits, len(axes)):
            axes[i].set_visible(False)
    
    # Overall formatting
    fig.suptitle(f'LSH Collision Count Distributions: {data_lines["hashname"].iloc[0]}', 
                 fontsize=16, fontweight='bold')
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    
    # Save figure with border
    plt.savefig(output_filename, dpi=300, bbox_inches='tight', 
                facecolor='white', edgecolor='black', linewidth=2)
    plt.close()
    print(f"  Saved: {output_filename}")


def generate_boxplot_numerical(data_lines, output_filename):
    """
    Generate box plots with numerical x-axis (to-scale) showing collision distribution.
    
    Creates box plots where x-axis positions correspond to actual similarity rate
    values (numerical scale). This preserves the proportional spacing between
    different similarity rates. Useful for visualizing density in high-similarity regions.
    
    Args:
        data_lines (DataFrame): Parsed collision data
        output_filename (str): Path to save the output PNG file
    """
    print("Generating Plot 3: Box plots with numerical x-axis (to-scale)...")
    
    # Get unique values for subplot organization
    unique_keybits = sorted(data_lines['keybits'].unique())
    unique_tokenlengths = sorted(data_lines['tokenlength'].unique())
    
    n_keybits = len(unique_keybits)
    n_tokenlengths = len(unique_tokenlengths)
    
    # Calculate rows and columns for 2 subplots per row
    n_cols = 2
    n_rows = (n_keybits + n_cols - 1) // n_cols
    
    fig, axes = plt.subplots(n_rows, n_cols, figsize=(40, 6*n_rows))
    
    # Handle subplot configurations
    if n_keybits == 1:
        axes = [axes] if n_rows == 1 else axes.flatten()
    elif n_rows == 1:
        axes = axes if n_keybits > 1 else [axes]
    else:
        axes = axes.flatten()
    
    # Define colors
    colors = plt.cm.Set1(np.linspace(0, 1, n_tokenlengths))
    
    # Plot data
    for i, keybit in enumerate(unique_keybits):
        ax = axes[i]
        
        # Get all unique similarity rates for this keybit
        all_sim_rates = set()
        for tokenlength in unique_tokenlengths:
            subset = data_lines[
                (data_lines['keybits'] == keybit) & 
                (data_lines['tokenlength'] == tokenlength)
            ]
            if not subset.empty:
                all_sim_rates.update(subset['similarity_rate'].unique())
        
        unique_sim_rates = sorted(all_sim_rates)
        
        # For each similarity rate, create grouped box plots
        for sim_rate in unique_sim_rates:
            box_data = []
            colors_for_this_group = []
            positions = []
            
            for j, tokenlength in enumerate(unique_tokenlengths):
                subset = data_lines[
                    (data_lines['keybits'] == keybit) & 
                    (data_lines['tokenlength'] == tokenlength) &
                    (data_lines['similarity_rate'] == sim_rate)
                ]
                
                if not subset.empty:
                    # Get collision counts for this combination
                    collision_counts = subset['collision_counts'].iloc[0]
                    box_data.append(collision_counts)
                    colors_for_this_group.append(colors[j])
                    
                    # Use actual similarity rate as x-position with small offset
                    offset_width = 0.01  # Small offset between token lengths
                    offset = (j - (n_tokenlengths - 1) / 2) * offset_width
                    positions.append(sim_rate + offset)
            
            # Create box plot with actual numerical positions
            if box_data and positions:
                bp = ax.boxplot(box_data, positions=positions, 
                               widths=0.008, patch_artist=True,  # Smaller width for better spacing
                               boxprops=dict(linewidth=1),
                               medianprops=dict(color='black', linewidth=1.5),
                               whiskerprops=dict(linewidth=1),
                               capprops=dict(linewidth=1))
                
                # Color the boxes
                for patch, color in zip(bp['boxes'], colors_for_this_group):
                    patch.set_facecolor(color)
                    patch.set_alpha(0.7)
        
        # Customize subplot
        ax.set_xlabel('Similarity Rate', fontsize=12)
        ax.set_ylabel('Collision Counts Distribution', fontsize=12)
        ax.set_title(f'Keybits = {keybit}', fontsize=14, fontweight='bold')
        ax.grid(True, alpha=0.3)
        
        # Set x-axis to use numerical values with proper spacing
        ax.set_xlim(-0.02, 1.02)
        
        # Set x-ticks to show the actual similarity rate values
        # Show more ticks in the dense region (0.9-1.0)
        major_ticks = [0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]
        minor_ticks = [0.91, 0.92, 0.93, 0.94, 0.95, 0.96, 0.97, 0.98, 0.99]
        
        ax.set_xticks(major_ticks)
        ax.set_xticks(minor_ticks, minor=True)
        ax.set_xticklabels([f'{x:.1f}' for x in major_ticks])
    
    # Create legend
    legend_elements = [plt.Rectangle((0,0),1,1, facecolor=colors[j], alpha=0.7, 
                                    label=f'Token Length {tokenlength}') 
                      for j, tokenlength in enumerate(unique_tokenlengths)]
    fig.legend(handles=legend_elements, loc='upper right', bbox_to_anchor=(0.98, 0.98))
    
    # Hide unused subplots
    if n_keybits < len(axes):
        for i in range(n_keybits, len(axes)):
            axes[i].set_visible(False)
    
    # Overall formatting
    fig.suptitle(f'LSH Collision Count Distributions: {data_lines["hashname"].iloc[0]}', 
                 fontsize=16, fontweight='bold')
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    
    # Save figure with border
    plt.savefig(output_filename, dpi=300, bbox_inches='tight', 
                facecolor='white', edgecolor='black', linewidth=2)
    plt.close()
    print(f"  Saved: {output_filename}")


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
    print(f"  Loaded {len(data_lines)} data rows")
    print(f"  Hash function: {data_lines['hashname'].iloc[0]}")
    print(f"  Unique keybits: {sorted(data_lines['keybits'].unique())}")
    print(f"  Unique token lengths: {sorted(data_lines['tokenlength'].unique())}")
    print()
    
    # Generate output filenames
    output_file_1 = os.path.join(output_dir, f"{base_name}_plot1.png")
    output_file_2 = os.path.join(output_dir, f"{base_name}_plot2.png")
    output_file_3 = os.path.join(output_dir, f"{base_name}_plot3.png")
    
    # Generate plots
    generate_line_plots(data_lines, output_file_1)
    generate_boxplot_categorical(data_lines, output_file_2)
    generate_boxplot_numerical(data_lines, output_file_3)
    
    print(f"\n{'='*70}")
    print("All plots generated successfully!")
    print(f"{'='*70}")
    print(f"Output files:")
    print(f"  1. {output_file_1}")
    print(f"  2. {output_file_2}")
    print(f"  3. {output_file_3}")
    print(f"{'='*70}\n")


if __name__ == "__main__":
    main()
