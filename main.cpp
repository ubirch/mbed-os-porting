#include "mbed.h"
#include "BME280.h"
//#include "M66/M66ATParser/M66ATParser.h"
#include "M66Interface.h"

DigitalOut led1(LED1);

BME280       env_sensor(I2C_SDA, I2C_SCL);
M66Interface modem(GSM_UART_TX, GSM_UART_RX, GSM_PWRKEY, GSM_POWER, true);

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

void http_demo(NetworkInterface *net)
{
    TCPSocket socket;

    printf("Sending HTTP request to api.ubirch.com...\r\n");

    // Open a socket on the network interface, and create a TCP connection to www.arm.com
    socket.open(net);

    socket.connect("api.ubirch.com", 80);

    // Send a simple http request
    char sbuffer[] = "GET / HTTP/1.1\r\n\r\n";

    int scount = socket.send(sbuffer, sizeof sbuffer);
    printf("sent %d [%.*s]\r\n", scount, strstr(sbuffer, "\r\n")-sbuffer, sbuffer);

    // Recieve a simple http response and print out the response line
    char rbuffer[64];
    int rcount = socket.recv(rbuffer, sizeof rbuffer);
    printf("recv %d [%.*s]\r\n", rcount, strstr(rbuffer, "\r\n")-rbuffer, rbuffer);

    // Close the socket to return its memory and bring down the network interface
    socket.close();
}

void modem_thread(void const *args) {

    printf("M66 example\r\n\r\n");

    printf("\r\nConnecting...\r\n");
    int ret = modem.connect("eseye.com", "ubirch", "internet");
    if (ret != 0) {
        printf("\r\nConnection error\r\n");
    }
    else {
        printf("Success\r\n\r\n");
        printf("IP: %s\r\n", modem.get_ip_address());

        http_demo(&modem);

        modem.disconnect();

        printf("\r\nDone\r\n");
    }
}

osThreadDef(led_thread,   osPriorityNormal, DEFAULT_STACK_SIZE);
//osThreadDef(bme_thread,   osPriorityNormal, DEFAULT_STACK_SIZE);
//osThreadDef(modem_thread, osPriorityNormal, DEFAULT_STACK_SIZE);

// main() runs in its own thread in the OS
// (note the calls to Thread::wait below for delays)
int main() {

    printf("THREADS!\r\n");

    osThreadCreate(osThread(led_thread), NULL);
//    osThreadCreate(osThread(bme_thread), NULL);
//    osThreadCreate(osThread(modem_thread), NULL);

    printf("M66 example\r\n\r\n");

    printf("\r\nConnecting...\r\n");
    int ret = modem.connect("eseye.com", "ubirch", "internet");
    if (ret != 0) {
        printf("\r\nConnection error\r\n");
    }
    else {
        printf("Success\r\n\r\n");
        printf("IP: %s\r\n", modem.get_ip_address());

        http_demo(&modem);

        modem.disconnect();

        printf("\r\nDone\r\n");
    }

    while (true) {
      led1 = !led1;
      Thread::wait(200);
    }
}
