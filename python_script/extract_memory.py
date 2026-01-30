import esptool

# Configuration from your partition table
PORT = '/dev/cu.usbmodem21201'
BAUD = 460800   #115200, 460800
START_OFFSET = '0x270000'   # Start of LittleFS
SIZE_HEX = '0x180000'       # Size of LittleFS (1.5MB)
OUTPUT_FILE = './output_folder/littlefs_raw.bin'

# Updated command for esptool v5.1.0
cmd_args = [
    '--port', PORT,
    '--baud', str(BAUD),
    'read-flash', 
    START_OFFSET, 
    SIZE_HEX, 
    OUTPUT_FILE
]

print(f"Attempting to extract LittleFS from {START_OFFSET}...")

try:
    esptool.main(cmd_args)
    print(f"Success! Data saved to {OUTPUT_FILE}")
except Exception as e:
    print(f"Error: {e}")