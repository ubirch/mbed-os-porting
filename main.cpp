#include "mbed.h"
#include "BME280.h"
//#include "temp/ESP8266Interface.h"
#include "M66/M66ATParser/M66ATParser.h"

#define BME280_DEVICE_ADDRESS 0x77  //!< BME280 device address (same address for multiple devices)
#define BME280_CHIP_ID        0x60  //!< BME280 chip id is fixed

DigitalOut led1(LED1);

BME280     env_sensor(I2C_SDA, I2C_SCL);
M66ATParser modem(GSM_UART_TX, GSM_UART_RX, GSM_PWRKEY, GSM_POWER, true);

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


    if(modem.startup()) {
        if(modem.connect("eseye.com", "ubirch", "internet")) {
            if(modem.open("TCP", 1, "46.231.178.105", 80)) {
                printf("connected\r\n");
                if(!modem.send(1, "GET / HTTP/1.1\r\n\r\n", 18)) {
                    printf("send failed\r\n");
                }

                uint8_t data[128];
                int received = modem.recv(1, data, 128);

                printf("received %d bytes\r\n", received);
//                modem.close(1);
            } else {
                printf("ERROR: open()\r\n");
            }
        } else {
            printf("ERROR: connect()\r\n");
        }
    } else {
        printf("ERROR: startup()\r\n");
    }
}

//osThreadDef(led_thread,   osPriorityNormal, DEFAULT_STACK_SIZE);
//osThreadDef(bme_thread,   osPriorityNormal, DEFAULT_STACK_SIZE);
osThreadDef(modem_thread, osPriorityNormal, DEFAULT_STACK_SIZE);

// main() runs in its own thread in the OS
// (note the calls to Thread::wait below for delays)
int main() {

    printf("THREADS!\r\n");

//    osThreadCreate(osThread(led_thread), NULL);
//    osThreadCreate(osThread(bme_thread), NULL);
    osThreadCreate(osThread(modem_thread), NULL);

    while (true) {
      led1 = !led1;
      Thread::wait(200);
    }
}
