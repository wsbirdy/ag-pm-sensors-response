#include <Arduino.h>
#include "OpenAirMultiSense.h"

// #define DEBUG_OUT_ENABLED

#ifndef DEBUG_OUT_ENABLED
#define FLASH_MEM
#endif

#define CUBIC_SERIAL_PORT  Serial1
#define SPS30_SERIAL_PORT Serial0
#define DEBUG_OUT Serial
#define DEBUG_OUT_BAUD 115200
#define NO_ERROR 0
#define READ_INTERVAL   1000    // [ms]
#define BOOT_TIME       10000   // [ms]
#define TOTAL_SCREEN    3

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SensirionUartSps30 sps30_sensor;
Adafruit_PM25AQI pmsa_sensor = Adafruit_PM25AQI();
PM2008_I2C pm2016_i2c;
Cubic_PMsensor_UART pm2012_uart(CUBIC_SERIAL_PORT);
SensorPayload sensorPayload;

static char errorMessage[64];
static int16_t error;
unsigned long buttonPressTime = 0;
bool isPressing = false;
bool longPressTriggered = false;
bool loggingActive = false;
uint32_t loop_delay = 0;
uint32_t button_cnt = 0;
String log_name = "./sensor_logs";
const size_t MIN_FREE_SPACE = 200000; // 200KB

// 'volatile' is required for variables used inside interrupts
volatile unsigned long pressStartTime = 0;
volatile unsigned long button_duration = 0;
volatile bool buttonReleased = false;
volatile bool buttonPressed = false;

void readStoredLogs();
void startLogRawStream(String new_log, const SensorPayload& payload);
void stopLogRawStream();
void startLogging(bool enable);
void changeScreen(uint32_t* currentScreen);
void systemDisplay(uint8_t currentScreen);
void displayLoggingStatus();
void displayPmValue();
void displayFilesSystem();
void handleButton();
int getFileList();
void ensureSpace();
String startNewLogFile(String log_name);
void deleteSpecificFile(const char* filename) ;
void deleteAllFiles();


// Interrupt Service Routine (ISR) - must be in RAM for speed
void IRAM_ATTR handleButtonInterrupt() {
    int currentState = digitalRead(BUTTON_PIN);
    
    if (currentState == LOW) { // Button Pressed
        pressStartTime = millis();
        buttonPressed = true;
    } else { // Button Released
        button_duration = millis() - pressStartTime;
        buttonReleased = true;
    }
}

