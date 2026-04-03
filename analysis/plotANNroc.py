import argparse
import os
import pandas as pd
import matplotlib.pyplot as plt

def plot_roc_curve(df, ax=None, hashname='LSH', savename=None):
    """
    Plots a high-quality ROC-like curve (Recall vs. FPR) for LSH performance evaluation.
    Parameters:
    - df (pd.DataFrame): DataFrame with 'Avg_FPR', 'Avg_Recall', 'b', and 'r' columns.
    - ax (matplotlib.axes.Axes, optional): Matplotlib axes to plot on. If None, a new figure is created.
    - hashname (str): The name of the hash function for the plot title.
    - savename (str, optional): Path to save the figure.
    """
    if ax is None:
        fig, ax = plt.subplots(figsize=(8, 8))
    else:
        fig = ax.figure

    # --- Plotting ---
    # Group data by the 'b' parameter (hashes per band/table). This creates a separate
    # curve for each 'b' value, showing how performance changes as 'r' (number of tables) increases.
    for b_val, group in df.groupby('b'):
        group_sorted = group.sort_values(by='Avg_Recall')
        x = group_sorted['Avg_Recall']
        y = group_sorted['Avg_FPR']
        
        # Plot the curve with markers for each (b, r) point.
        ax.plot(x, y, marker='o', linestyle='None', label=f'b = {b_val}', alpha=1)
        
        # Annotate each point with its corresponding 'r' value for clarity.
        for i, row in group_sorted.iterrows():
            ax.text(row['Avg_Recall'], row['Avg_FPR'], f"{int(row['r'])}", fontsize=9, ha='center', va='bottom')

    # --- Academic Paper Quality Enhancements ---
    ax.set_xlabel('Recall (True Positive Rate)', fontsize=14)
    ax.set_ylabel('False Positive Rate (FPR)', fontsize=14)
    ax.set_title(f'LSH Performance for {hashname}: FPR vs. Recall', fontsize=16)
    
    # Use a symmetric log scale for the x-axis to better visualize low FPR values.
    # This allows plotting zero values while showing distinctions for very small non-zero values.
    ax.set_yscale('symlog', linthresh=1e-5)
    ax.set_xlim(left=0, right=1.05) # Set clean x-axis limits
    ax.set_aspect('auto') # Ensure aspect ratio is not fixed

    # Add a grid for better readability.
    ax.grid(True, which='both', linestyle='--', linewidth=0.5)
    
    # Add a diagonal line representing the performance of a random classifier for reference.
    ax.plot([0, 1], [0, 1], 'k--', label='Random Classifier')
    
    # Add a legend. The title clarifies that 'b' is the number of hashes per band.
    legend = ax.legend(title="b (hashes per band)", fontsize=12)
    plt.setp(legend.get_title(),fontsize=12)

    fig.tight_layout()

    if savename:
        # Save with high DPI for publication quality.
        fig.savefig(savename, dpi=300, bbox_inches='tight')
        print(f"ROC-like curve saved to {savename}")

    return fig, ax

def main():
    """
    Main function to parse command-line arguments and generate the plot.
    """
    parser = argparse.ArgumentParser(
        description="Plot Recall vs. FPR from BioHasher's Approximate Nearest Neighbour test results."
    )
    parser.add_argument(
        "csvfile",
        help="Path to the ApproxNearestNeighbourResults_*.csv file."
    )
    args = parser.parse_args()

    # --- File Validation ---
    if not os.path.isfile(args.csvfile):
        print(f"Error: File not found at '{args.csvfile}'")
        return

    basename = os.path.basename(args.csvfile)
    if not basename.startswith('ApproxNearestNeighbourResults_'):
        print(f"Warning: The input file '{basename}' does not seem to be a standard ANN result file.")

    # --- Data Loading ---
    try:
        df = pd.read_csv(args.csvfile)
        # Verify that the CSV contains the necessary columns.
        if not {'b', 'r', 'Avg_FPR', 'Avg_Recall'}.issubset(df.columns):
            print("Error: CSV file is missing required columns: 'b', 'r', 'Avg_FPR', 'Avg_Recall'.")
            return
    except Exception as e:
        print(f"Error reading or parsing CSV file: {e}")
        return

    # --- Plotting ---
    # Extract the hash name from the filename to use in the title and saved plot name.
    hashname = basename.replace('ApproxNearestNeighbourResults_', '').replace('.csv', '')
    
    savename = f"{hashname}_roc_curve.png"

    plot_roc_curve(df, hashname=hashname, savename=savename)
    
    # Display the plot.
    plt.show()

if __name__ == "__main__":
    main()
