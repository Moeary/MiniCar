/*
 * Copyright (c) 2020, HiHope Community.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "iot_pwm.h"
#include "iot_errno.h"
#include "hi_io.h"
#include "car_control.h"
#include "KeyEvent.h"
#include "wifi_connecter.h"

#include "lwip/sockets.h"

#include "MQTTClient.h"
#include "cJSON.h"

#define WIFI_SSID "***" //当前wifi
#define WIFI_PW "***"//密码
#define MQTT_BROKER "192.168.31.226"  //mqttIP
#define MQTT_PORT 1883
/*
#define MQTT_BROKER "eb63dee59d.iot-mqtts.cn-north-4.myhuaweicloud.com"  //mqttIP
#define MQTT_PORT 1883
*/
MQTTMessage ackmsg;
int needAck = 0;
static MQTTClient c;
unsigned char *onenet_mqtt_buf;
unsigned char *onenet_mqtt_readbuf;
int buf_size;
Network n;
MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
char recv_mqtt_buf[1024];

struct opts_struct
{
    char *clientid;
    int nodelimiter;
    char *delimiter;
    enum QoS qos;
    char *username;
    char *password;
    char *host;
    int port;
    int showtopics;
} opts = {
    (char *)"6414651136eaa44b9c900dd0_Hi3861_0_0_2023032014",
    0,
    (char *)"\n",
    QOS1,
    (char *)"6414651136eaa44b9c900dd0_Hi3861",
    (char *)"154221f09e84c79275133c99ada5db847e033553352be6df01bac87780e4cdef",
    (char *)MQTT_BROKER,
    MQTT_PORT,
    1};

void messageArrived(MessageData *md)
{

    MQTTMessage *message = md->message;

    memset(&ackmsg, '\0', sizeof(ackmsg));
    ackmsg.qos = QOS0;
    ackmsg.retained = 0;
    ackmsg.dup = 0;
    if (opts.showtopics)
        printf("%.*s\t", md->topicName->lenstring.len, md->topicName->lenstring.data);
    if (opts.nodelimiter)
        printf("%.*s\n", (int)message->payloadlen, (char *)message->payload);
    else
        printf("%.*s%s\n", (int)message->payloadlen, (char *)message->payload, opts.delimiter);

    // // cJSON指针
    // cJSON *obj_root;
    // cJSON *obj_para;
    // cJSON *obj_value;

    // // 将普通的json串处理成json对象
    // obj_root = cJSON_Parse(message->payload);

    // 1 = cJSON_GetObjectItem(obj_root, "333");
    // 2 = cJSON_GetObjectItem(obj_root, "value");
    // {
    //     "pwm_forward": {
    //         "value":"1"
    //     },
    //     "333"
    // }
    // double obj_params = cJSON_GetNumberValue(2);
    // printf("%s", obj_params);
    // if (obj_params == 1)
    // {
    // }
    if (strncmp("forward", message->payload, 7) == 0)
    {
        needAck = 1;
        car_go_forward();
        ackmsg.payload = (void *)"forward";
    }
    if (strncmp("back", message->payload, 4) == 0)
    {
        needAck = 1;
        car_go_back();
        ackmsg.payload = (void *)"back";
    }
    if (strncmp("left", message->payload, 4) == 0)
    {
        needAck = 1;
        car_turn_left();
        ackmsg.payload = (void *)"left";
    }
    if (strncmp("right", message->payload, 5) == 0)
    {
        needAck = 1;
        car_turn_right();
        ackmsg.payload = (void *)"right";
    }
    if (strncmp("stop", message->payload, 4) == 0)
    {
        needAck = 1;
        car_stop();
        ackmsg.payload = (void *)"stop";
    }

    if (needAck == 1)
    {
        ackmsg.payloadlen = strlen((char *)ackmsg.payload);
    }
}

unsigned char buf[100];
unsigned char readbuf[100];

