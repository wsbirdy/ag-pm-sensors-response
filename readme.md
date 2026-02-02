# PM Sensor Response Time Test

The test compares the response times of 4 PM sensors from three manufacturers: **Sensirion**, **Plantower**, and **Cubic**. The goal is to analyze changes in sensor output when moving through areas with high PM density, such as traffic or street food stalls.

## Project Requirements

* **Enclosures**: Put the sensors into 3D printed enclosures.
* **Data Logging**: Log all data to one ESP32.
* **Frequency**: Save data on device with highest frequency possible, ideally every 1 second.
* **Raw Signals**: If the sensors deliver a raw signal (a particle count), then we take it along with the PM2.5 reading. This should be possible for the Sensirion and Plantower sensors; CUBIC probably doesn't provide it.
* **Environmental Data**: Temperature and humidity might also be of interest.
* **Timestamping**: Time stamp can be a consecutive ms counter.

## PM Sensors

| Model | Manufacturer |
| :--- | :--- |
| **SPS30** | Sensirion |
| **PM2012** | CUBIC |
| **PM2016** | CUBIC |
| **PMSA003I** | Plantower |

---

## 🚀 ESP32 Data Extractor & Decoder

The python script provides an automated pipeline to extract raw binary sensor data from an **ESP32-C3** flash partition (LittleFS) and decode it into readable **CSV files**.

---

## 📂 Project Structure

* **`run.command`**: The daily script to double-click for data extraction and decoding.
* **`setup.command`**: The one-time setup script to install system dependencies and Python libraries.
* **`automated_script.py`**: The coordinator script that handles folder cleanup, runs the extraction, unpacks the LittleFS image using `mklittlefs`, and initiates the final decoding.
* **`extract_memory.py`**: Communicates with the hardware via `esptool` to read flash data from offset `0x270000` with a size of `0x180000`.
* **`convert_bin_ascii.py`**: Parses binary packets (Header: 'OA', Terminator: 0xAA 0xBB) into structured CSV data.
* **`requirements.txt`**: Contains the necessary Python libraries (e.g., `esptool`, `pyserial`).

---

## 🛠️ One-Time Setup (Do this first)

Before using the tool for the first time, you must grant permission for the scripts to run on your Mac and install the environment.

### 1. Set Permissions
1.  Open your **Terminal** (Cmd + Space, type "Terminal").
2.  Type `chmod +x ` (ensure there is a **space** after the `x`).
3.  Drag the `setup.command` and `run.command` files into the Terminal window.
4.  Press **Enter**.

### 2. Run the Setup
Double-click **`setup.command`**. This script will:
* Install `mklittlefs` via Homebrew if it is missing.
* Create a local Python Virtual Environment (`venv`) to keep your system clean.
* Install required libraries like `esptool` and `pyserial`.

---

## 📊 Daily Usage

1.  **Connect your ESP32-C3** to your Mac via USB.
2.  Double-click **`run.command`**.
3.  **Port Selection**: If multiple USB serial devices are detected, the script will prompt you to choose the correct one (e.g., type `0` and hit Enter).
4.  **Completion**: Once finished, the **`decoded_results`** folder will automatically open in Finder, containing your CSV data.

---

## ⚠️ Troubleshooting

* **No Devices Found**: Ensure you are using a USB cable that supports data, not just charging.
* **mklittlefs Errors**: Ensure [Homebrew](https://brew.sh) is installed on your Mac so the setup script can download the unpacking tool.
* **Permission Denied**: If you see a security warning, go to **System Settings > Privacy & Security** and click "Open Anyway" for the script, or repeat the `chmod +x` step.

---

### Data Technical Details
* **Partition Offset**: `0x270000`
* **Partition Size**: `0x180000` (1.5MB)
* **Packet Format**: Little-endian, 30-byte packets containing timestamps and multi-sensor readings (SPS30, PMSA003I, PM2012, PM2016).