#include <SPI.h>
#include <LoRa.h>
// ----------------------------
// LoRa Pins
// ----------------------------
#define SS      5
#define RST     14
#define DIO0    2

void setup()
{
    Serial.begin(115200);

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
    // ----------------------------------------

    Serial.println("--------------------------------");
    Serial.println("LoRa Telemetry Receiver");
    Serial.println("Frequency : 433 MHz");
    Serial.println("SF        : 12");
    Serial.println("Bandwidth : 125 kHz");
    Serial.println("CodingRate: 4/5");
    Serial.println("Preamble  : 8");
    Serial.println("--------------------------------");
}

void loop()
{
    int packetSize = LoRa.parsePacket();
    if (packetSize <= 0)
        return;
    uint8_t frame[256];
    int index = 0;
    while (LoRa.available())
    {
        frame[index++] = LoRa.read();
    }
    int16_t rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();
    // -------------------------
    // Send packet to MATLAB
    // -------------------------
    Serial.write((uint8_t)0xAA);      // Start Marker
    Serial.write((uint8_t)index);     // CCSDS Frame Length
    Serial.write(frame, index);       // Complete CCSDS Frame
    Serial.write((uint8_t*)&rssi, sizeof(rssi));
    Serial.write((uint8_t*)&snr, sizeof(snr));
    Serial.write((uint8_t)0x55);      // End Marker
}