int car_mqtt(void)
{
    int rc = 0;

    MQTTMessage pubmsg;
    char *topic = "carControl";

    if (strchr(topic, '#') || strchr(topic, '+'))
        opts.showtopics = 1;
    if (opts.showtopics)
        printf("topic is %s\n", topic);

    NetworkInit(&n);
    NetworkConnect(&n, opts.host, opts.port);
    // NetworkConnect(&n, "eb63dee59d.iot-mqtts.cn-north-4.myhuaweicloud.com", 1883);

    buf_size = 4096 + 1024;
    onenet_mqtt_buf = (unsigned char *)malloc(buf_size);
    onenet_mqtt_readbuf = (unsigned char *)malloc(buf_size);
    if (!(onenet_mqtt_buf && onenet_mqtt_readbuf))
    {
        printf("No memory for MQTT client buffer!");
        return -2;
    }
    MQTTClientInit(&c, &n, 1000, onenet_mqtt_buf, buf_size, onenet_mqtt_readbuf, buf_size);
    MQTTStartTask(&c);

    // data.willFlag = 0;
    // data.MQTTVersion = 3;
    data.clientID.cstring = opts.clientid;
    data.username.cstring = opts.username;
    data.password.cstring = opts.password;
    data.keepAliveInterval = 30; // 保活间隔
    data.cleansession = 1;       // 清楚会话
    // data.clientID.cstring = "6414651136eaa44b9c900dd0_Hi3861_0_0_2023032014";    //客户端ID
    // data.username.cstring = "6414651136eaa44b9c900dd0_Hi3861";
    // data.password.cstring = "154221f09e84c79275133c99ada5db847e033553352be6df01bac87780e4cdef";

    c.defaultMessageHandler = messageArrived;

    printf("Connecting to %s %d\n", opts.host, opts.port);

    rc = MQTTConnect(&c, &data);
    printf("Connected %d\n", rc);

    printf("Subscribing to %s\n", topic);
    rc = MQTTSubscribe(&c, topic, opts.qos, messageArrived);
    printf("Subscribed %d\n", rc);

    memset(&ackmsg, '\0', sizeof(ackmsg));
    ackmsg.payload = (void *)"ACK";
    ackmsg.payloadlen = strlen((char *)ackmsg.payload);
    ackmsg.qos = QOS0;
    ackmsg.retained = 0;
    ackmsg.dup = 0;

    while (1)
    {
        if (needAck == 1)
        {
            needAck = 0;
            printf("Publish carStatus ackmsg %d %s \n", (int)ackmsg.payloadlen, (char *)ackmsg.payload);
            MQTTPublish(&c, "carStatus", &ackmsg);
        }
        sleep(1);
    }

    printf("Stopping\n");

    MQTTDisconnect(&c);
    NetworkDisconnect(&n);

    return 0;
}

static void KeyEvent_Callback(KEY_ID_TYPE keyid, KEY_EVENT_TYPE event)
{
    printf("[CarDemo] KeyEvent_Callback() : keyid=%d event= %d\n", (int)keyid, (int)event);

    // int ret = -1;
    switch (keyid)
    {
    case KEY_ID_USER:
        printf("[CarDemo] KEY_ID_USER\n");
        if (event == KEY_EVENT_PRESSED)
        { /*  按下事件处理代码 */
            car_stop();
        }
        if (event == KEY_EVENT_LONG_PRESSED)
        { /*  长按事件处理代码 */
        }
        if (event == KEY_EVENT_RELEESED)
        { /*  松开事件处理代码 */
        }
        break;
    case KEY_ID_S1:
        printf("[CarDemo] KEY_ID_S1\n");
        if (event == KEY_EVENT_PRESSED)
        { /*  按下事件处理代码 */
            car_go_forward();
        }
        if (event == KEY_EVENT_LONG_PRESSED)
        { /*  长按事件处理代码 */
            car_turn_left();
        }
        if (event == KEY_EVENT_RELEESED)
        { /*  松开事件处理代码 */
        }
        break;
    case KEY_ID_S2:
        printf("[CarDemo] KEY_ID_S2\n");
        if (event == KEY_EVENT_PRESSED)
        { /*  按下事件处理代码 */
            car_go_back();
        }
        if (event == KEY_EVENT_LONG_PRESSED)
        { /*  长按事件处理代码 */
            car_turn_right();
        }
        if (event == KEY_EVENT_RELEESED)
        { /*  松开事件处理代码 */
        }
        break;

    default:
        break;
    }
}

static void CarDemoTask(void *arg)
{
    int ret = 0;
    (void)arg;

    // setup your AP params
    WifiDeviceConfig apConfig = {0};
    strcpy(apConfig.ssid, WIFI_SSID);
    strcpy(apConfig.preSharedKey, WIFI_PW);
    apConfig.securityType = WIFI_SEC_TYPE_PSK;

    int netId = ConnectToHotspot(&apConfig);

    printf("[CarDemo] ConnectToHotspot done netId=%d!\n", netId);

    ret += KeyEvent_Init(); // 初始化按键事件处理上下文

    /* 设置GPIO_5按键的回调函数，同时需要响应按下，释放以及长按三个事件 */
    /* 按键触发顺序： Pressed -> LongPressed(optional) -> Released */

    ret += KeyEvent_Connect("GPIO_5", KeyEvent_Callback, KEY_EVENT_PRESSED | KEY_EVENT_LONG_PRESSED | KEY_EVENT_RELEESED);
    printf("ok");

    printf("are you ready");

    car_mqtt();

    printf("[CarDemo] create CarDemoTask!\n");
}

static void CarDemo(void)
{
    osThreadAttr_t attr;
    printf("[CarDemo] enter!\n");
    attr.name = "CarDemoTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = osPriorityNormal;

    if (osThreadNew(CarDemoTask, NULL, &attr) == NULL)
    {
        printf("[CarDemo] Falied to create CarDemoTask!\n");
    }
}
// SYS_RUN(CarDemo);
APP_FEATURE_INIT(CarDemo);
