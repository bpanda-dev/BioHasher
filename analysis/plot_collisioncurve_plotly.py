import argparse
import os
import sys
import pandas as pd
import numpy as np
import plotly.graph_objects as go
import plotly.express as px
import ast

mutation_expressions = {0:"BALANCED",1:"SUB_ONLY",2:"DEL_LITE",3:"INS_LITE",4:"SUB_LITE"}

def safe_literal_eval(val):
    if isinstance(val, str):
        try:
            return ast.literal_eval(val)
        except:
            return val
    return val

def read_processed_dataframe(csv_path):
    df = pd.read_csv(csv_path)
    # Reconstruct array columns from strings
    list_cols = [
        'similarity_values', 'snp_rate', 'delRate', 'insmean', 'insrate',
        'stayRate', 'collision_rates', 'rand_base_params', 'parameter_columns'
    ]
    for col in list_cols:
        if col in df.columns:
            df[col] = df[col].apply(safe_literal_eval)
    return df

def generate_row_label(row):
    mutation_model_used = ""
    if row['MutationModel'] == 0:
        mutation_model_used = "SUB-ONLY"
    elif row['MutationModel'] == 1:
        mutation_model_used = "GEO-" + mutation_expressions.get(row["MutationExpression"], "")

    param_labels = ""
    # Safe handling of parameter columns depending on whether it's list or missing
    if 'parameter_columns' in row and isinstance(row['parameter_columns'], list):
        param_labels = ", ".join([f"{name}={row.get(name, '?')}" for name in row['parameter_columns']])

    and_p = str(row.get('AND_param', '1'))
    or_p = str(row.get('OR_param', '1'))

    if and_p == '1' and or_p == '1':
        return f"{row['hashname']} (L={row['sequencelength']}, {param_labels})[{mutation_model_used}]"
    else:
        return f"{row['hashname']} (L={row['sequencelength']}, AND={and_p}, OR={or_p}, {param_labels})[{mutation_model_used}]"

def plot_scatter(df):
    fig = go.Figure()

    for idx, row in df.iterrows():
        label = generate_row_label(row)
        x_vals = row['similarity_values']
        y_vals = row['collision_rates']

        # Optionally size/color by snp_rate if valid
        snp = row.get('snp_rate', None)

        fig.add_trace(go.Scatter(
            x=x_vals, y=y_vals,
            mode='markers',
            name=label,
            # marker=dict(color=snp, colorscale='Viridis', showscale=True) if snp is not None else dict(),
            marker=dict(size=6, opacity=0.6),
            text=[f"SNP: {s}" if snp else "" for s in (snp if snp else [])],
            hovertemplate='Similarity: %{x:.4f}<br>Collision Rate: %{y:.4f}<br>%{text}'
        ))

    fig.update_layout(
        title="Collision Rates vs Similarity (Scatter)",
        xaxis_title="Similarity",
        yaxis_title="Collision Rates",
        xaxis=dict(range=[0, 1.01]),
        yaxis=dict(range=[0, 1.01]),
        template="plotly_white",
        legend=dict(x=0.01, y=0.99, bgcolor='rgba(255,255,255,0.7)')
    )

    return fig

def plot_binned_average(df):
    fig = go.Figure()

    bin_edges = np.arange(0, 1.04, 0.04)
    bin_centers = (bin_edges[:-1] + bin_edges[1:]) / 2

    for idx, row in df.iterrows():
        label = generate_row_label(row)
        x_vals = np.array(row['similarity_values'])
        y_vals = np.array(row['collision_rates'])

        bin_means = []
        for i in range(len(bin_edges) - 1):
            mask = (x_vals >= bin_edges[i]) & (x_vals < bin_edges[i + 1])
            if mask.sum() > 0:
                bin_means.append(np.mean(y_vals[mask]))
            else:
                bin_means.append(np.nan)

        valid = ~np.isnan(bin_means)

        fig.add_trace(go.Scatter(
            x=bin_centers[valid],
            y=np.array(bin_means)[valid],
            mode='lines+markers',
            name=label,
            marker=dict(size=8),
            line=dict(width=2)
        ))

    fig.update_layout(
        title="Average Collision Rate per Bin",
        xaxis_title="Similarity",
        yaxis_title="Average Collision Rate",
        xaxis=dict(range=[0, 1.01]),
        yaxis=dict(range=[0, 1.01]),
        template="plotly_white",
        legend=dict(x=0.01, y=0.99, bgcolor='rgba(255,255,255,0.7)')
    )

    return fig