void setup() {
    // Wait for serial monitor to open
    DEBUG_OUT.begin(DEBUG_OUT_BAUD);
    // This is the most important part for the ESP32-C3!
    unsigned long startWait = millis();
    while (!DEBUG_OUT && (millis() - startWait < 5000)) {
        delay(100); 
    }

    pinMode(BUTTON_PIN,INPUT);
    pinMode(WATCHDOG_DONE_PIN,OUTPUT);
    pinMode(PMSA003I_SET_PIN, OUTPUT);
    pinMode(PMSA003I_RESET_PIN, OUTPUT);
    pinMode(LED_PIN,OUTPUT);
    digitalWrite(PMSA003I_SET_PIN, HIGH);   // Set to HIGH to enable I2C mode
    digitalWrite(PMSA003I_RESET_PIN, HIGH); // Keep the sensor out of reset
    digitalWrite(WATCHDOG_DONE_PIN,LOW);
    digitalWrite(LED_PIN,LOW);
    delay(100);

    // DEBUG_OUT.print("Adafruit PMSA003I Air Quality Sensor: ");
    if (!Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN)) {
        Serial.println("Failed to initialize I2C bus!");
        while (1);
    }

    DEBUG_OUT.println("OpenAir Multi-Sensor Testing....");
    // Initialize the display
    if(!display.begin(i2c_Address, true)) {
        Serial.println(F("SH110X allocation failed"));
        for(;;);
    }
    display.clearDisplay();
    display.setContrast(128);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println("OpenAir Multi-Sensor Testing....");
    display.display(); // You MUST call this to actually show data

    // Trigger on CHANGE (both press and release)
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonInterrupt, CHANGE);

    SPS30_SERIAL_PORT.begin(DEBUG_OUT_BAUD);
    if(SPS30_SERIAL_PORT){
        DEBUG_OUT.println("Serial0 initialized successfully.");
    } else {
        DEBUG_OUT.println("Serial0 initialization failed!");
    }
    
    int8_t serialNumber[32] = {0};
    int8_t productType[9] = {0};
    sps30_sensor.begin(SPS30_SERIAL_PORT);
    sps30_sensor.stopMeasurement();
    error |= sps30_sensor.readSerialNumber(serialNumber, 32); delay(100);
    error |= sps30_sensor.readProductType(productType, 9); delay(100);
    error |= sps30_sensor.startMeasurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_UINT16); delay(100);
    if (error != NO_ERROR) {
        DEBUG_OUT.print("Error trying to execute sps30 sensor: ");
        errorToString(error, errorMessage, sizeof errorMessage);
        DEBUG_OUT.println(errorMessage);
    }else{
        DEBUG_OUT.print("SPS30 serialNumber: ");
        DEBUG_OUT.print((const char*)serialNumber);
        DEBUG_OUT.println();
        DEBUG_OUT.print("SPS30 productType: ");
        DEBUG_OUT.print((const char*)productType);
        DEBUG_OUT.println();
    }

    if (!pmsa_sensor.begin_I2C()) {      // connect to the sensor over I2C
        DEBUG_OUT.println("Could not find PM 2.5 sensor!");
    }else{
        DEBUG_OUT.println("PMSA003I found!");
    }
    pm2016_i2c.command();

    CUBIC_SERIAL_PORT.begin(9600, SERIAL_8N1, UART2_RX, UART2_TX);
    startWait = millis();
    while (!CUBIC_SERIAL_PORT && (millis() - startWait < 5000)) {
        delay(100); 
    }
    Serial.println("Cubic PM UART sensor initialize.");

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
    }
    #ifdef DEBUG_OUT_ENABLED
    readStoredLogs();
    // deleteAllFiles();
    // deleteSpecificFile("/sensor_logs.bin");
    #endif

    // DEBUG_OUT.println("Initialization complete. Wait for 30 seconds for sensors to stabilize...");
    delay(BOOT_TIME); // Wait for sensors to stabilize
    // DEBUG_OUT.println("Starting measurements now.");
}

