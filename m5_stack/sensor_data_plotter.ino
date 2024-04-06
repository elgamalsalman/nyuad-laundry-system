#include <M5Core2.h>

// ----- STRUCTS -----
struct Vector {
  float x = 0, y = 0, z = 0;
  float get_magnitude() {
    return sqrt(x * x + y * y + z * z);
  }
};

// ----- MAIN -----
Vector gyro, acc, tilt;
float temp = 0.0F;

void setup() {
  // init M5Stack and IMU sensor unit
  M5.begin();
  M5.IMU.Init();

  // init LCD screen
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(GREEN, BLACK);
  M5.Lcd.setTextSize(2);

  // start serial communication
  Serial.begin(9600);
}

void loop() {
  // collect gyro, accelerometer, tilt and temperature data
  M5.IMU.getGyroData(&gyro.x, &gyro.y, &gyro.z);
  M5.IMU.getAccelData(&acc.x, &acc.y, &acc.z);
  M5.IMU.getAhrsData(&tilt.x, &tilt.y, &tilt.z);
  M5.IMU.getTempData(&temp);

  // display data on LCD
  M5.Lcd.setCursor(0, 20);
  M5.Lcd.printf("gyro.x,  gyro.y, gyro.z");
  M5.Lcd.setCursor(0, 42);
  M5.Lcd.printf("%6.2f %6.2f%6.2f o/s", gyro.x, gyro.y, gyro.z);

  M5.Lcd.setCursor(0, 70);
  M5.Lcd.printf("acc.x,   acc.y,  acc.z");
  M5.Lcd.setCursor(0, 92);
  M5.Lcd.printf("%5.2f  %5.2f  %5.2f G", acc.x, acc.y, acc.z);

  M5.Lcd.setCursor(0, 120);
  M5.Lcd.printf("tilt.x,  tilt.y,  tilt.z");
  M5.Lcd.setCursor(0, 142);
  M5.Lcd.printf("%5.2f  %5.2f  %5.2f deg", tilt.x, tilt.y, tilt.z);

  M5.Lcd.setCursor(0, 175);
  M5.Lcd.printf("Temperature : %.2f C", temp);

  // plot sensor magnitude data in serial plotter
  Serial.print("gyro:");
  Serial.print(gyro.get_magnitude());
  Serial.print("\t");
  Serial.print("acc:");
  Serial.print(acc.get_magnitude());
  Serial.print("\t");
  Serial.print("tilt:");
  Serial.print(tilt.get_magnitude());
  Serial.print("\t");
  Serial.print("temp:");
  Serial.println(temp);

  delay(10);
}
