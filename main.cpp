#include "mbed.h"
#include "BME280.h"

#define BME280_DEVICE_ADDRESS 0x77  //!< BME280 device address (same address for multiple devices)
#define BME280_CHIP_ID        0x60  //!< BME280 chip id is fixed

DigitalOut led1(LED1);
BME280     env_sensor(I2C_SDA, I2C_SCL);

void led_thread(void const *args) {
    while (true) {
        printf("BLINK\r\n");
        led1 = !led1;
        Thread::wait(1000);
    }
}

void bme_thread(void const *args) {
    while (true) {
      printf("Temprature: %2.2f C :: Pressure: %04.2f hPa :: Humidity: %2.2f%%\r\n",
         env_sensor.getTemperature(),
         env_sensor.getPressure(),
         env_sensor.getHumidity()
         );
      Thread::wait(3000);
    }
}

osThreadDef(led_thread, osPriorityNormal, DEFAULT_STACK_SIZE);
osThreadDef(bme_thread, osPriorityNormal, DEFAULT_STACK_SIZE);

// main() runs in its own thread in the OS
// (note the calls to Thread::wait below for delays)
int main() {
    printf("THREADS!\r\n");
    osThreadCreate(osThread(led_thread), NULL);
    osThreadCreate(osThread(bme_thread), NULL);

    while (true) {
      led1 = !led1;
      Thread::wait(200);
    }
}
