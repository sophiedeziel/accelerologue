#include<Wire.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>

const int buttonPin = 2;
const int LEDPin = 4;

const int MPU_addr = 0x68;
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

String filename;
char numstr[4];
bool recording = false;
bool lastState = false;
long last_toggle;
int filenumber = 0;

void setup() {
  Serial.begin(115200);
  pinMode(MOSI, INPUT_PULLUP);
  pinMode(buttonPin, INPUT);
  pinMode(LEDPin, OUTPUT);

  Wire.begin();

  Wire.beginTransmission(MPU_addr);

  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  Wire.beginTransmission(MPU_addr);
  Wire.write(0x1C);  // setting sensitivity
  Wire.write(0b00011000);
  Wire.endTransmission(false);

  //sanity check sensitivity register
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x1C);
  Wire.endTransmission(true);

  Serial.println("Trying card");

  // see if the card is present and can be initialized:
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
  Serial.println("card initialized.");
  do {
    incrementFileName();
  } while (SD.exists(filename));
}

void loop() {
  String dataString = "";

  if ((millis() - last_toggle > 200) && digitalRead(buttonPin) == HIGH && lastState == false) {
    last_toggle = millis();
    toggleRecord();
    lastState = true;
  }

  if (digitalRead(buttonPin) == LOW) {
    lastState = false;
  }

  if (recording) {
    getAccData();

    File dataFile = SD.open(filename, FILE_APPEND);

    // if the file is available, write to it:
    if (dataFile) {
      dataString += millis();
      dataString += ",";
      dataString += AcX;
      dataString += ",";
      dataString += AcY;
      dataString += ",";
      dataString += AcZ;
      dataString += ",";
      dataString += Tmp;
      dataString += ",";
      dataString += GyX;
      dataString += ",";
      dataString += GyY;
      dataString += ",";
      dataString += GyZ;
      dataFile.println(dataString);
      dataFile.close();
    }
    // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening " + filename);
    }
  }
}

void incrementFileName() {
  snprintf(numstr, 4, "%03d", filenumber);
  filename = "/log" + String(numstr) + ".csv";
  filenumber++;
}

void getAccData() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);

  Wire.requestFrom(MPU_addr, 14, true); // request a total of 14 registers
  AcX = Wire.read() << 8 | Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
  AcY = Wire.read() << 8 | Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ = Wire.read() << 8 | Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp = Wire.read() << 8 | Wire.read(); // 0x41 (TEMP_OUT_H)   & 0x42 (TEMP_OUT_L)
  GyX = Wire.read() << 8 | Wire.read(); // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY = Wire.read() << 8 | Wire.read(); // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ = Wire.read() << 8 | Wire.read(); // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
}

void toggleRecord() {
  recording = !recording;
  if (recording) {
    Serial.println("Saving to " + filename);
  } else {
    Serial.println("Done.");
    incrementFileName();
  }
  digitalWrite(LEDPin, recording);
}
