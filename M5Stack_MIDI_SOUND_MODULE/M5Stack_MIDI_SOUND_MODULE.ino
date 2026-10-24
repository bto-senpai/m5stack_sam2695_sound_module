//TARGET COMPONENTS[M5Stack CoreS3 SE with SAM2695 MODULE connected through Grove CABLE(Port.A)]

#include <M5Unified.h>
#include <M5UnitSynth.h>    // ADD MANUALLY
#include <USB.h>
#include <USBMIDI.h>

// SETTING FOR SAM2695(MIDI STANDARD)
#define MIDI_BAUDRATE 31520

//SETTING FOR PORT.A
#define RX_PIN 1  // PORT.A RX (G2)
#define TX_PIN 2  // PORT.A TX (G1)

USBMIDI MIDI;

// FUNCTION TO SEND MIDI MESSAGE TOWARD SAM2695
void sendMIDI(uint8_t status, uint8_t data1, uint8_t data2 = 0xFF) {
  Serial2.write(status);
  Serial2.write(data1);
  if (data2 != 0xFF) {
    Serial2.write(data2);
  }
}

// INITIALIZE SAM2695
void initSAM2695() {
  // 1. GM SYSTEM ON
  Serial2.write(0xF0); // SysEx Start
  Serial2.write(0x7E); // All Call
  Serial2.write(0x7F); // Device ID
  Serial2.write(0x09); // General MIDI
  Serial2.write(0x01); // On
  Serial2.write(0xF7); // SysEx End
  delay(100);          // WAIT

  uint8_t Master_Volume[] = {0xB0, 0x63, 0x37, 0xB0, 0x62, 0x07, 0xB0, 0x06, 0x40};
  Serial2.write(Master_Volume,  sizeof(Master_Volume));
  //SAMPLE TO SEND RAW MASTER VOLUME MIDI MESSAGE
  //Serial2.write(0xB0); Serial2.write(0x63); Serial2.write(0x37); // ① NRPN MSB (37h)
  //Serial2.write(0xB0); Serial2.write(0x62); Serial2.write(0x07); // ② NRPN LSB (07h)
  //Serial2.write(0xB0); Serial2.write(0x06); Serial2.write(0x40); // ③ Data Entry 0x40=64(0-127)
  delay(100);

  // DEFAULT SETTING FOR EACH MIDI CHANNELS
  for (uint8_t ch = 0; ch < 16; ch++) {
    sendMIDI(0xB0 | ch, 7, 100);  // VOLUME(CC#7)
    sendMIDI(0xB0 | ch, 10, 64);  // PAN(CC#10)
    sendMIDI(0xB0 | ch, 91, 40);  // REVERB(CC#91)
    sendMIDI(0xB0 | ch, 93, 0);   // CHORUS(CC#93)

    if (ch != 9) {
      Serial2.write(0xC0 | ch);
      Serial2.write(0); // 0: Acoustic Grand Piano
    }
  }
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  // INITIALIZE PORT.A(G1, G2)
  Serial2.begin(MIDI_BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
  delay(500); // WAIT

  // INITIALIZE SAM2695
  initSAM2695();

  // LCD DISPLAY
  M5.Display.setTextSize(2);
  M5.Display.println("USB-MIDI Sound Module");
  M5.Display.println("CoreS3-SE with SAM2695");
  M5.Display.printf("PORT.A: TX=%d, RX=%d\n", TX_PIN, RX_PIN);

  // START USB-MIDI
  MIDI.begin();
  USB.begin();
}

void loop() {
M5.update();

  // OFFITIAL ESP32 STYLE
  midiEventPacket_t packet;

  // CHECK USB-MIDI PACKET
  while (MIDI.readPacket(&packet)) {
    // IDENTIFY THE MESSAGE WITH THE LOWER 4BITS CODE FROM packet.header(CIN: Code Index Number)
    uint8_t cin = packet.header & 0x0F;

    // DIVIDE THE CODE DEPENDING ON THE MESSAGE TYPE(CIN)
    switch (cin) {
      // 1BYTE MESSAGE(MIDI CLOCK, ACTIVE SENSING, ETC)
      case 0x05: // SINGLE BYTE
      case 0x0F: // SINGLE BYTE
        Serial2.write(packet.byte1);
        break;

      // 2BYTES MESSAGE(PROGRAM CHANGE, CHANNEL AFTER TOUCH, ETC)
      case 0x0C: // Program Change
      case 0x0D: // Channel Pressure
        Serial2.write(packet.byte1);
        Serial2.write(packet.byte2);
        break;

      // 3BYTES MESSAGE(NOTE ON, NOTE OFF, CONTROL CHANGE, PITCH BEND, ETC)
      case 0x08: // Note Off
      case 0x09: // Note On
      case 0x0A: // Poly Key Pressure
      case 0x0B: // Control Change
      case 0x0E: // Pitch Bend Change
        Serial2.write(packet.byte1);
        Serial2.write(packet.byte2);
        Serial2.write(packet.byte3);
        break;

      // PARSE SysEx DATA PACKET
      case 0x04: // SysEx 3 bytes
        Serial2.write(packet.byte1);
        Serial2.write(packet.byte2);
        Serial2.write(packet.byte3);
        break;
      case 0x06: // SysEx 2 bytes
        Serial2.write(packet.byte1);
        Serial2.write(packet.byte2);
        break;
      case 0x07: // SysEx 1 byte
        Serial2.write(packet.byte1);
        break;

      default:
        // SKIP UNEXPECTED PACKET
        break;
    }
  }

  // WAIT
  delayMicroseconds(100);
}
