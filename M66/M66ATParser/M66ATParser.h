/* M66Interface Example
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

#ifndef M66ATPARSER_H
#define M66ATPARSER_H

#include <stdint.h>
#include <features/netsocket/nsapi_types.h>
#include "ATParser.h"

/** M66 AT Parser Interface class.
    This is an interface to a M66 modem.
 */
class M66ATParser
{
public:
    M66ATParser(PinName tx, PinName rx, PinName rst, PinName pwr, bool debug=false);

    /**
    * Startup the M66
    *
    * @return true only if M66 was started correctly
    */
    bool startup(void);

    /**
    * Reset M66
    *
    * @return true only if M66 resets successfully
    */
    bool reset(void);

    /**
    * Connect M66 to the network
    *
    * @param apn the address of the network APN
    * @param userName the user name
    * @param passPhrase the password
    * @return true only if M66 is connected successfully
    */
    bool connect(const char *apn, const char *userName, const char *passPhrase);

    /**
    * Disconnect M66 from AP
    *
    * @return true only if M66 is disconnected successfully
    */
    bool disconnect(void);

    /**
     * Get the IP address of M66
     *
     * @return null-teriminated IP address or null if no IP address is assigned
     */
    const char *getIPAddress(void);

    /**
    * Get the MAC address of M66
    *
    * TODO: check if we need it, or to change this method
    * @return null-terminated MAC address or null if no MAC address is assigned
    */
    const char *getMACAddress(void);

     /** Get the local gateway
     *
    * TODO: check if we need it, or to change this method
     *  @return         Null-terminated representation of the local gateway
     *                  or null if no network mask has been recieved
     */
    const char *getGateway();

    /** Get the local network mask
     *
    * TODO: check if we need it, or to change this method
     *  @return         Null-terminated representation of the local network mask
     *                  or null if no network mask has been recieved
     */
    const char *getNetmask();

    /**
    * Check if M66 is connected
    *
    * @return true only if the chip has an IP address
    */
    bool isConnected(void);

    /**
    * Open a socketed connection
    *
    * @param type the type of socket to open "UDP" or "TCP"
    * @param id id to give the new socket, valid 0-4
    * @param port port to open connection with
    * @param addr the IP address of the destination
    * @return true only if socket opened successfully
    */
    bool open(const char *type, int id, const char* addr, int port);

    /**
    * Sends data to an open socket
    *
    * @param id id of socket to send to
    * @param data data to be sent
    * @param amount amount of data to be sent - max 1024
    * @return true only if data sent successfully
    */
    bool send(int id, const void *data, uint32_t amount);

    /**
    * Receives data from an open socket
    *
    * @param id id to receive from
    * @param data placeholder for returned information
    * @param amount number of bytes to be received
    * @return the number of bytes received
    */
    int32_t recv(int id, void *data, uint32_t amount);

    /**
    * Closes a socket
    *
    * @param id id of socket to close, valid only 0-4
    * @return true only if socket is closed successfully
    */
    bool close(int id);

    /**
    * Allows timeout to be changed between commands
    *
    * @param timeout_ms timeout of the connection
    */
    void setTimeout(uint32_t timeout_ms);

    /**
    * Checks if data is available
    */
    bool readable();

    /**
    * Checks if data can be written
    */
    bool writeable();

    /**
    * Attach a function to call whenever network state has changed
    *
    * @param func A pointer to a void function, or 0 to set as none
    */
    void attach(Callback<void()> func);

    /**
    * Attach a function to call whenever network state has changed
    *
    * @param obj pointer to the object to call the member function on
    * @param method pointer to the member function to call
    */
    template <typename T, typename M>
    void attach(T *obj, M method) {
        attach(Callback<void()>(obj, method));
    }

private:
    BufferedSerial _serial;
    ATParser _parser;

    DigitalOut _powerPin;
    DigitalOut _resetPin;
    struct packet {
        struct packet *next;
        int id;
        uint32_t len;
        // data follows
    } *_packets, **_packets_end;
    void _packet_handler();

    char _ip_buffer[16];
    char _gateway_buffer[16];
    char _netmask_buffer[16];
    char _mac_buffer[18];
};

#endif
