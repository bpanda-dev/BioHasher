#!/bin/bash

# Configuration
START_K=1
END_K=35
STEP=1
BASE_DIR=$(pwd)
RESULTS_DIR="$BASE_DIR/results_sweep"
BUILD_DIR="$BASE_DIR/build"

# Create results directory
mkdir -p "$RESULTS_DIR"

echo "=========================================="
echo "BioHasher SUBSEQUENCE_LENGTH Sweep"
echo "Range: $START_K to $END_K (step: $STEP)"
echo "=========================================="

for k in $(seq $START_K $STEP $END_K); do
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Processing SUBSEQUENCE_LENGTH=$k"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    # Clean build directory
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure
    echo "[1/3] Configuring CMake..."
    if ! cmake .. -DSUBSEQUENCE_LENGTH=$k -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1; then
        echo "CMake configuration failed"
        continue
    fi
    echo "CMake configured"
    
    # Compile
    echo "[2/3] Building..."
    if ! make -j$(nproc) > /dev/null 2>&1; then
        echo "Build failed"
        continue
    fi
    echo "Build successful"
    
    # Run
    echo "[3/3] Running tests..."
    if [ -f "./BioHasher" ]; then
        # Run your tests and save output
        if ./BioHasher --test=LSHCollision SubseqHash-64 --ncpu=128 > "$RESULTS_DIR/output_k$k.txt" 2>&1; then
            echo "Tests completed for k=$k"
            # Optional: Show summary
            # head -n 20 "$RESULTS_DIR/output_k$k.txt"
        else
            echo "Tests failed for k=$k"
        fi
    else
        echo "Executable not found"
    fi
    
    cd "$BASE_DIR"
done

echo ""
echo "=========================================="
echo "All iterations completed!"
echo "Results saved to: $RESULTS_DIR"
echo "=========================================="

# Optional: Aggregate results
# echo ""
# echo "Summary:"
# ls -lh "$RESULTS_DIR"/output_k*.txt 2>/dev/null | awk '{print $9, "-", $5}'