def plot_box_plot(df, num_bins=26):
    fig = go.Figure()

    bin_edges = np.linspace(0, 1.05, num_bins + 1)

    for idx, row in df.iterrows():
        label = generate_row_label(row)
        x_vals = np.array(row['similarity_values'])
        y_vals = np.array(row['collision_rates'])

        # Bin data
        binned_y = []
        binned_x = []
        for j in range(len(x_vals)):
            sim_value = x_vals[j]
            bin_idx = int(sim_value * num_bins)
            if bin_idx >= num_bins:
                bin_idx = num_bins - 1

            # Use bin_edge left point or center as x label
            binned_x.append(f"{bin_edges[bin_idx]:.2f}")
            binned_y.append(y_vals[j])

        fig.add_trace(go.Box(
            x=binned_x,
            y=binned_y,
            name=label,
            boxpoints=False, # to hide outliers like original implementation
            marker_color=px.colors.qualitative.Plotly[idx % len(px.colors.qualitative.Plotly)]
        ))

    fig.update_layout(
        title="Collision Rates Boxplot per Bin",
        xaxis_title="Similarity Bin Start",
        yaxis_title="Collision Rates",
        boxmode='group', # Group boxes of different rows together
        template="plotly_white",
        yaxis=dict(range=[0, 1.05]),
        legend=dict(title="Hash Configuration")
    )

    # Sort x-axis by parsing float so "0.00" defaults appear in order
    fig.update_xaxes(categoryorder='array', categoryarray=[f"{v:.2f}" for v in bin_edges])

    return fig