void loop() {
    
    delay(READ_INTERVAL - loop_delay); // 1 second delay between readings
    sensorPayload.counter++;
    sensorPayload.timestamp = millis();
#ifdef DEBUG_OUT_ENABLED
    DEBUG_OUT.printf("Reading - #%d \n", sensorPayload.counter);
    DEBUG_OUT.printf("Timestamp: %d ms\n", sensorPayload.timestamp);
#endif

    //Reset Watchdog
    digitalWrite(WATCHDOG_DONE_PIN,HIGH);
    delay(1);
    digitalWrite(WATCHDOG_DONE_PIN,LOW);

    uint32_t time_taken[4];     //[SPS30,003i,PM2012,PM2016]
    uint32_t tStart_measure;    // Timestamp before start measurement each sensors;

    // Reading PMSA003i Measurement
    tStart_measure = micros();
    PM25_AQI_Data data;
    if (!pmsa_sensor.read(&data)) {
        DEBUG_OUT.println("Could not read from PMSA003");
        sensorPayload.pmsa003iData.particles = -1;
        sensorPayload.pmsa003iData.concentration = -1;
    }else{
        sensorPayload.pmsa003iData.particles = data.particles_03um;
        sensorPayload.pmsa003iData.concentration = data.pm25_env;
    }
    time_taken[PMSA003I] = micros() - tStart_measure;

    // Reading PM2012 Measurement
    tStart_measure = micros();
    PMData s3_data;
    bool s3_ret = pm2012_uart.readMeasurement(s3_data);
    if(s3_ret == true) {
        sensorPayload.cubicPm2012.particles = s3_data.count_0_3;
        sensorPayload.cubicPm2012.concentration = s3_data.pm2_5_grimm;
        sensorPayload.cubicPm2012Tsi = s3_data.pm2_5_tsi;
    }else{
        Serial.println("Could not read from PM2012");
        sensorPayload.cubicPm2012.particles = -1;
        sensorPayload.cubicPm2012.concentration = -1;
        sensorPayload.cubicPm2012Tsi = -1;
    }
    time_taken[PM2012] = micros() - tStart_measure;


    // Reading PM2016 Measurement
    tStart_measure = micros();
    uint8_t ret = pm2016_i2c.read();
    if (ret == 0) {
        sensorPayload.cubicPm2016.particles = pm2016_i2c.number_of_0p3_um;
        sensorPayload.cubicPm2016.concentration = pm2016_i2c.pm2p5_grimm;

    }else{
        Serial.println("Could not read from PM2016");
        sensorPayload.cubicPm2016.particles = -1;
        sensorPayload.cubicPm2016.concentration = -1;
    }
    time_taken[PM2016] = micros() - tStart_measure;


    // Reading SPS30 Measurement
    tStart_measure = micros();
    SensirionMeasurement sps30_measurement;
    error = sps30_sensor.readMeasurementValuesUint16(sps30_measurement.mc1p0, sps30_measurement.mc2p5, sps30_measurement.mc4p0, sps30_measurement.mc10p0,
                                                    sps30_measurement.nc0p5, sps30_measurement.nc1p0, sps30_measurement.nc2p5, sps30_measurement.nc4p0,
                                                    sps30_measurement.nc10p0, sps30_measurement.typicalParticleSize);
    if (error != NO_ERROR) {
        DEBUG_OUT.print("Could not read from SPS30");
        sensorPayload.sps30Data.particles = -1;
        sensorPayload.sps30Data.concentration = -1;
    }else{
        sensorPayload.sps30Data.particles = sps30_measurement.nc0p5;
        sensorPayload.sps30Data.concentration = sps30_measurement.mc2p5;
    }
    time_taken[SPS30] = micros() - tStart_measure;

    handleButton();
    systemDisplay(button_cnt);

#ifdef DEBUG_OUT_ENABLED
    // Print out the data for debugging
    DEBUG_OUT.println("SPS30 Data:");
    DEBUG_OUT.printf(" - Polling Time: %d us\n", time_taken[SPS30]);
    DEBUG_OUT.printf(" - Particles >0.5um: %d \n", sensorPayload.sps30Data.particles);
    DEBUG_OUT.printf(" - Concentration PM2.5: %d µg/m3\n", sensorPayload.sps30Data.concentration);

    DEBUG_OUT.println("PMSA003I Data:");
    DEBUG_OUT.printf(" - Polling Time: %d us\n", time_taken[PMSA003I]);
    DEBUG_OUT.printf(" - Particles >0.3um: %d \n", sensorPayload.pmsa003iData.particles);
    DEBUG_OUT.printf(" - Concentration PM2.5: %d µg/m3\n", sensorPayload.pmsa003iData.concentration);

    DEBUG_OUT.println("Cubic PM2012 Data:");
    DEBUG_OUT.printf(" - Polling Time: %d us\n", time_taken[PM2012]);
    DEBUG_OUT.printf(" - Particles >0.3um: %d \n", sensorPayload.cubicPm2012.particles);
    DEBUG_OUT.printf(" - Concentration PM2.5 [GRIMM]: %d µg/m3\n", sensorPayload.cubicPm2012.concentration);
    DEBUG_OUT.printf(" - Concentration PM2.5 [TSI]: %d µg/m3\n", sensorPayload.cubicPm2012Tsi);

    DEBUG_OUT.println("Cubic PM2016 Data:");
    DEBUG_OUT.printf(" - Polling Time: %d us\n", time_taken[PM2016]);
    DEBUG_OUT.printf(" - Particles >0.3um: %d \n", sensorPayload.cubicPm2016.particles);
    DEBUG_OUT.printf(" - Concentration PM2.5: %d µg/m3\n", sensorPayload.cubicPm2016.concentration);
    // DEBUG_OUT.printf("sensorPayload raw data: ");
    // for (int i = 0; i < sizeof(sensorPayload); i++) {
    //     DEBUG_OUT.printf("%02x ", ((uint8_t*)&sensorPayload)[i]);
    // }
    loop_delay = millis()-sensorPayload.timestamp;
    DEBUG_OUT.printf("\nTotal time (including screen & button): %d ms\n\n", loop_delay);

#else
    // Send the raw binary structure over UART
    // Serial.write((uint8_t*)&sensorPayload, sizeof(sensorPayload));
    #ifdef FLASH_MEM
    if(loggingActive){
        Serial.println("Action: Starting Data Log...");        
        startLogRawStream(log_name, sensorPayload);
    }
    #endif

#endif

}


