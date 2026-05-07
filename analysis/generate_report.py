import argparse
import subprocess
import sys
import os

def get_similarity_name(filepath):
    try:
        with open(filepath, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith(':3:'):
                    parts = line[3:].strip().split(',')
                    if len(parts) > 2:
                        return parts[2].strip()
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
    return "results"

def main():
    parser = argparse.ArgumentParser(description="Generate BioHasher benchmarking dashboard from raw results.")
    parser.add_argument("--ann", type=str, help="Path to the raw ANN test output CSV file.")
    parser.add_argument("--coll", type=str, help="Path to the raw Collision Curve test output CSV file.")
    parser.add_argument("--show", action="store_true", help="Launch browser with the generated dashboard immediately.")
    
    args = parser.parse_args()
    
    if not args.ann and not args.coll:
        print("Error: You must provide at least one of --ann or --coll.")
        parser.print_help()
        sys.exit(1)
        
    script_dir = os.path.dirname(os.path.abspath(__file__))
    processed_csvs = []
    
    # Process Collision Curve data
    if args.coll:
        if not os.path.exists(args.coll):
            print(f"Error: Collision file '{args.coll}' not found.")
            sys.exit(1)
            
        print(f"--- Processing Collision Curve data: {args.coll} ---")
        cmd = [sys.executable, os.path.join(script_dir, "plot_collisioncurve.py"), args.coll]
        try:
            subprocess.run(cmd, check=True)
        except subprocess.CalledProcessError:
            print("Error occurred while plotting collision curve.")
            sys.exit(1)
            
        similarity_name = get_similarity_name(args.coll)
        csv_dir = os.path.dirname(os.path.abspath(args.coll))
        processed_file = os.path.join(csv_dir, similarity_name, f"{similarity_name}_processed_coll.csv")
        
        if os.path.exists(processed_file):
            processed_csvs.append(processed_file)
        else:
            print(f"Warning: Expected processed file not found: {processed_file}")

    # Process ANN data
    if args.ann:
        if not os.path.exists(args.ann):
            print(f"Error: ANN file '{args.ann}' not found.")
            sys.exit(1)
            
        print(f"--- Processing ANN data: {args.ann} ---")
        cmd = [sys.executable, os.path.join(script_dir, "plot_ann.py"), args.ann]
        try:
            subprocess.run(cmd, check=True)
        except subprocess.CalledProcessError:
            print("Error occurred while plotting ANN data.")
            sys.exit(1)
            
        similarity_name = get_similarity_name(args.ann)
        csv_dir = os.path.dirname(os.path.abspath(args.ann))
        processed_file = os.path.join(csv_dir, similarity_name, f"{similarity_name}_processed_ann.csv")
        
        if os.path.exists(processed_file):
            processed_csvs.append(processed_file)
        else:
            print(f"Warning: Expected processed file not found: {processed_file}")

    # Generate Dashboard
    if not processed_csvs:
        print("No processed CSV files were generated. Cannot create dashboard.")
        sys.exit(1)
        
    print("\n--- Generating Dashboard ---")
    main_cmd = [sys.executable, os.path.join(script_dir, "webplotter.py")] + processed_csvs
    if args.show:
        main_cmd.append("--show")
        
    try:
        subprocess.run(main_cmd, check=True)
    except subprocess.CalledProcessError:
        print("Error occurred while generating dashboard.")
        sys.exit(1)
        
if __name__ == "__main__":
    main()
