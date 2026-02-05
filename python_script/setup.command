#!/bin/bash
cd "$(dirname "$0")"

echo "------------------------------------------"
echo "🛠️  Initial Setup: Installing Dependencies"
echo "------------------------------------------"

# 1. Install mklittlefs via Homebrew if missing
if ! command -v mklittlefs &> /dev/null; then
    echo "mklittlefs not found. Attempting to install via Homebrew..."
    if ! command -v brew &> /dev/null; then
        echo "❌ Homebrew is not installed. Please install it first at https://brew.sh"
        exit
    fi
    brew install mklittlefs
fi

# 2. Create Virtual Environment
if [ ! -d "venv" ]; then
    echo "📦 Creating virtual environment..."
    python3 -m venv venv
fi

# 3. Install Python requirements
source venv/bin/activate
echo "📥 Installing Python libraries..."

# This command installs everything listed in your requirements.txt
if [ -f "requirements.txt" ]; then
    pip install -r requirements.txt
else
    echo "⚠️  requirements.txt not found. Installing defaults..."
    pip install esptool pyserial pandas matplotlib
fi

echo "------------------------------------------"
echo "✅ Setup complete! You can now use run_decoder.command"
read -n 1 -p "Press any key to close..."