import subprocess
import os
import glob
import shutil

# --- Configuration ---
EXTRACT_SCRIPT = "extract_memory.py"
DECODE_SCRIPT = "convert_bin_ascii.py"
RAW_BIN = "./output_folder/littlefs_raw.bin"
OUTPUT_DIR = "./extracted_files"
DECODED_DIR = "./decoded_results"

def run_command(command_list, description):
    print(f"\n>>> Running: {description}...")
    result = subprocess.run(command_list)
    if result.returncode != 0:
        print(f"!!! Error during {description}. Process aborted.")
        return False
    return True

def main():
    # 1. Clean up previous runs
    for folder in [OUTPUT_DIR, DECODED_DIR]:
        if os.path.exists(folder):
            shutil.rmtree(folder)
        os.makedirs(folder)

    # 2. Extract Raw Binary from ESP32
    # This runs your existing extract_memory.py script
    if not run_command(["python3", EXTRACT_SCRIPT], "Hardware Extraction"):
        return

    # 3. Unpack the LittleFS Image
    # Using the specific parameters you provided
    mklittlefs_cmd = [
        "mklittlefs", 
        "-u", OUTPUT_DIR, 
        "-b", "4096", 
        "-p", "256", 
        "-s", "0x180000", 
        RAW_BIN
    ]
    if not run_command(mklittlefs_cmd, "Unpacking LittleFS"):
        return

    # 4. Decode each extracted file
    print(f"\n>>> Decoding individual files...")
    # Find all files in the output directory
    extracted_files = glob.glob(os.path.join(OUTPUT_DIR, "*"))
    
    if not extracted_files:
        print("No files found inside the LittleFS partition. Is it formatted?")
        return

    for file_path in extracted_files:
        filename = os.path.basename(file_path)
        # Skip directories if any exist
        if os.path.isdir(file_path):
            continue
            
        print(f"  Processing: {filename}")
        
        # Run your decoder script on this specific file
        # We output the result to the DECODED_DIR
        decode_cmd = ["python3", DECODE_SCRIPT, file_path]
        subprocess.run(decode_cmd)

    print(f"\n🎉 Done! Files extracted to {OUTPUT_DIR} and decoded.")

if __name__ == "__main__":
    main()