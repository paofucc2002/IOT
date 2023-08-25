// cac thu vien cho mqtt
#include"stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "MQTTClient.h"
//  cac thu vien cho I2C va GPIO
#include <wiringPi.h>
#include <time.h>
// define cac chan cho GPIO va dia chi I2C
#define TRIG_PIN 4
#define ECHO_PIN 5
// define MQTT broker
#define ADDRESS     "tcp://broker.emqx.io:1883"
#define CLIENTID    "ras"
#define SUB_TOPIC   "control"
#define PUB_TOPIC   "giatricambienkhoangcach"
#define QOS         0
// khai bao bien trang thai tuong ung vs tan suat lay mau
int status;
int tansuatlaymau =0;

void publish(MQTTClient client, char* topic, char* payload) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(pubmsg.payload);
    pubmsg.qos = 1;
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;
    MQTTClient_publishMessage(client, topic, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, 1000L);
    //printf("Message '%s' with delivery token %d delivered\n", payload, token);
}

int on_message(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char* payload = message->payload;
    
    status = atoi(payload);
    if(status ==0)
    {
        // thay doi tan suat lay mau thanh 3s
        tansuatlaymau = 5;
        printf("tan suat lay mau la 5 giay \n");  
    }
    else
    {
        // thay doi tan suat lay mau thanh 1s
        tansuatlaymau = 1;
        printf("tan suat lay mau la 1 giay\n"); 
    }
    printf("Thay dong nay coi nhu qua mon\n"); 
    //printf("Received message: %s\n", payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

    
int main(int argc, char* argv[]) {
    char giatri[10];
    // Khởi tạo thư viện WiringPi
    wiringPiSetup();
    // kiem tra wiringPiSetup
     if (wiringPiSetup() == -1) {
    printf("Failed to initialize wiringPi\n");
    return 1;
	}

    pinMode(TRIG_PIN, OUTPUT);
	pinMode(ECHO_PIN, INPUT);

    MQTTClient client;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    //conn_opts.username = "your_username>>";
    //conn_opts.password = "password";

    MQTTClient_setCallbacks(client, NULL, NULL, on_message, NULL);
    // kiem tra ket noi den MQTT broker
    int rc;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }
   
    //listen for operation
    MQTTClient_subscribe(client, SUB_TOPIC, 0);
    
    while(1) {
        // Doc gia tri khoang cach
        digitalWrite(TRIG_PIN, LOW);
		delayMicroseconds(2);
		digitalWrite(TRIG_PIN, HIGH);
		delayMicroseconds(10);
		digitalWrite(TRIG_PIN, LOW);

		while (digitalRead(ECHO_PIN) == LOW);

		long startTime = micros();
		while (digitalRead(ECHO_PIN) == HIGH);
		long travelTime = micros() - startTime;

		int distance = travelTime / 58;
        // In ra khoang cach
        printf("Distance: %d cm\n", distance);
        sprintf(giatri,"%d",distance);
        
        //send khoang cach
        if(giatri !=NULL)
        {
        publish(client, PUB_TOPIC,giatri);
        sleep(tansuatlaymau);
        }
    }
        // tu dong ngat ket noi MQTT sau 1000s    
        MQTTClient_disconnect(client, 1000);
        MQTTClient_destroy(&client);
        return rc;
}




