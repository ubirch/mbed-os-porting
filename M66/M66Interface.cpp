/* ESP8266 implementation of NetworkInterfaceAPI
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

#include <string.h>
#include "M66Interface.h"

// Various timeouts for different ESP8266 operations
#define M66_CONNECT_TIMEOUT 15000
#define M66_SEND_TIMEOUT    500
#define M66_RECV_TIMEOUT    0
#define M66_MISC_TIMEOUT    500

// M66Interface implementation
M66Interface::M66Interface(PinName tx, PinName rx, PinName rstPin, PinName pwrPin, bool debug)
    : _m66(tx, rx, rstPin, pwrPin, debug)
{
    memset(_ids, 0, sizeof(_ids));
    memset(_cbs, 0, sizeof(_cbs));

    _m66.attach(this, &M66Interface::event);
}

int M66Interface::connect(const char *apn, const char *userName, const char *passPhrase)
{
    _m66.setTimeout(M66_CONNECT_TIMEOUT);

    if (!set_credentials(apn, userName, passPhrase)) {
        return NSAPI_ERROR_PARAMETER;
    }

    if (!_m66.startup()) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }

    if (!_m66.connect(apn, userName, passPhrase)) {
        return NSAPI_ERROR_NO_CONNECTION;
    }

    if (!_m66.getIPAddress()) {
        return NSAPI_ERROR_NO_ADDRESS;
    }

    return NSAPI_ERROR_OK;
}

int M66Interface::set_credentials(const char *apn, const char *userName, const char *passPhrase)
{
    memset(_apn, 0, sizeof(_apn));
    strncpy(_apn, apn, sizeof(_apn));

    memset(_userName, 0, sizeof(_userName));
    strncpy(_userName, userName, sizeof(_userName));

    memset(_passPhrase, 0, sizeof(_passPhrase));
    strncpy(_passPhrase, passPhrase, sizeof(_passPhrase));

    return 0;
}

int M66Interface::disconnect()
{
    _m66.setTimeout(M66_MISC_TIMEOUT);

    if (!_m66.disconnect()) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }

    return NSAPI_ERROR_OK;
}

const char *M66Interface::get_ip_address()
{
    return _m66.getIPAddress();
}

struct m66_socket {
    int id;
    nsapi_protocol_t proto;
    bool connected;
    SocketAddress addr;
};

int M66Interface::socket_open(void **handle, nsapi_protocol_t proto)
{
    // Look for an unused socket
    int id = -1;
 
    for (int i = 0; i < M66_SOCKET_COUNT; i++) {
        if (!_ids[i]) {
            id = i;
            _ids[i] = true;
            break;
        }
    }
 
    if (id == -1) {
        return NSAPI_ERROR_NO_SOCKET;
    }
    
    struct m66_socket *socket = new struct m66_socket;
    if (!socket) {
        return NSAPI_ERROR_NO_SOCKET;
    }
    
    socket->id = id;
    socket->proto = proto;
    socket->connected = false;
    *handle = socket;
    return 0;
}

int M66Interface::socket_close(void *handle)
{
    struct m66_socket *socket = (struct m66_socket *)handle;
    int err = 0;
    _m66.setTimeout(M66_MISC_TIMEOUT);
 
    if (!_m66.close(socket->id)) {
        err = NSAPI_ERROR_DEVICE_ERROR;
    }

    _ids[socket->id] = false;
    delete socket;
    return err;
}

int M66Interface::socket_bind(void *handle, const SocketAddress &address)
{
    return NSAPI_ERROR_UNSUPPORTED;
}

int M66Interface::socket_listen(void *handle, int backlog)
{
    return NSAPI_ERROR_UNSUPPORTED;
}

int M66Interface::socket_connect(void *handle, const SocketAddress &addr)
{
    struct m66_socket *socket = (struct m66_socket *)handle;
    _m66.setTimeout(M66_MISC_TIMEOUT);

    const char *proto = (socket->proto == NSAPI_UDP) ? "UDP" : "TCP";
    if (!_m66.open(proto, socket->id, addr.get_ip_address(), addr.get_port())) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    
    socket->connected = true;
    return 0;
}
    
int M66Interface::socket_accept(void *server, void **socket, SocketAddress *addr)
{
    return NSAPI_ERROR_UNSUPPORTED;
}

int M66Interface::socket_send(void *handle, const void *data, unsigned size)
{
    struct m66_socket *socket = (struct m66_socket *)handle;
    _m66.setTimeout(M66_SEND_TIMEOUT);
 
    if (!_m66.send(socket->id, data, size)) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
 
    return size;
}

int M66Interface::socket_recv(void *handle, void *data, unsigned size)
{
    struct m66_socket *socket = (struct m66_socket *)handle;
    _m66.setTimeout(M66_RECV_TIMEOUT);
 
    int32_t recv = _m66.recv(socket->id, data, size);
    if (recv < 0) {
        return NSAPI_ERROR_WOULD_BLOCK;
    }
 
    return recv;
}

int M66Interface::socket_sendto(void *handle, const SocketAddress &addr, const void *data, unsigned size)
{
    struct m66_socket *socket = (struct m66_socket *)handle;

    if (socket->connected && socket->addr != addr) {
        _m66.setTimeout(M66_MISC_TIMEOUT);
        if (!_m66.close(socket->id)) {
            return NSAPI_ERROR_DEVICE_ERROR;
        }
        socket->connected = false;
    }

    if (!socket->connected) {
        int err = socket_connect(socket, addr);
        if (err < 0) {
            return err;
        }
        socket->addr = addr;
    }
    
    return socket_send(socket, data, size);
}

int M66Interface::socket_recvfrom(void *handle, SocketAddress *addr, void *data, unsigned size)
{
    struct m66_socket *socket = (struct m66_socket *)handle;
    int ret = socket_recv(socket, data, size);
    if (ret >= 0 && addr) {
        *addr = socket->addr;
    }

    return ret;
}

void M66Interface::socket_attach(void *handle, void (*callback)(void *), void *data)
{
    struct m66_socket *socket = (struct m66_socket *)handle;    
    _cbs[socket->id].callback = callback;
    _cbs[socket->id].data = data;
}

void M66Interface::event() {
    for (int i = 0; i < M66_SOCKET_COUNT; i++) {
        if (_cbs[i].callback) {
            _cbs[i].callback(_cbs[i].data);
        }
    }
}
