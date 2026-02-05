import struct
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np  # Added for skipping 65535
import sys
import os

def plot_data(file_path):
    # ========================================================
    # 📝 CONFIGURATION SECTION
    # ========================================================
    CFG = {
        "x_label": "Raw Timestamp (ms)", # Updated to Raw
        "y_label_p": "Particles (count)",
        "y_label_c": "Concentration (µg/m³)",
        "legends": {
            "S1": "SPS30",
            "S2": "PMSA003I",
            "S3": "PM2012",
            "S4": "PM2016"
        },
        "line_width": 1.6,
        "fig_size": (10, 6)
    }
    # ========================================================

    ext = os.path.splitext(file_path)[1].lower()
    
    # 1. Loading and Mapping Data 
    if ext == '.csv':
        # index_col=False fixes the data shift issue
        df = pd.read_csv(file_path, index_col=False)
        column_map = {
            'SPS30_Particles': 'S1_P', 
            'SPS30_Conc':      'S1_C',
            'PMSA_Particles':  'S2_P', 
            'PMSA_Conc':       'S2_C',
            'PM2012_Particles': 'S3_P', 
            'PM2012_Conc_GRIMM': 'S3_C_Grimm',
            'PM2012_Conc_TSI':   'S3_C_TSI',
            'PM2016_Particles': 'S4_P', 
            'PM2016_Conc':      'S4_C',
            'Timestamp_ms':    'Timestamp_ms'
        }
        df = df.rename(columns=column_map)
    else:
        records = []
        with open(file_path, 'rb') as f:
            while True:
                byte = f.read(1)
                if not byte: break
                if byte != b'O': continue
                if f.read(1) != b'A': continue
                data = f.read(28)
                if len(data) < 28: break
                try:
                    seq, ts = struct.unpack('<II', data[0:8])
                    vals = struct.unpack('<9H', data[8:26])
                    records.append({
                        'Timestamp_ms': ts,
                        'S1_P': vals[0], 'S1_C': vals[1],
                        'S2_P': vals[2], 'S2_C': vals[3],
                        'S3_P': vals[4], 'S3_C_Grimm': vals[5], 'S3_C_TSI': vals[6],
                        'S4_P': vals[7], 'S4_C': vals[8]
                    })
                except: break
        df = pd.DataFrame(records)

    if df.empty:
        print("❌ No data found.")
        return

    df = df.sort_values('Timestamp_ms')
    
    # --- FEATURE: Saturation Filter (Skip 65535) ---
    # Identify data columns (Sensor readings)
    sensor_cols = [c for c in df.columns if c not in ['Counter', 'Timestamp_ms']]
    for col in sensor_cols:
        # Replace the max 16-bit value with NaN so it's skipped in plots
        df[col] = df[col].replace(65535, np.nan)
    # -----------------------------------------------

    base_name = os.path.splitext(file_path)[0]
    x_data = df['Timestamp_ms'] # Using Raw Timestamp as requested
    
    # helper to save and print
    def finalize_plot(filename, title, y_label):
        plt.title(title, fontsize=14, fontweight='bold')
        plt.xlabel(CFG["x_label"])
        plt.ylabel(y_label)
        plt.xticks(rotation=45) # Rotate for readability
        plt.grid(True, alpha=0.3)
        plt.legend(loc='upper right')
        plt.tight_layout()
        save_path = f"{base_name}_{filename}.png"
        plt.savefig(save_path, dpi=300)
        print(f"✅ Saved: {save_path}")
        plt.close()

    # --- PART 1: OVERALL COMPARISON PLOTS ---

    # Plot 1: Comparison - All Particles
    plt.figure(figsize=CFG["fig_size"])
    for i in range(1, 5): # Fix: include S4
        key = f'S{i}_P'
        if key in df.columns:
            plt.plot(x_data, df[key], label=CFG["legends"][f"S{i}"], linewidth=CFG["line_width"])
    finalize_plot("Comparison_Particles_Raw", "Global Comparison: Particle Counts", CFG["y_label_p"])

    # Plot 2: Comparison - All Concentrations
    plt.figure(figsize=CFG["fig_size"])
    plt.plot(x_data, df['S1_C'], label=CFG["legends"]["S1"])
    plt.plot(x_data, df['S2_C'], label=CFG["legends"]["S2"])
    plt.plot(x_data, df['S3_C_Grimm'], label=f'{CFG["legends"]["S3"]} (Grimm)')
    plt.plot(x_data, df['S3_C_TSI'], label=f'{CFG["legends"]["S3"]} (TSI)', linestyle='--')
    plt.plot(x_data, df['S4_C'], label=CFG["legends"]["S4"])
    finalize_plot("Comparison_Concentrations_Raw", "Global Comparison: PM Concentrations", CFG["y_label_c"])

    # --- PART 2: INDIVIDUAL SENSOR PLOTS (SEPARATED) ---

    sensors = [
        {"id": "S1", "name": "SPS30", "P": "S1_P", "C": ["S1_C"]},
        {"id": "S2", "name": "PMSA", "P": "S2_P", "C": ["S2_C"]},
        {"id": "S3", "name": "PM2012", "P": "S3_P", "C": ["S3_C_Grimm", "S3_C_TSI"]},
        {"id": "S4", "name": "PM2016", "P": "S4_P", "C": ["S4_C"]}
    ]

    for s in sensors:
        # Generate Particles Plot
        plt.figure(figsize=CFG["fig_size"])
        plt.plot(x_data, df[s["P"]], color='tab:blue', label=f"{s['name']} Particles")
        finalize_plot(f"{s['name']}_Particles_Raw", f"{s['name']}: Particle Data", CFG["y_label_p"])

        # Generate Concentration Plot
        plt.figure(figsize=CFG["fig_size"])
        for i, c_col in enumerate(s["C"]):
            label = c_col.split('_')[-1] if len(s["C"]) > 1 else "Conc"
            style = '-' if i == 0 else '--'
            plt.plot(x_data, df[c_col], color='tab:red', linestyle=style, label=f"{s['name']} {label}")
        finalize_plot(f"{s['name']}_Concentration_Raw", f"{s['name']}: Concentration Data", CFG["y_label_c"])

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 plotSensorData.py <data.csv/bin>")
    else:
        plot_data(sys.argv[1])


