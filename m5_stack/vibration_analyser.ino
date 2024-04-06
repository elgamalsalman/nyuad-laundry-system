#include <M5Core2.h>

// ----- CONSTANTS -----
#define INTERVAL_SIZE 100
#define ACC_MEAN 1.0F
#define ACC_SCALAR 100.0F

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

float acc_squareddevs_sum = 0.0F, acc_stddev = 0.0F;
float acc_squareddevs[INTERVAL_SIZE];
int acc_index = 0;

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

	// keeping track of the deviations squared for the last INTERVAL_SIZE datapoints
	acc_squareddevs_sum -= acc_squareddevs[acc_index];
	float curr_dev = ACC_SCALAR * (ACC_MEAN - acc.get_magnitude());
	acc_squareddevs[acc_index] = curr_dev * curr_dev;
	acc_squareddevs_sum += acc_squareddevs[acc_index];
	acc_index = (acc_index + 1) % INTERVAL_SIZE;

	// calculating the acceleration magnitude standard deviation
	acc_stddev = sqrt(acc_squareddevs_sum / INTERVAL_SIZE);

	// plot sensor magnitude data in serial plotter
	Serial.print("acc:");
	Serial.print(acc.get_magnitude());
	Serial.print("\t");
	Serial.print("acc_squareddevs_sum:");
	Serial.print(acc_squareddevs_sum);
	Serial.print("\t");
	Serial.print("acc_stddev:");
	Serial.println(acc_stddev);

  // display acc_stddev
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.printf("acc_stddev: %5.2f", acc_stddev);

  delay(10);
}