/**
 * Logs the entire SensorPayload struct to Flash.
 * @param payload: The populated SensorPayload object.
 */
void startLogRawStream(String new_log, const SensorPayload& payload) {

    // new_log = new_log + ".bin";

    // Open the file in APPEND mode
    File file = LittleFS.open(new_log, FILE_APPEND);
    
    if (!file) {
        Serial.println("[-] Error: Could not open file for writing.");
        return;
    }

    // Add this inside logRawStream
    if (file.size() > MIN_FREE_SPACE) { // If file > 200kB
        Serial.println("[!] Log file full.");
        file.close();
        return;
    }else{
        // Cast the struct to a byte pointer (uint8_t*) and write its total size
        size_t written = file.write((const uint8_t*)&payload, sizeof(SensorPayload));
        if (written != sizeof(SensorPayload)) {
            Serial.printf("[-] Write Error! Expected %d bytes, wrote %d\n", sizeof(SensorPayload), written);
        } else {
            Serial.printf("[+] Logged Payload #%d (%d bytes)\n", payload.counter, written);
        }
    }

    // Close to ensure data is committed to the physical flash
    file.close();
}

void stopLogRawStream() {
    File logFile;
    if (logFile) {
        String finalName = logFile.name();
        size_t finalSize = logFile.size();
        logFile.flush(); // Force write any remaining data in the buffer
        logFile.close();
        Serial.printf("Closed: %s | Total Size: %d bytes\n", finalName.c_str(), finalSize);
    }
}

void readStoredLogs() {

    File root = LittleFS.open("/");
    File file = root.openNextFile();

    if (!file) {
        Serial.println("No files found in storage.");
        return;
    }
    
    Serial.println("--- START OF FLASH LOGS ---");

    while (file) {
        Serial.println("-----------------------------------------");
        Serial.print("READING FILE: ");
        Serial.println(file.name());
        Serial.println("-----------------------------------------");

        // Temporary struct to hold each entry as we read it
        SensorPayload entry;

        // Read until there are no more full packets left
        while (file.available() >= sizeof(SensorPayload)) {
            size_t bytesRead = file.read((uint8_t*)&entry, sizeof(SensorPayload));

            if (bytesRead == sizeof(SensorPayload)) {
                // Check headers/terminators to ensure data integrity
                if (entry.header[0] == 0x4f && entry.header[1] == 0x41 &&
                    entry.terminater[0] == 0xaa && entry.terminater[1] == 0xbb) {
                    
                #ifdef DEBUG_OUT_ENABLED
                    Serial.printf("Count: %d | Time: %d ms\n", entry.counter, entry.timestamp);
                    Serial.printf("  SPS30   : particles=%d concentration=%d\n", entry.sps30Data.particles, entry.sps30Data.concentration);
                    Serial.printf("  PMSA003i: particles=%d concentration=%d\n", entry.pmsa003iData.particles, entry.pmsa003iData.concentration);
                    Serial.printf("  PM2012A : particles=%d GRIMM_conc=%d, TSI_conc=%d\n", entry.cubicPm2012.particles, entry.cubicPm2012.concentration, entry.cubicPm2012Tsi);
                    Serial.printf("  PM2016  : particles=%d concentration=%d\n", entry.cubicPm2016.particles, entry.cubicPm2016.concentration);
                #endif
                } else {
                    Serial.println("[!] Data corruption detected: Magic bytes don't match.");
                }
            }
        }

        Serial.println("\n--- END OF FILE ---\n");

        file.close();            // Always close before opening the next
        file = root.openNextFile(); // Move to the next file in the system
    }

    // // Open the file for reading
    // File file = LittleFS.open("/pmLogs.bin", FILE_READ);    
    // if (!file) {
    //     Serial.println("[-] No log file found to read.");
    //     return;
    // }
    // file.close();
    Serial.println("--- END OF FLASH LOGS ---");

}

