/* ESP8266 Example
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cctype>
#include "mbed_debug.h"
#include "M66ATParser.h"

#define CIODEBUG(...) debug_if(true, __VA_ARGS__)
#define CSTDEBUG(...) debug_if(true, __VA_ARGS__)

M66ATParser::M66ATParser(PinName txPin, PinName rxPin, PinName rstPin, PinName pwrPin, bool debug)
    : _serial(txPin, rxPin, 1024)
    , _packets(0), _packets_end(&_packets)
    , _resetPin(rstPin), _powerPin(pwrPin)
{
    // TODO make baud rate configurable for Modem code
    _serial.baud(115200);
}

//TODO chec if we need to make the QIMUS configurable by passing a variable
bool M66ATParser::startup(void)
{
    _powerPin = 1;

    CSTDEBUG("M66.startup()\r\n");
    bool success = reset()
                   && tx("AT+QIMUX=1") && rx("OK");

    // TODO identify the URC for data available
    //_parser.oob("+RECEIVE: ", this, &M66ATParser::_packet_handler);

return success;
}

bool M66ATParser::reset(void)
{
    char response[4];

    CSTDEBUG("M66.reset()\r\n");
    // switch off the modem
    _resetPin = 1;
    wait_ms(200);
    _resetPin = 0;
    wait_ms(1000);
    _resetPin = 1;

    bool modemOn = false;
    for(int tries = 0; !modemOn && tries < 3; tries++) {
        CSTDEBUG("M66.reset(%d)\r\n", tries);
        // switch on modem
        _resetPin = 1;
        wait_ms(200);
        _resetPin = 0;
        wait_ms(1000);
        _resetPin = 1;

        // TODO check if need delay here to wait for boot
        for (int i = 0; !modemOn && i < 2; i++) {
            modemOn = tx("AT") && scan("%2s", &response)
                      && (!strncmp("AT", response, 2) || !strncmp("OK", response, 2));
        }
    }
    CSTDEBUG("M66.reset(modemOn=%d)\r\n", modemOn);

    if(modemOn) {
        // TODO check if the parser ignores any lines it doesn't expect
        modemOn = tx("ATE0") && scan("%3s", response)
                  &&  (!strncmp("ATE0", response, 3) || !strncmp("OK", response, 2))
                  && tx("AT+QIURC=1") && rx("OK");
    }

    CSTDEBUG("M66.reset(modemOn=%d)\r\n", modemOn);
    return modemOn;
}

bool M66ATParser::connect(const char *apn, const char *userName, const char *passPhrase)
{
    CSTDEBUG("M66.connect()\r\n");
    // TODO implement setting the pin number, add it to the contructor arguments
    bool connected = false, attached = false;
    for(int tries = 0; !connected && !attached && tries < 3; tries++) {
        // TODO implement timeout
        // connecte to the mobile network
        for (int networkTries = 0; !connected && networkTries < 20; networkTries++) {
            int bearer = -1, status = -1;
            if (tx("AT+CREG?") && scan("+CREG: %d,%d", &bearer, &status)) {
                // TODO add an enum of status codes
                connected = status == 1 || status == 5;
            }
            // TODO check if we need to use thread wait
            wait_ms(1000);
        }
        if(!connected) continue;

        // attach GPRS
        tx("AT+QIDEACT");
        for(int attachTries = 0; !attached && attachTries < 20; attachTries++) {
            attached = tx("AT+CGATT=1") && rx("OK");
        }
        if(!attached) continue;

        // set APN and finish setup
        attached =
                tx("AT+QIFGCNT=0") && rx("OK") &&
                tx("AT+QICSGP=1,\"%s\",\"%s\",\"%s\"", apn, userName, passPhrase) && rx("OK") &&
                tx("AT+QIREGAPP") && rx("OK") &&
                tx("AT+QIACT") && rx("OK");
    }

    return connected && attached;
}

bool M66ATParser::disconnect(void)
{
    return tx("AT+QIDEACT") && rx("OK");
}

const char *M66ATParser::getIPAddress(void)
{
    if (!(tx("AT+QILOCIP") && scan("%s", _ip_buffer))) {
        return 0;
    }

    return _ip_buffer;
}

// TODO is getMACAddress necessary?
//const char *M66ATParser::getMACAddress(void)
//{
//    if (!(tx("AT+CIFSR")
//        && rx("+CIFSR:STAMAC,\"%17[^\"]\"", _mac_buffer)
//        && rx("OK"))) {
//        return 0;
//    }
//
//    return _mac_buffer;
//}

bool M66ATParser::isConnected(void)
{
    return getIPAddress() != 0;
}

bool M66ATParser::open(const char *type, int id, const char* addr, int port)
{
    //IDs only 1-6
    if(id > 6) {
        return false;
    }

//    if (!(tx("AT+QINDI=1") && rx("OK"))) return false;

    if (!(tx("AT+QIDNSIP=1") && rx("OK"))) return false;

    return tx("AT+QIOPEN=%d,\"%s\",\"%s\",\"%d\"", id, type, addr, port)
        && rx("OK") && rx("1, CONNECT OK");
}

bool M66ATParser::send(int id, const void *data, uint32_t amount)
{
    tx("AT+QISRVC=1");
    rx("OK");

    // TODO if this retry is required?
    //May take a second try if device is busy
    for (unsigned i = 0; i < 2; i++) {
        if (tx("AT+QISEND=%d,%d", id, amount)
            && _serial.write((char*)data, (int)amount) >= 0
            && rx("SEND OK")) {
            return true;
        }
    }
    return false;
}

void M66ATParser::_packet_handler(const char *response)
{
    int id;
    int amount;

    CSTDEBUG("M66.packet handler: '%s'\r\n", response);

    // parse out the packet
    if (sscanf(response, "+RECEIVE: %d, %d", &id, &amount) != 2) {
        return;
    }
    CSTDEBUG("M66 receive (%d), %d bytes\r\n", id, amount);

    struct packet *packet = (struct packet*)malloc(
            sizeof(struct packet) + amount);
    if (!packet) {
        return;
    }

    packet->id = id;
    packet->len = (uint32_t) amount;
    packet->next = 0;

    const size_t bytesRead = read((char *)(packet + 1), (size_t) amount);
    if(bytesRead != amount) {
        CSTDEBUG("M66 data receive failed: %d != %d\r\n", bytesRead, amount);
        free(packet);
        return;
    }

    // append to packet list
    *_packets_end = packet;
    _packets_end = &packet->next;
}

int32_t M66ATParser::recv(int id, void *data, uint32_t amount)
{
    while (true) {
        // check if any packets are ready for us
        for (struct packet **p = &_packets; *p; p = &(*p)->next) {
            if ((*p)->id == id) {
                struct packet *q = *p;

                if (q->len <= amount) { // Return and remove full packet
                    memcpy(data, q+1, q->len);

                    if (_packets_end == &(*p)->next) {
                        _packets_end = p;
                    }
                    *p = (*p)->next;

                    uint32_t len = q->len;
                    free(q);
                    return len;
                } else { // return only partial packet
                    memcpy(data, q+1, amount);

                    q->len -= amount;
                    memmove(q+1, (uint8_t*)(q+1) + amount, q->len);

                    return amount;
                }
            }
        }

        // Wait for inbound packet
        // TODO check what happens when we receive a packet (like OK)
        // TODO the response code may be different if connection is still open
        // TODO it may need to be moved to packet handler
        int receivedId;
        if (!scan("%d, CLOSED", &receivedId) && id == receivedId) {
            return -1;
        }
    }
}

bool M66ATParser::close(int id)
{
    // TODO check if this retry is required
    //May take a second try if device is busy
    for (unsigned i = 0; i < 2; i++) {
        if (tx("AT+QICLOSE=%d", id)
            && rx("CLOSE OK")) {
            return true;
        }
    }

    return false;
}

void M66ATParser::setTimeout(uint32_t timeout_ms)
{
    _timeout = timeout_ms;
}

bool M66ATParser::readable()
{
    return _serial.readable();
}

bool M66ATParser::writeable()
{
    return _serial.writeable();
}

void M66ATParser::attach(Callback<void()> func)
{
    _serial.attach(func);
}

bool M66ATParser::tx(const char *pattern, ...) {
    char cmd[512];

    while (readline(cmd, sizeof(cmd))) {
        CIODEBUG("GSM (%02d) -> '%s'\r\n", strlen(cmd), cmd);
        checkURC(cmd);
    }

    // cleanup the input buffer and check for URC messages
    cmd[0] = '\0';

    va_list ap;
    va_start(ap, pattern);
    vsnprintf(cmd, 512, pattern, ap);
    va_end(ap);

    CIODEBUG("GSM (%02d) <- '%s'\r\n", strlen(cmd), cmd);
    _serial.puts(cmd);
    _serial.puts("\r\n");

    return true;
}

int M66ATParser::scan(const char *pattern, ...) {
    char response[512];
    do {
        readline(response, 512 - 1);
    } while (checkURC(response) != -1);

    va_list ap;
    va_start(ap, pattern);
    int matched = vsscanf(response, pattern, ap);
    va_end(ap);

    CIODEBUG("GSM (%02d) -> '%s' (%d)\r\n", strlen(response), response, matched);
    return matched;
}

bool M66ATParser::rx(const char *pattern) {
    char response[512];
    size_t length = 0, patternLength = strnlen(pattern, sizeof(response));
    do {
        length = readline(response, 512 - 1);
        if(!length) return false;

        CIODEBUG("GSM (%02d) -> '%s'\r\n", strlen(response), response);
    } while (checkURC(response) != -1);

    return strncmp(pattern, (const char *) response, MIN(length, patternLength)) == 0;
}

int M66ATParser::checkURC(const char *response) {
    if(!strncmp("+RECEIVE:", response, 9)) {
        _packet_handler(response);
        return 0;
    }
    return -1;
}

size_t M66ATParser::read(char *buffer, size_t max) {
    Timer timer;
    timer.start();

    size_t idx = 0;
    while (idx < max && timer.read() < 5) {
        if(!_serial.readable()) {
            __WFI();
            continue;
        }

        if (max - idx) buffer[idx++] = _serial.getc();
    }

    return idx;

}

size_t M66ATParser::readline(char *buffer, size_t max) {
    Timer timer;
    timer.start();

    size_t idx = 0;
    while (idx < max && timer.read() < 5) {

        if (!_serial.readable()) {
            // nothing in the buffer, allow some sleep
//            __WFI();
            wait(0.05f);
            continue;
        }

        int c = _serial.getc();

        if (c == '\r') continue;
        if (c == '\n') {
            if (!idx) {
                idx = 0;
                continue;
            }
            break;
        }
        if (max - idx && isprint(c)) buffer[idx++] = (char) c;
    }


    buffer[idx] = 0;
    return idx;
}
