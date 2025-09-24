#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"


#define ADDRESS     "tcp://192.168.219.103:1883"       // 브로커 주소
#define CLIENTID    "dev1-publisher"
#define DEVICEID "dev1"
#define EMECYTOPIC  "msg/emecy/" DEVICEID
#define LOCATIONTOPIC  "msg/location/" DEVICEID
#define QOS         1
#define TIMEOUT     10000L

int main() {
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

    // 1. 클라이언트 생성
    MQTTClient_create(&client, ADDRESS, CLIENTID,
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);

    // 2. 연결 옵션 설정
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    // 3. 브로커 연결
    int rc;
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("MQTT 연결 실패. 코드: %d\n", rc);
        return -1;
    }

    // 4. 예시 payload (JSON 형식)
    const char* payload = "{\"deviceId\":\"pi3\",\"lat\":37.123,\"lon\":127.456}";
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = (char*)payload;
    pubmsg.payloadlen = (int)strlen(payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    // 5. 메시지 전송
    MQTTClient_deliveryToken token;
    MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, TIMEOUT);

    printf("위치 메시지 전송 완료: %s\n", payload);

    // 6. 연결 종료
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return 0;
}

