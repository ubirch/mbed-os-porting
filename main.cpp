/*******************************************************************************
 * Copyright (c) 2014, 2015 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Ian Craggs - make sure QoS2 processing works, and add device headers
 *******************************************************************************/

/**
 This is a sample program to illustrate the use of the MQTT Client library
 on the mbed platform.  The Client class requires two classes which mediate
 access to system interfaces for networking and timing.  As long as these two
 classes provide the required public programming interfaces, it does not matter
 what facilities they use underneath. In this program, they use the mbed
 system libraries.

*/

#define logMessage printf

#define MQTTCLIENT_QOS2 1

#include "M66Interface.h"
#include "M66/MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

int arrivedcount = 0;

void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    logMessage("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    logMessage("Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    ++arrivedcount;
}


int main(int argc, char* argv[])
{
    float version = 0.6;
    char* topic = "hello/world";

    const char* hostname = "hostname.com";
    int port = 1883;

    logMessage("HelloMQTT: version is %.2f\r\n", version);

    M66Interface network(GSM_UART_TX, GSM_UART_RX, GSM_PWRKEY, GSM_POWER, true);

    network.connect("eseye.com", "ubirch", "internet");


    MQTTNetwork mqttNetwork(&network);

    MQTT::Client<MQTTNetwork, Countdown> client = MQTT::Client<MQTTNetwork, Countdown>(mqttNetwork);


    logMessage("Connecting to %s:%d\r\n", hostname, port);
    int rc = mqttNetwork.connect(hostname, port);
    if (rc != 0)
    logMessage("rc from TCP connect is %d\r\n", rc);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "clientID";
    data.username.cstring = "username";
    data.password.cstring = "password";

    if ((rc = client.connect(data)) != 0) {
        logMessage("rc from MQTT connect is, try again %d\r\n", rc);
    }

    if ((rc = client.subscribe(topic, MQTT::QOS0, messageArrived)) != 0) {
        logMessage("rc from MQTT subscribe is %d\r\n", rc);
    }


    MQTT::Message message;

    uint32_t msgCount = 0;
    
    while (true) {
        // QoS 0

        char buf[100];
        sprintf(buf, "Hello World! %d :: QoS 0 message from app version %f\r\n", msgCount, version);
        message.qos = MQTT::QOS0;
        message.retained = false;
        message.dup = false;
        message.payload = (void *) buf;
        message.payloadlen = strlen(buf) + 1;
        rc = client.publish(topic, message);
        while (arrivedcount < 1)
            client.yield(100);

        msgCount++;
        // publish Hello World message once in 10 sec
        wait(10);
    }

//    // QoS 1
//    sprintf(buf, "Hello World!  QoS 1 message from app version %f\r\n", version);
//    message.qos = MQTT::QOS1;
//    message.payloadlen = strlen(buf)+1;
//    rc = client.publish(topic, message);
//    while (arrivedcount < 2)
//        client.yield(100);
//
//    // QoS 2
//    sprintf(buf, "Hello World!  QoS 2 message from app version %f\r\n", version);
//    message.qos = MQTT::QOS2;
//    message.payloadlen = strlen(buf)+1;
//    rc = client.publish(topic, message);
//    while (arrivedcount < 3)
//        client.yield(100);

//    if ((rc = client.unsubscribe(topic)) != 0)
//    logMessage("rc from unsubscribe was %d\r\n", rc);
//
//    if ((rc = client.disconnect()) != 0)
//    logMessage("rc from disconnect was %d\r\n", rc);
//
//    mqttNetwork.disconnect();
//
//    logMessage("Version %.2f: finish %d msgs\r\n", version, arrivedcount);

    return 0;
}