void ensureSpace() {
    //Loop not break
    while ((LittleFS.totalBytes() - LittleFS.usedBytes()) < MIN_FREE_SPACE) {
        // Serial.printf("Free Disk space: %d\n", (LittleFS.totalBytes() - LittleFS.usedBytes()));
        File root = LittleFS.open("/");
        File file = root.openNextFile();
        
        if (!file) break; // No files left to delete

        String oldestFile = file.name();
        file.close();

        Serial.print("Low space! Deleting oldest: ");
        Serial.println(oldestFile);
        LittleFS.remove(oldestFile);
        delay(100);
    }
}

String startNewLogFile(String log_name) {

    // Find the next available ID
    int fileID = 0;
    while (LittleFS.exists(log_name + String(fileID) + ".bin")) {   // "/log_"
        fileID++;
    }

    String fileName = log_name + String(fileID) + ".bin";
    // File file = LittleFS.open(fileName, FILE_WRITE);
    Serial.println("New log created: " + fileName);
    return fileName;

    // if (file) {
        // Write CSV Header
        // file.println("Timestamp,PM2.5,PM10,Uptime_ms");
        // file.close();
        // Serial.println("New log created: " + fileName);
        // return fileName;
    // }
    // return "";
}

void handleButton() {

    // --- 1. Continuous Long Press Check (Real-time 5s trigger) ---
    if (buttonPressed && digitalRead(BUTTON_PIN) == LOW) {
        if (millis() - pressStartTime >= 5000) {
            digitalWrite(LED_PIN,HIGH);
            delay(100);
            digitalWrite(LED_PIN,LOW);
            delay(100);
            digitalWrite(LED_PIN,HIGH);
            delay(100);
            digitalWrite(LED_PIN,LOW);
            delay(100);
            loggingActive = !loggingActive;     // Enable/Disable logging function
            buttonPressed = false;              // Reset to prevent double trigger
            Serial.printf(">>> LOGGING = %s<<<\n", loggingActive ? "START":"STOP");

            // check flash space -> auto circular file (delete oldest file)
            ensureSpace();
            // create file name
            log_name = startNewLogFile("/pmLogs");
        
            // startLogging(loggingActive); // Trigger Long Press Action
        }
    }

    // --- 2. Short Press Check (On Release) ---
    if (buttonReleased) {
        // Serial.printf("Press duration = %d\n", button_duration);
        // Only trigger if it wasn't already caught by the long-press logic
        if (// button_duration > 50 && 
            button_duration < 1000) {
            Serial.println(">>> CHANGE SCREEN <<<");
            changeScreen(&button_cnt);
        } 
        
        // If we were logging and released, we can stop or just reset flags
        if (loggingActive && button_duration >= 5000) {
            Serial.println("Logging session confirmed.");
        }
        buttonPressed = false;
        buttonReleased = false;
    }

}

