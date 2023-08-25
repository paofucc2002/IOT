#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "MQTTClient.h"
#include "mysql.h"

// setup MQTT Broker
#define ADDRESS     "tcp://broker.emqx.io:1883"
#define CLIENTID    "pc"
#define SUB_TOPIC  "giatricambienkhoangcach"
#define PUB_TOPIC   "control"
#define QOS         0

// setup cac bien, con tro ngoai vong lap while 1
MYSQL *conn;
MYSQL_RES *res;
MYSQL_ROW row;
// setup cac bien, con tro ket trong vong lap while 1
MYSQL *connw;
MYSQL_RES *resw;
MYSQL_ROW roww;

// setup ket noi den database tren server
char *server = "localhost";
char *user = "admin";
char *password = "123456";
char *database = "nhomiot";
int status =0;

// ham gui du lieu sang broker
void publish(MQTTClient client, char* topic, char* payload) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(pubmsg.payload);
    pubmsg.qos = 1;
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;
    MQTTClient_publishMessage(client, topic, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, 1000L);
    // printf("Message '%s' with delivery token %d delivered\n", payload, token);
}

// ham nhan du lieu tu broker
int on_message(void *context, char *topicName, int topicLen, MQTTClient_message *message) 
    { 
        char* payload = message->payload;
        int giatri = atoi(payload);
        //ket noi mysql
        conn = mysql_init(NULL);
        if (mysql_real_connect(conn, server, user, password, database, 0, NULL, 0) == NULL) 
            {
                fprintf(stderr, "%s\n", mysql_error(conn));
                mysql_close(conn);
                exit(1);
            }   
        char sql[200];
        // Doc trang thai cam bien can doc tu database
        sprintf(sql,"SELECT status  FROM pqp  WHERE stt=(SELECT max(stt) FROM pqp);");
        mysql_query(conn,sql);
        res = mysql_store_result(conn); 
        row = mysql_fetch_row(res);
        status = atoi(row[0]);

        sprintf(sql,"insert into pqp(khoangcach, status) values (%d,%d)",giatri,status);
        printf("Gia tri khoang cach la %d\n", giatri);
        mysql_query(conn,sql);
        // Xoa result va dong ket noi
        mysql_free_result(res);
        mysql_close(conn);  
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        return 1;
    }



int main(int argc, char* argv[]) {

    // khoi tao mqtt
    MQTTClient client;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    //conn_opts.username = "your_username>>";
    //conn_opts.password = "password";
    // ket noi den broker
    MQTTClient_setCallbacks(client, NULL, NULL, on_message, NULL);
    int rc;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }

    //listen for operation
    MQTTClient_subscribe(client, SUB_TOPIC, 0);  

    while(1) {
            //ket noi mysql
            connw = mysql_init(NULL);
            if (mysql_real_connect(connw, server, user, password, database, 0, NULL, 0) == NULL) 
            {
                fprintf(stderr, "%s\n", mysql_error(connw));
                mysql_close(connw);
                exit(1);
            }
            char sql[200];
            // Doc trang thai cam bien can doc tu database
            sprintf(sql,"SELECT status  FROM pqp  WHERE stt=(SELECT max(stt) FROM pqp);");
            mysql_query(connw,sql);
            resw = mysql_store_result(connw); 
            roww = mysql_fetch_row(resw);
            status = atoi(roww[0]);
            publish(client, PUB_TOPIC,roww[0]);
            // Xoa result va dong ket noi
            mysql_free_result(resw);
            mysql_close(connw); 

            // delay theo yeu cau ung dung
            sleep(0.01);          
    }
    // tu dong ngat ket noi MQTT sau 1000s
    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);
    return rc;
}