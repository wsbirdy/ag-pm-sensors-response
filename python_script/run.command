#!/bin/bash
cd "$(dirname "$0")"

# Check if setup was done
if [ ! -d "venv" ]; then
    echo "❌ Error: Virtual environment not found."
    echo "Please run setup_env.command first."
    exit
fi

source venv/bin/activate

echo "------------------------------------------"
echo "🚀 Starting Data Extraction"
echo "------------------------------------------"

# Runs the automated coordinator
python3 automated_script.py

echo "------------------------------------------"
echo "✅ Process Complete!"
open "./decoded_results"  # Opens the folder for them automatically
read -n 1 -p "Press any key to exit..."