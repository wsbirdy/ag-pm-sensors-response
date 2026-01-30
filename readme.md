# PM Sensor Response Time Test

The test compares the response times of 4 PM sensors from three manufacturers: **Sensirion**, **Plantower**, and **Cubic**. The goal is to analyze changes in sensor output when moving through areas with high PM density, such as traffic or street food stalls.

## Project Requirements

* **Enclosures**: Put the sensors into 3D printed enclosures.
* **Data Logging**: Log all data to one ESP32.
* **Frequency**: Save data on device with highest frequency possible, ideally every 1 second.
* **Raw Signals**: If the sensors deliver a raw signal (a particle count), then we take it along with the PM2.5 reading. This should be possible for the Sensirion and Plantower sensors; CUBIC probably doesn't provide it.
* **Environmental Data**: Temperature and humidity might also be of interest.
* **Timestamping**: Time stamp can be a consecutive ms counter.
* **Units**: We want 2x the same unit (so 8 in total).

## PM Sensors

| Model | Manufacturer |
| :--- | :--- |
| **SPS30** | Sensirion |
| **PM2012** | CUBIC |
| **PM2016** | CUBIC |
| **PMSA003I** | Plantower |