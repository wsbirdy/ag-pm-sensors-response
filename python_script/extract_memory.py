import esptool
import sys
import serial.tools.list_ports

def get_esp_port():
    # Find all potential USB serial ports
    ports = [p.device for p in serial.tools.list_ports.comports() if "USB" in p.description or "UART" in p.description]
    
    if not ports:
        print("❌ No ESP32 devices found. Please check your connection.")
        sys.exit(1)
    
    if len(ports) == 1:
        print(f"✅ Found device on: {ports[0]}")
        return ports[0]
    
    # If multiple are found, let the user choose
    print("\nMultiple devices detected:")
    for i, port in enumerate(ports):
        print(f"[{i}] {port}")
    
    choice = input(f"Select port (0-{len(ports)-1}): ")
    try:
        return ports[int(choice)]
    except (ValueError, IndexError):
        print("Invalid selection.")
        sys.exit(1)

# Configuration from your partition table
PORT = get_esp_port() #'/dev/cu.usbmodem21201'
BAUD = 460800   #115200, 460800
START_OFFSET = '0x270000'   # Start of LittleFS
SIZE_HEX = '0x180000'       # Size of LittleFS (1.5MB)
OUTPUT_FILE = './output_folder/littlefs_raw.bin'

# Updated command for esptool
cmd_args = [
    '--port', PORT,
    '--baud', str(BAUD), 
    'read-flash', 
    START_OFFSET, 
    SIZE_HEX, 
    OUTPUT_FILE]

print(f"Attempting to extract LittleFS from {START_OFFSET}...")

try:
    esptool.main(cmd_args)
    print(f"Success! Data saved to {OUTPUT_FILE}")
except Exception as e:
    print(f"Error: {e}")