def main():
    parser = argparse.ArgumentParser(description="Generate interactive Plotly visualizations for BioHasher processed metrics.")
    parser.add_argument("processed_csv", help="Path to the *_processed.csv file output by plot_collisioncurves.py")
    parser.add_argument("--show", action="store_true", help="Launch browser with plots immediately")
    args = parser.parse_args()

    if not os.path.isfile(args.processed_csv):
        print(f"File not found: {args.processed_csv}")
        sys.exit(1)

    df = read_processed_dataframe(args.processed_csv)

    out_dir = os.path.dirname(os.path.abspath(args.processed_csv))
    basename = os.path.basename(args.processed_csv).replace("_processed.csv", "")

    fig_scatter = plot_scatter(df)
    fig_bin = plot_binned_average(df)
    fig_box = plot_box_plot(df)

    # Extract banner information
    hashname = df.iloc[0].get('hashname', 'Unknown Hash')
    length = df.iloc[0].get('sequencelength', 'N/A')
    token_len = df.iloc[0].get('tokenlength', 'N/A')
    mut_mod_idx = df.iloc[0].get('MutationModel', 0)
    mut_exp_idx = df.iloc[0].get('MutationExpression', 0)

    if mut_mod_idx == 0:
        mutation_str = "SUB-ONLY"
    else:
        mutation_str = "GEO-" + mutation_expressions.get(mut_exp_idx, "")

    # Render Plotly to HTML divs
    scatter_div = fig_scatter.to_html(full_html=False, include_plotlyjs=False)
    binned_div = fig_bin.to_html(full_html=False, include_plotlyjs=False)
    box_div = fig_box.to_html(full_html=False, include_plotlyjs=False)

    # Create unified HTML dashboard with tabs
    html_template = f"""<!DOCTYPE html>
<html>
<head>
    <title>{hashname} Evaluation Panel</title>
    <script src="https://cdn.plot.ly/plotly-latest.min.js"></script>
    <style>
        body {{ font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 0; background-color: #f4f6f9; color: #333; }}
        .header-banner {{ 
            background: linear-gradient(135deg, #2c3e50, #3498db);
            color: white; 
            padding: 25px 20px; 
            text-align: center; 
            box-shadow: 0 4px 6px rgba(0,0,0,0.1); 
        }}
        .header-banner h1 {{ margin: 0 0 10px 0; font-size: 28px; letter-spacing: 1px; }}
        .header-banner p {{ margin: 0; font-size: 16px; opacity: 0.9; }}
        
        .tab {{ 
            display: flex; 
            justify-content: center; 
            background-color: #fff; 
            border-bottom: 2px solid #dee2e6; 
            margin-bottom: 20px;
        }}
        .tab button {{ 
            background-color: inherit; 
            border: none; 
            outline: none; 
            cursor: pointer; 
            padding: 15px 30px; 
            transition: 0.3s; 
            font-size: 16px; 
            font-weight: 600; 
            color: #6c757d; 
            border-bottom: 3px solid transparent; 
        }}
        .tab button:hover {{ color: #007bff; background-color: #f8f9fa; }}
        .tab button.active {{ color: #007bff; border-bottom: 3px solid #007bff; }}
        
        .tabcontent {{ 
            display: none; 
            padding: 0 20px 40px 20px; 
            animation: fadeEffect 0.5s; 
        }}
        @keyframes fadeEffect {{ from {{opacity: 0;}} to {{opacity: 1;}} }}
        
        .tabcontent.active {{ display: block; }}
        
        .plot-container {{ 
            max-width: 1400px; 
            margin: 0 auto; 
            background: white; 
            border-radius: 8px; 
            box-shadow: 0 4px 12px rgba(0,0,0,0.05); 
            padding: 20px; 
            overflow: hidden;
        }}
    </style>
</head>
<body>

<div class="header-banner">
    <h1>{hashname} Analysis Dashboard</h1>
    <p>Length: {length} &nbsp;|&nbsp; Token Length: {token_len} &nbsp;|&nbsp; Mutation Model: {mutation_str}</p>
</div>

<div class="tab">
  <button class="tablinks active" onclick="openPlot(event, 'Scatter')">Scatter Plot</button>
  <button class="tablinks" onclick="openPlot(event, 'Binned')">Binned Average Plot</button>
  <button class="tablinks" onclick="openPlot(event, 'Box')">Box Plot</button>
</div>

<div id="Scatter" class="tabcontent active">
  <div class="plot-container">{scatter_div}</div>
</div>

<div id="Binned" class="tabcontent">
  <div class="plot-container">{binned_div}</div>
</div>

<div id="Box" class="tabcontent">
  <div class="plot-container">{box_div}</div>
</div>

<script>
function openPlot(evt, plotName) {{
  var i, tabcontent, tablinks;
  tabcontent = document.getElementsByClassName("tabcontent");
  for (i = 0; i < tabcontent.length; i++) {{
    tabcontent[i].className = tabcontent[i].className.replace(" active", "");
  }}
  tablinks = document.getElementsByClassName("tablinks");
  for (i = 0; i < tablinks.length; i++) {{
    tablinks[i].className = tablinks[i].className.replace(" active", "");
  }}
  document.getElementById(plotName).className += " active";
  evt.currentTarget.className += " active";
  
  // Resize plotly charts inside hidden tabs that are now shown to fix layout issues
  window.dispatchEvent(new Event('resize'));
}}
</script>

</body>
</html>
"""

    dashboard_path = os.path.join(out_dir, f"{basename}_dashboard.html")
    with open(dashboard_path, "w", encoding="utf-8") as f:
        f.write(html_template)

    print(f"Dashboard generated successfully at: {dashboard_path}")

    if args.show:
        import webbrowser
        webbrowser.open('file://' + os.path.realpath(dashboard_path))

if __name__ == "__main__":
    main()
