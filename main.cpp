#include "mbed.h"

DigitalOut led1(LED1);

void led_thread(void const *args) {
    while (true) {
        printf("BLINK\r\n");
        led1 = !led1;
        Thread::wait(1000);
    }
}

osThreadDef(led_thread, osPriorityNormal, DEFAULT_STACK_SIZE);
 
// main() runs in its own thread in the OS
// (note the calls to Thread::wait below for delays)
int main() {
    printf("THREADS!\r\n");
    osThreadCreate(osThread(led_thread), NULL);
 
    while (true) {
        led1 = !led1;
        Thread::wait(200);
    }
}