void startLogging(bool enable){

    Serial.println("Test Logging");
    #ifdef FLASH_MEM
    // if(enable){
    //     Serial.println("Action: Starting Data Log...");
    //     startLogRawStream(sensorPayload);
    // }else{
    //     Serial.println("Action: Stopping Data Log...");
    //     //stop the log
    // }
    #endif
}

void changeScreen(uint32_t* currentScreen){
    *currentScreen = (*currentScreen + 1) % TOTAL_SCREEN;
    Serial.printf("Button Counter = %d\n", button_cnt);

}

void systemDisplay(uint8_t currentScreen){

    switch(currentScreen)
    {
    case 0:
        displayPmValue();
        break;

    case 1:
        displayLoggingStatus();
        break;

    case 2:
        displayFilesSystem();
        break;

    default:

        break;

    }

}

void displayPmValue(){
    display.clearDisplay(); // Always clear the buffer before drawing new data

    // 1. Header
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.printf("Time: %4ds Meas:%4d", sensorPayload.timestamp/1000, sensorPayload.counter);

    // 2. Horizontal Divider
    display.drawLine(0, 10, 128, 10, SH110X_WHITE);

    // 3. PM Readings (Larger text for visibility)
    display.setTextSize(1);
    display.setCursor(0, 17);  //  x: 128/4-6-6, y: 64/2
    display.printf("SPS30  : %3d, %4d\n", sensorPayload.sps30Data.concentration, sensorPayload.sps30Data.particles);
    display.printf("003i   : %3d, %4d\n", sensorPayload.pmsa003iData.concentration, sensorPayload.pmsa003iData.particles);
    display.printf("Cubic_S: %3d, %4d\n", sensorPayload.cubicPm2012.concentration, sensorPayload.cubicPm2012.particles);
    display.printf("Cubic_L: %3d, %4d\n", sensorPayload.cubicPm2016.concentration, sensorPayload.cubicPm2016.particles);
    // display.setCursor(12, 25);  //  x: 128/4-6-6, y: 64/2 
    // display.setCursor(84, 17);  //  x: 128*3/4-6-6, y: 64/2
    // display.setCursor(76, 25);  //  x: 128/4-6-6, y: 64/2 

    // 4. Horizontal Divider
    display.drawLine(0, 52, 128, 52, SH110X_WHITE);

    // 5. Unit
    display.setTextSize(1);
    display.setCursor(0, 55);
    if(loggingActive){
        display.print("R");
    }
    display.setCursor(30, 55);
    display.print("ug/m3,Particles");

    // 6. Push to hardware
    display.display();

}

void displayLoggingStatus(){

    size_t total = LittleFS.totalBytes();
    size_t used = LittleFS.usedBytes();
    float usagePercent = ((float)used / (float)total) * 100.0;

#ifdef DEBUG_OUT_ENABLE
    Serial.printf("Total Storage: %d bytes\n", total);
    Serial.printf("Used Storage:  %d bytes\n", used);
    Serial.printf("Usage:         %.2f%%\n", usagePercent);
#endif
    display.clearDisplay(); // Always clear the buffer before drawing new data

    // 1. Push Text
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println("-- Logging Status --\n");
    display.drawLine(0,12,128,12,SH110X_WHITE);
    display.printf("Total: %8d B\n", total);
    display.printf("Used : %8d B\n", used);
    display.printf("Free : %8d B\n", total-used);
    display.printf("Usage: %8.2f %%\n", usagePercent);
    display.printf("Total Files: %d \n", getFileList());
    // 2. Push to hardware
    display.display();
    
}