# import struct
# import pandas as pd
# import matplotlib.pyplot as plt
# import sys
# import os

# def plot_data(file_path):
#     # ========================================================
#     # 📝 CONFIGURATION SECTION
#     # ========================================================
#     CFG = {
#         "x_label": "Elapsed Time (seconds)",
#         "y_label_p": "Particles (count)",
#         "y_label_c": "Concentration (µg/m³)",
#         "legends": {
#             "S1": "SPS30",
#             "S2": "PMSA003I",
#             "S3": "PM2012",
#             "S4": "PM2016"
#         },
#         "line_width": 1.6,
#         "fig_size": (10, 6)
#     }
#     # ========================================================

#     ext = os.path.splitext(file_path)[1].lower()
    
#     # 1. Loading and Mapping Data 
#     if ext == '.csv':
#         # Force pandas to read the file without using the first column as an index
#         df = pd.read_csv(file_path, index_col=False)
#         column_map = {
#             'SPS30_Particles': 'S1_P', 
#             'SPS30_Conc':      'S1_C',
#             'PMSA_Particles':  'S2_P', 
#             'PMSA_Conc':       'S2_C',
#             'PM2012_Particles': 'S3_P', 
#             'PM2012_Conc_GRIMM': 'S3_C_Grimm',
#             'PM2012_Conc_TSI':   'S3_C_TSI',
#             'PM2016_Particles': 'S4_P', 
#             'PM2016_Conc':      'S4_C',
#             'Timestamp_ms':    'Timestamp_ms'
#         }
#         df = df.rename(columns=column_map)
#     else:
#         records = []
#         with open(file_path, 'rb') as f:
#             while True:
#                 byte = f.read(1)
#                 if not byte: break
#                 if byte != b'O': continue
#                 if f.read(1) != b'A': continue
#                 data = f.read(28)
#                 if len(data) < 28: break
#                 try:
#                     seq, ts = struct.unpack('<II', data[0:8])
#                     vals = struct.unpack('<9H', data[8:26])
#                     records.append({
#                         'Timestamp_ms': ts,
#                         'S1_P': vals[0], 'S1_C': vals[1],
#                         'S2_P': vals[2], 'S2_C': vals[3],
#                         'S3_P': vals[4], 'S3_C_Grimm': vals[5], 'S3_C_TSI': vals[6],
#                         'S4_P': vals[7], 'S4_C': vals[8]
#                     })
#                 except: break
#         df = pd.DataFrame(records)

