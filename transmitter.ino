#include <Wire.h>
#include <LoRa.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_ADXL345_U.h>
// ----------------------------
// LoRa Pins
// ----------------------------
#define SS    5
#define RST   14
#define DIO0  2
// ----------------------------
// Sensors
// ----------------------------
Adafruit_BMP280 bmp;
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
#define TMP102_ADDR 0x48
uint16_t packetCount = 0;
// ----------------------------------------------------
// CCSDS CRC16 (Polynomial 0x1021)
// ----------------------------------------------------
uint16_t crc16_ccsds(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}
// ----------------------------------------------------
// Read TMP102 Temperature
// ----------------------------------------------------
float readTMP102()
{
    Wire.beginTransmission(TMP102_ADDR);
    Wire.write(0x00);
    Wire.endTransmission();
    Wire.requestFrom(TMP102_ADDR, 2);
    if (Wire.available() == 2)
    {
        byte MSB = Wire.read();
        byte LSB = Wire.read();
        int16_t tempRaw = ((MSB << 8) | LSB) >> 4;
        return tempRaw * 0.0625;
    }
    return NAN;
}
// ----------------------------------------------------
// Setup
// ----------------------------------------------------
void setup()
{
    Serial.begin(115200);
    Wire.begin();

// LoRa Initialization
LoRa.setPins(SS, RST, DIO0);

if (!LoRa.begin(433E6))
{
    Serial.println("LoRa initialization failed!");
    while (1);
}

// -------- LoRa PHY Configuration --------
LoRa.setSpreadingFactor(12);      // SF12
LoRa.setSignalBandwidth(125E3);   // 125 kHz
LoRa.setCodingRate4(5);           // Coding Rate 4/5
LoRa.setPreambleLength(8);        // 8-symbol preamble
LoRa.setTxPower(17);              // 17 dBm
// ----------------------------------------

Serial.println("--------------------------------");
Serial.println("LoRa Telemetry Transmitter");
Serial.println("CCSDS Packet Format Enabled");
Serial.println("LoRa Settings:");
Serial.println("Frequency : 433 MHz");
Serial.println("SF        : 12");
Serial.println("Bandwidth : 125 kHz");
Serial.println("CodingRate: 4/5");
Serial.println("Preamble  : 8");
Serial.println("TX Power  : 17 dBm");
Serial.println("--------------------------------");
// BMP280
    if (!bmp.begin(0x76))
    {
        Serial.println("BMP280 not found!");
        while (1);
    }
    // ADXL345
    if (!accel.begin())
    {
        Serial.println("ADXL345 not found!");
        while (1);
    }
    accel.setRange(ADXL345_RANGE_16_G);
    Serial.println("Sensors Initialized.");
    Serial.println();
}
// ----------------------------------------------------
// Main Loop
// ----------------------------------------------------
void loop()
{
    // ----------------------------
    // Read Sensors
    // ----------------------------
    float bmpPressure = bmp.readPressure() / 100.0F;
    float tmp102Temp = readTMP102();
    sensors_event_t event;
    accel.getEvent(&event);
    float ax = event.acceleration.x;
    float ay = event.acceleration.y;
    float az = event.acceleration.z;
    // ----------------------------
    // Create Telemetry Payload
    // ----------------------------
    String dataStr =
        "BMP_P=" + String(bmpPressure, 2) +
        ",TMP102_T=" + String(tmp102Temp, 2) +
        ",AX=" + String(ax, 2) +
        ",AY=" + String(ay, 2) +
        ",AZ=" + String(az, 2);
    // ----------------------------
    // CCSDS Packet
    // ----------------------------
    uint8_t frame[256];
    uint16_t index = 0;
    // Primary Header
    frame[index++] = 0x08;               // Version, Type, Secondary Header Flag
    frame[index++] = 0x01;               // APID
    frame[index++] = (packetCount >> 8) & 0xFF;
    frame[index++] = packetCount & 0xFF;
    frame[index++] = ((dataStr.length() - 1) >> 8) & 0xFF;
    frame[index++] = (dataStr.length() - 1) & 0xFF;
    // Payload
    memcpy(&frame[index], dataStr.c_str(), dataStr.length());
    index += dataStr.length();
    // CRC
    uint16_t crc = crc16_ccsds(frame, index);
    frame[index++] = (crc >> 8) & 0xFF;
    frame[index++] = crc & 0xFF;
    // ----------------------------
    // Transmit Packet
    // ----------------------------
    LoRa.beginPacket();
    LoRa.write(frame, index);
    LoRa.endPacket();
    // ----------------------------
    // Debug Output
    // ----------------------------
    Serial.println("------------------------------------------------");
    Serial.print("Packet Count : ");
    Serial.println(packetCount);
    Serial.print("TX Frame (HEX): ");
    for (uint16_t i = 0; i < index; i++)
    {
        if (frame[i] < 16)
            Serial.print("0");
        Serial.print(frame[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    Serial.print("Payload : ");
    Serial.println(dataStr);
    Serial.println("------------------------------------------------");
    Serial.println();
    packetCount++;
    delay(2000);
}