void displayFilesSystem(){

    display.clearDisplay(); // Always clear the buffer before drawing new data

    // 1. Set Header
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println("-- Files System --\n");
    display.drawLine(0,12,128,12,SH110X_WHITE);
    display.setCursor(0, 15);

    std::vector<String> fileList;
    File root = LittleFS.open("/");
    File file = root.openNextFile();

    // 1. Collect all filenames
    while (file) {
        fileList.push_back(String(file.name()));
        file = root.openNextFile();
    }

    // 2. Sort names Descending (Natural Numerical Sort: 20, 19, ... 2, 1)
    std::sort(fileList.begin(), fileList.end(), [](String a, String b) {
        // Function to extract the number from a filename like "pmLogs12.bin"
        auto extractNum = [](String s) {
            String numStr = "";
            for (int i = 0; i < s.length(); i++) {
                if (isDigit(s[i])) numStr += s[i];
            }
            return numStr.length() > 0 ? numStr.toInt() : 0;
        };

        int numA = extractNum(a);
        int numB = extractNum(b);

        if (numA != numB) {
            return numA > numB; // Higher number first (20 > 19)
        }
        return a > b; // Fallback to alphabetical if numbers are the same
    });

    // 3. Display the sorted list
    int count = 0;
    for (const String& name : fileList) {
        if (count >= 6) break; // Limit to 6 files to fit on screen
        
        File f = LittleFS.open("/" + name, "r");
        #ifdef DEBUG_OUT_ENABLED
        // Print file details
        Serial.print("File: ");
        Serial.print(file.name());
        Serial.print("\tSize: ");
        Serial.print(file.size());
        Serial.println(" bytes");
        #endif
        display.print(name);
        display.print(" ");
        display.print(f.size());
        display.println(" B");
        f.close();
        count++;
    }

    if (fileList.empty()) {
        display.println("No files found.");
    }
    #ifdef DEBUG_OUT_ENABLES
    Serial.println("-------------------------");
    Serial.print("Total number of files: ");
    Serial.println(fileCount);
    Serial.println("-------------------------\n");
    #endif

    // 2. Push to hardware
    display.display();
    
}

int getFileList() {
    int fileCount = 0;
    File root = LittleFS.open("/");
    File file = root.openNextFile();

    while (file) {
        fileCount++;

        #ifdef DEBUG_OUT_ENABLED
        // Print file details
        Serial.print("File: ");
        Serial.print(file.name());
        Serial.print("\tSize: ");
        Serial.print(file.size());
        Serial.println(" bytes");
        #endif

        file = root.openNextFile();
    }
    #ifdef DEBUG_OUT_ENABLES
    Serial.println("-------------------------");
    Serial.print("Total number of files: ");
    Serial.println(fileCount);
    Serial.println("-------------------------\n");
    #endif

    return fileCount;
}

void deleteSpecificFile(const char* filename) {
    // Usage:
    // deleteSpecificFile("/log_0.csv");
    if (LittleFS.exists(filename)) {
        if (LittleFS.remove(filename)) {
            Serial.print("Successfully deleted: ");
            Serial.println(filename);
        } else {
            Serial.println("Delete failed!");
        }
    } else {
        Serial.println("File does not exist.");
    }
}

void deleteAllFiles() {

    if (LittleFS.format()) {
        Serial.println("LittleFS formatted successfully");
    } else {
        Serial.println("LittleFS format failed");
    }

    // File root = LittleFS.open("/");
    // File file = root.openNextFile();

    // Serial.println("Start deleting all files...");

    // while (file) {
    //     // 1. Get the name
    //     String fileName = "/" + String(file.name()); // Ensure full path with "/"
        
    //     // 2. Close the current file handle immediately
    //     file.close(); 
        
    //     // 3. Delete the file
    //     if (LittleFS.remove(fileName)) {
    //         Serial.print("Deleted: ");
    //         Serial.println(fileName);
    //     } else {
    //         Serial.print("Failed to delete: ");
    //         Serial.println(fileName);
    //     }
        
    //     // 4. Move to the actual NEXT file without resetting root
    //     file = root.openNextFile();
    // }

    // Serial.println("All files cleared.");

}

