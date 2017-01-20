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

#include "M66ATParser.h"

M66ATParser::M66ATParser(PinName tx, PinName rx, PinName rst, PinName pwr, bool debug)
    : _serial(tx, rx, 1024), _parser(_serial)
    , _packets(0), _packets_end(&_packets)
    , _resetPin(rst), _powerPin(pwr)
{
    // TODO make baud rate configurable for Modem code
    _serial.baud(115200);
    _parser.debugOn(debug);
}

bool M66ATParser::startup(void)
{
    _powerPin = 1;

    printf("M66.startup()\r\n");
    bool success = reset()
        && _parser.send("AT+QIMUX=1")
        && _parser.recv("OK")
        && _parser.send("AT+QINDI=2")
        && _parser.recv("OK");

    // TODO identify the URC for data available
    _parser.oob("+RECEIVE", this, &M66ATParser::_packet_handler);

    return success;
}

bool M66ATParser::reset(void)
{
    printf("M66.reset()\r\n");
    // switch off the modem
    _resetPin = 1;
    wait_ms(200);
    _resetPin = 0;
    wait_ms(1000);
    _resetPin = 1;

    bool modemOn = false;
    for(int tries = 0; !modemOn && tries < 2; tries++) {
        printf("M66.reset(%d)\r\n", tries);
        // switch on modem
        _resetPin = 1;
        wait_ms(200);
        _resetPin = 0;
        wait_ms(1000);
        _resetPin = 1;

        // TODO check if need delay here to wait for boot

        for (int i = 0; !modemOn && i < 2; i++) {
            modemOn = _parser.send("AT") && _parser.recv("OK");
        }
    }
    printf("M66.reset(modemOn=%d)\r\n", modemOn);

    if(modemOn) {
        // TODO check if the parser ignores any lines it doesn't expect
        _parser.send("ATE0");
        modemOn = _parser.send("AT+QIURC=1") && _parser.recv("OK");
    }

    printf("M66.reset(modemOn=%d)\r\n", modemOn);
    return modemOn;
}

bool M66ATParser::connect(const char *apn, const char *userName, const char *passPhrase)
{
    printf("M66.connect()\r\n");
    // TODO implement setting the pin number, add it to the contructor arguments
    bool connected = false, attached = false;
    for(int tries = 0; !connected && !attached && tries < 3; tries++) {
        // TODO implement timeout
        // connecte to the mobile network
        for (int networkTries = 0; !connected && networkTries < 20; networkTries++) {
            int bearer = -1, status = -1;
            if (_parser.send("AT+CREG?") && _parser.recv("+CREG: %d,%d", &bearer, &status)) {
                // TODO add an enum of status codes
                connected = status == 1 || status == 5;
            }
            // TODO check if we need to use thread wait
            wait_ms(1000);
        }
        if(!connected) continue;

        // attach GPRS
        _parser.send("AT+QIDEACT");
        for(int attachTries = 0; !attached && attachTries < 20; attachTries++) {
            attached = _parser.send("AT+CGATT=1") && _parser.recv("OK");
        }
        if(!attached) continue;

        // set APN and finish setup
        attached =
                _parser.send("AT+QIFGCNT=0") && _parser.recv("OK") &&
                _parser.send("AT+QICSGP=1,\"%s\",\"%s\",\"%s\"", apn, userName, passPhrase) && _parser.recv("OK") &&
                _parser.send("AT+QIREGAPP") && _parser.recv("OK") &&
                _parser.send("AT+QIACT") && _parser.recv("OK");
    }

    return connected && attached;
}

bool M66ATParser::disconnect(void)
{
    return _parser.send("AT+QIDEACT") && _parser.recv("OK");
}

const char *M66ATParser::getIPAddress(void)
{
    if (!(_parser.send("AT+QILOCIP") && _parser.recv("%s", _ip_buffer))) {
        return 0;
    }

    return _ip_buffer;
}

// TODO is getMACAddress necessary?
//const char *M66ATParser::getMACAddress(void)
//{
//    if (!(_parser.send("AT+CIFSR")
//        && _parser.recv("+CIFSR:STAMAC,\"%17[^\"]\"", _mac_buffer)
//        && _parser.recv("OK"))) {
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

    if (!(_parser.send("AT+QINDI=1") && _parser.recv("OK"))) return false;

    if (!(_parser.send("AT+QIDNSIP=1") && _parser.recv("OK"))) return false;

    return _parser.send("AT+QIOPEN=%d,\"%s\",\"%s\",\"%d\"", id, type, addr, port)
        && _parser.recv("OK") && _parser.recv("1, CONNECT OK");
}

bool M66ATParser::send(int id, const void *data, uint32_t amount)
{
    int id = 0, sc = 0, sid = 0;

    // TODO if this retry is required?
    //May take a second try if device is busy
    for (unsigned i = 0; i < 2; i++) {
        if (_parser.send("AT+QISEND=%d,%d\r\n", id, amount)
            && _parser.write((char*)data, (int)amount) >= 0
            && _parser.recv("SEND OK")
            && _parser.recv("+QIRDI: %d,%d,%d", id, sc, sid)){

            if (_parser.send("AT+QIRD=%d,%d,%d,325", id, sc, sid)) return true;
        }
    }
    return false;
}

void M66ATParser::_packet_handler()
{
    int id;
    uint32_t amount;

    // parse out the packet
    if (!_parser.recv(" %d,%d", &id, &amount)) {
        return;
    }

    struct packet *packet = (struct packet*)malloc(
            sizeof(struct packet) + amount);
    if (!packet) {
        return;
    }

    packet->id = id;
    packet->len = amount;
    packet->next = 0;

    if (!(_parser.read((char*)(packet + 1), amount))) {
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
        if (!_parser.recv("OK")) {
            return -1;
        }
    }
}

bool M66ATParser::close(int id)
{
    // TODO check if this retry is required
    //May take a second try if device is busy
    for (unsigned i = 0; i < 2; i++) {
        if (_parser.send("AT+QICLOSE=%d", id)
            && _parser.recv("CLOSE OK")) {
            return true;
        }
    }

    return false;
}

void M66ATParser::setTimeout(uint32_t timeout_ms)
{
    _parser.setTimeout(timeout_ms);
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