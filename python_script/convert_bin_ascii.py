import struct
import argparse
import os
import sys

# ... rest of your conversion code ...
# PACKET_FORMAT Breakdown:
# <  : Little-endian (Standard for ESP32-C3)
# 2s : char[2] (Header 'OA')
# I  : uint32_t (Counter)
# I  : uint32_t (Timestamp)
# H  : uint16_t (SPS30 Particles)
# H  : uint16_t (SPS30 Concentration)
# H  : uint16_t (PMSA003I Particles)
# H  : uint16_t (PMSA003I Concentration)
# H  : uint16_t (PM2012 Particles)
# H  : uint16_t (PM2012 Concentration GRIMM)
# H  : uint16_t (PM2012 Concentration TSI)
# H  : uint16_t (PM2016 Particles)
# H  : uint16_t (PM2016 Concentration)
# 2B : uint8_t x 2 (Terminator 0xAA, 0xBB)
# Total Size: 30 bytes
PACKET_FORMAT = '<2sIIHHHHHHHHHBB'
PACKET_SIZE = struct.calcsize(PACKET_FORMAT)
HEADER = b'OA'
FOOTER = (0xAA, 0xBB)
# --- Configuration (Hardcoded) ---
OUTPUT_DIR = "./decoded_results"  # Change this to your desired path
# --------------------------------

def decode_sensor_file(input_filename):
    if not os.path.exists(input_filename):
        print(f"Error: File '{input_filename}' not found.")
        return

    # 1. Ensure the output directory exists
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)
        print(f"Created directory: {OUTPUT_DIR}")

    # 2. Construct output path: Directory + original filename + .csv
    base_name = os.path.basename(input_filename) # Extract filename from path
    file_no_ext = os.path.splitext(base_name)[0]
    output_filename = os.path.join(OUTPUT_DIR, file_no_ext + ".csv")

    print(f"Scanning {input_filename} for valid packets...")

    with open(input_filename, 'rb') as f:
        raw_data = f.read()

    records_saved = 0
    with open(output_filename, 'w') as csv_file:
        csv_file.write("Counter,Timestamp_ms,SPS30_Particles,SPS30_Conc,PMSA_Particles,PMSA_Conc,PM2012_Particles,PM2012_Conc_GRIMM,PM2012_Conc_TSI,PM2016_Particles,PM2016_Conc\n")
        
        i = 0
        # Scan through the raw bytes one by one
        while i <= len(raw_data) - PACKET_SIZE:
            # Look for the Header 'OA'
            if raw_data[i:i+2] == HEADER:
                # Potential packet found, extract the full 20-byte block
                potential_packet = raw_data[i : i + PACKET_SIZE]
                
                try:
                    # Unpack the block
                    header, count, ts, s1_p, s1_c, s2_p, s2_c, s3_p, s3_c_grimm, s3_c_tsi, s4_p, s4_c, t1, t2 = struct.unpack(PACKET_FORMAT, potential_packet)
                    print(f"whole packet: {potential_packet.hex()}")
                    # Verify the Terminator (0xAA, 0xBB)
                    if (t1, t2) == FOOTER:
                        csv_file.write(f"{count},{ts},{s1_p},{s1_c},{s2_p},{s2_c},{s3_p},{s3_c_grimm},{s3_c_tsi},{s4_p},{s4_c},\n")
                        records_saved += 1
                        i += PACKET_SIZE  # Valid packet, skip ahead 20 bytes
                        continue
                except struct.error:
                    pass
            
            # If not a valid packet, move forward by only 1 byte to keep searching
            i += 1

    print(f"Finished! Successfully decoded {records_saved} valid records into '{output_filename}'.")

if __name__ == "__main__":

    # Check if a filename was passed as an argument
    if len(sys.argv) > 1:
        input_file = sys.argv[1]
    else:
        input_file = "littlefs_raw.bin" # Default fallback
    print(f"Processing: {input_file}")

    parser = argparse.ArgumentParser(description='Robust Binary Decoder for ESP32-C3')
    parser.add_argument('filename', help='The binary log file to process')
    args = parser.parse_args()
    
    decode_sensor_file(args.filename)

# import struct
# import argparse
# import os

# # PACKET_FORMAT Breakdown:
# # <  : Little-endian (Correct for ESP32-C3 / RISC-V)
# # 2s : char[2] (Header 'OA')
# # I  : uint32_t (Counter)
# # I  : uint32_t (Timestamp)
# # H  : uint16_t (SPS30 Particles)
# # H  : uint16_t (SPS30 Concentration)
# # H  : uint16_t (PMSA003I Particles)
# # H  : uint16_t (PMSA003I Concentration)
# # BB : uint8_t (Terminator 0xAA)
# # BB : uint8_t (Terminator 0xBB)
# # Total: 20 bytes
# PACKET_FORMAT = '<2sBBBBBBBB'
# PACKET_SIZE = struct.calcsize(PACKET_FORMAT)

# def decode_sensor_file(input_filename):
#     if not os.path.exists(input_filename):
#         print(f"Error: File '{input_filename}' not found.")
#         return

#     # Generate CSV filename based on input filename
#     output_filename = os.path.splitext(input_filename)[0] + ".csv"
#     print(f"Decoding {input_filename} -> {output_filename}...")

#     with open(input_filename, 'rb') as bin_file, open(output_filename, 'w') as csv_file:
#         # Write CSV Header
#         csv_file.write("Counter,Timestamp_ms,SPS30_Particles,SPS30_Conc,PMSA_Particles,PMSA_Conc\n")
        
#         records_saved = 0
#         while True:
#             # Step 1: Sync to Header 'O'
#             byte = bin_file.read(1)
#             if not byte: break # EOF
            
#             if byte == b'O':
#                 # Step 2: Check for 'A'
#                 next_byte = bin_file.read(1)
#                 if next_byte == b'A':
#                     # Step 3: Read the rest of the 20-byte packet (18 bytes remaining)
#                     payload = bin_file.read(PACKET_SIZE - 2)
#                     print(f"Read payload: {payload.hex()}")
#                     if len(payload) < (PACKET_SIZE - 2):
#                         break
                    
#                     full_packet = b'OA' + payload
                    
#                     try:
#                         # Unpack the data
#                         header, count, ts, s_p, s_c, p_p, p_c, term1, term2 = struct.unpack(PACKET_FORMAT, full_packet)
                        
#                         # Step 4: Verify Terminator (\r\n is 0x0D 0x0A)
#                         if term1 == 0xAA and term2 == 0xBB:
#                             csv_file.write(f"{count},{ts},{s_p},{s_c},{p_p},{p_c}\n")
#                             records_saved += 1
#                         else:
#                             print(f"Warning: Invalid terminator at record {count}. Skipping. term1={term1:#x}, term2={term2:#x}")
#                     except struct.error as e:
#                         print(f"Unpacking error: {e}")
#                         continue

#     print(f"Finished! Successfully decoded {records_saved} records.")

# if __name__ == "__main__":
#     parser = argparse.ArgumentParser(description='Binary Decoder for ESP32-C3 Sensor Logs')
#     parser.add_argument('filename', help='The .bin file to decode')
#     args = parser.parse_args()
    
#     decode_sensor_file(args.filename)