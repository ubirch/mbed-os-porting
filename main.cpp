#include "mbed.h"
#include "BME280.h"
#include "ESP8266Interface.h"

#define BME280_DEVICE_ADDRESS 0x77  //!< BME280 device address (same address for multiple devices)
#define BME280_CHIP_ID        0x60  //!< BME280 chip id is fixed

DigitalOut led1(LED1);
DigitalOut modem_power(GSM_POWER);

BME280     env_sensor(I2C_SDA, I2C_SCL);
ESP8266Interface modem(GSM_UART_TX, GSM_UART_RX, GSM_PWRKEY, "121", "1212", 9600);
ESP8266 modem1(GSM_UART_TX, GSM_UART_RX, GSM_PWRKEY, "111", "1212", 9600);

void led_thread(void const *args) {
    while (true) {
//        printf("BLINK\r\n");
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
      Thread::wait(5000);
    }
}

void modem_thread(void const *args) {

    modem1.reset();

    modem1.modem_register(10000);

    if (modem1.modem_gprs_attach("eseye.com", "ubirch", "internet", 10000)){

        printf("Connected\r\n");
    }
}

osThreadDef(led_thread,   osPriorityNormal, DEFAULT_STACK_SIZE);
osThreadDef(bme_thread,   osPriorityNormal, DEFAULT_STACK_SIZE);
osThreadDef(modem_thread, osPriorityNormal, DEFAULT_STACK_SIZE);

// main() runs in its own thread in the OS
// (note the calls to Thread::wait below for delays)
int main() {

    printf("THREADS!\r\n");

    osThreadCreate(osThread(led_thread), NULL);
    osThreadCreate(osThread(bme_thread), NULL);
    osThreadCreate(osThread(modem_thread), NULL);

    while (true) {
      led1 = !led1;
      Thread::wait(200);
    }
}
