#include "mbed.h"
#include "BME280.h"
#include "WS2812b.h"
#include "fsl_port.h"

#define BME280_DEVICE_ADDRESS 0x77  //!< BME280 device address (same address for multiple devices)
#define BME280_CHIP_ID        0x60  //!< BME280 chip id is fixed

DigitalOut led1(LED1);
BME280 env_sensor(I2C_SDA, I2C_SCL);

ubirch::WS2812b rgbLED;

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

unsigned int RED[1] = {0x00ff00};
unsigned int GREEN[1] = {0xff0000};
unsigned int BLUE[1] = {0x0000ff};

void rgb_thread(void const *args) {
    int counter = 0;
    while (true) {
        switch (++counter % 3) {
            case 0:
                rgbLED.show(RED, 1);
                break;
            case 1:
                rgbLED.show(GREEN, 1);
                break;
            case 2:
                rgbLED.show(BLUE, 1);
                break;
            default:
                break;
        }

        Thread::wait(1000);
    }
}

osThreadDef(led_thread, osPriorityNormal, DEFAULT_STACK_SIZE);
osThreadDef(bme_thread, osPriorityNormal, DEFAULT_STACK_SIZE);
osThreadDef(rgb_thread, osPriorityNormal, DEFAULT_STACK_SIZE);

// main() runs in its own thread in the OS
// (note the calls to Thread::wait below for delays)
int main() {
    // enable external pin to output LED data signal
    CLOCK_EnableClock(kCLOCK_PortA);
    PORT_SetPinMux(PORTA, 14U, kPORT_MuxAlt5);

//    printf("THREADS!\r\n");
//    osThreadCreate(osThread(led_thread), NULL);
//    osThreadCreate(osThread(bme_thread), NULL);
//    osThreadCreate(osThread(rgb_thread), NULL);
    int counter = 0;

    rgbLED.show({0}, 1);
    Thread::wait(2000);

    while (true) {
        switch (++counter % 3) {
            case 0:
                rgbLED.show(RED, 1);
                break;
            case 1:
                rgbLED.show(GREEN, 1);
                break;
            case 2:
                rgbLED.show(BLUE, 1);
                break;
            default:
                break;
        }
        led1 = !led1;
        Thread::wait(2000);
    }
}