#     if df.empty:
#         print("❌ No data found.")
#         return

#     df = df.sort_values('Timestamp_ms')
#     df['Time_s'] = (df['Timestamp_ms'] - df['Timestamp_ms'].iloc[0]) / 1000.0
#     base_name = os.path.splitext(file_path)[0]
    
#     # helper to save and print
#     def finalize_plot(filename, title, y_label):
#         plt.title(title, fontsize=14, fontweight='bold')
#         plt.xlabel(CFG["x_label"])
#         plt.ylabel(y_label)
#         plt.grid(True, alpha=0.3)
#         plt.legend(loc='upper right')
#         plt.tight_layout()
#         save_path = f"{base_name}_{filename}.png"
#         plt.savefig(save_path, dpi=300)
#         print(f"✅ Saved: {save_path}")
#         plt.close()

#     # --- PART 1: OVERALL COMPARISON PLOTS ---

#     # Plot 1: Comparison - All Particles
#     plt.figure(figsize=CFG["fig_size"])
#     for i in range(1, 4):
#         plt.plot(df['Time_s'], df[f'S{i}_P'], label=CFG["legends"][f"S{i}"], linewidth=CFG["line_width"])
#     finalize_plot("Comparison_Particles", "Global Comparison: Particle Counts", CFG["y_label_p"])

#     # Plot 2: Comparison - All Concentrations
#     plt.figure(figsize=CFG["fig_size"])
#     plt.plot(df['Time_s'], df['S1_C'], label=CFG["legends"]["S1"])
#     plt.plot(df['Time_s'], df['S2_C'], label=CFG["legends"]["S2"])
#     plt.plot(df['Time_s'], df['S3_C_Grimm'], label=f'{CFG["legends"]["S3"]} (Grimm)')
#     plt.plot(df['Time_s'], df['S3_C_TSI'], label=f'{CFG["legends"]["S3"]} (TSI)', linestyle='--')
#     # plt.plot(df['Time_s'], df['S4_C'], label=CFG["legends"]["S4"])
#     finalize_plot("Comparison_Concentrations", "Global Comparison: PM Concentrations", CFG["y_label_c"])

#     # --- PART 2: INDIVIDUAL SENSOR PLOTS (SEPARATED) ---

#     sensors = [
#         {"id": "S1", "name": "SPS30", "P": "S1_P", "C": ["S1_C"]},
#         {"id": "S2", "name": "PMSA", "P": "S2_P", "C": ["S2_C"]},
#         {"id": "S3", "name": "PM2012", "P": "S3_P", "C": ["S3_C_Grimm", "S3_C_TSI"]},
#         {"id": "S4", "name": "PM2016", "P": "S4_P", "C": ["S4_C"]}
#     ]

#     for s in sensors:
#         # Generate Particles Plot
#         plt.figure(figsize=CFG["fig_size"])
#         plt.plot(df['Time_s'], df[s["P"]], color='tab:blue', label=f"{s['name']} Particles")
#         finalize_plot(f"{s['name']}_Particles", f"{s['name']}: Particle Data", CFG["y_label_p"])

#         # Generate Concentration Plot
#         plt.figure(figsize=CFG["fig_size"])
#         for i, c_col in enumerate(s["C"]):
#             label = c_col.split('_')[-1] if len(s["C"]) > 1 else "Conc"
#             style = '-' if i == 0 else '--'
#             plt.plot(df['Time_s'], df[c_col], color='tab:red', linestyle=style, label=f"{s['name']} {label}")
#         finalize_plot(f"{s['name']}_Concentration", f"{s['name']}: Concentration Data", CFG["y_label_c"])

# if __name__ == "__main__":
#     if len(sys.argv) < 2:
#         print("Usage: python3 plotSensorData.py <data.csv/bin>")
#     else:
#         plot_data(sys.argv[1])