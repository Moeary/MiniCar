#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "hi_adc.h"
#include "hi_systick.h"
#include "hi_gpio.h"
#include "hi_io.h"

#include "KeyEvent.h"


#define KEY_EVENT_STACK_SIZE   2048
#define PRESS_INTERVAL      10000
#define LOOP_INTERVAL       1000
#define LONG_PRESS_INTERVAL 64
#define LONG_PRESS_END      0xFFFF
#define INDEX_ERR           -1

typedef struct
{
    const char* name;
    unsigned int gpio;
    unsigned int event;
    PKeyEventCallback callback;
} KeyInfo;

typedef struct
{
    KEY_ID_TYPE keyid;
    int pressClicked;
    int longPressClicked;
    int longPressInterval;
    int releaseClicked;
} KeyClicked;

enum
{
    ADC_USR_MIN = 5,
    ADC_USR_MAX = 228,
    ADC_S1_MIN,
    ADC_S1_MAX  = 512, 
    ADC_S2_MIN,
    ADC_S2_MAX  = 854
};

static volatile hi_u64 gHisTick = 0;
static volatile int gToClick = 0;
static volatile int gIsInit = 0;

static const char* gKeyGPIOName[] = {
    "GPIO_0",  "GPIO_1",  "GPIO_2",  "GPIO_3",  "GPIO_4",
    "GPIO_5",  "GPIO_6",  "GPIO_7",  "GPIO_8",  "GPIO_9",
    "GPIO_10", "GPIO_11", "GPIO_12", "GPIO_13", "GPIO_14",
    NULL
};

static volatile KeyInfo gKeyInfo[HI_GPIO_IDX_MAX] = {0};
static volatile KeyClicked gKeyClicked[HI_GPIO_IDX_MAX] = {0};

 hi_u8 OnKeyPressed(hi_void* arg);
 hi_u8 OnKeyReleased(hi_void* arg);


KEY_ID_TYPE GetKeyID(hi_gpio_idx gpio)
{
    unsigned short data = 0;
    KEY_ID_TYPE ret = KEY_ID_NONE;
    
    if( HI_GPIO_IDX_8 == gpio ){
        ret = KEY_ID_GPIO8;
    }else if( HI_GPIO_IDX_5 == gpio ){
        if( hi_adc_read(HI_ADC_CHANNEL_2, &data, HI_ADC_EQU_MODEL_4, HI_ADC_CUR_BAIS_DEFAULT, 0) == 0 )
        {
            if( (ADC_USR_MIN <= data) && (data <= ADC_USR_MAX) )  ret = KEY_ID_USER;
            if( (ADC_S1_MIN  <= data) && (data <= ADC_S1_MAX ) )  ret = KEY_ID_S1;
            if( (ADC_S2_MIN  <= data) && (data <= ADC_S2_MAX ) )  ret = KEY_ID_S2;
        }
    }else{

    }
    return ret;
}

static int GetGpioIndex(const char* name)
{
    int ret = INDEX_ERR;
    int i = 0;
    while( gKeyGPIOName[i] && name )
    {
        if( strcmp(gKeyGPIOName[i], name) == 0 )
        {
            ret = i;
            break;
        }

        i++;
    }
    return ret;
}

hi_u8 OnKeyPressed(hi_void* arg)
{
    hi_gpio_idx gpio = (hi_gpio_idx)arg;
    hi_u64 tick = hi_systick_get_cur_tick();
    printf("[soon] OnKeyPressed()\n");
    gToClick = (tick - gHisTick) > PRESS_INTERVAL;
    
    if( gToClick )
    {
        gHisTick = tick;
        gKeyClicked[gpio].keyid = GetKeyID(gpio);
        gKeyClicked[gpio].pressClicked = 1;

        hi_gpio_register_isr_function(gpio,
                            HI_INT_TYPE_EDGE, 
                            HI_GPIO_EDGE_RISE_LEVEL_HIGH,
                            OnKeyReleased, arg);
    }          
}

hi_u8 OnKeyReleased(hi_void* arg)
{
    hi_gpio_idx gpio = (hi_gpio_idx)arg;
    printf("[soon] OnKeyReleased()\n");
    if( gToClick )
    {
        gKeyClicked[gpio].releaseClicked = 1;

        hi_gpio_register_isr_function(gpio,
                            HI_INT_TYPE_EDGE, 
                            HI_GPIO_EDGE_FALL_LEVEL_LOW,
                            OnKeyPressed, arg);
    }
}

static void CommonEventHandler(void)
{
    int i = 0;
    
    for(i=0; i<HI_GPIO_IDX_MAX; i++)
    {
        if( (gKeyInfo[i].event & KEY_EVENT_PRESSED) && gKeyClicked[i].pressClicked )
        {
            printf("[soon] CommonEventHandler pressd()\n");
            gKeyInfo[i].callback(gKeyClicked[i].keyid, KEY_EVENT_PRESSED);
            gKeyClicked[i].pressClicked = 0;
            gKeyClicked[i].longPressInterval = 0;
        }

        if( gKeyClicked[i].longPressInterval < LONG_PRESS_END )
        {
            gKeyClicked[i].longPressInterval++;
        }

        if( gKeyClicked[i].longPressInterval == LONG_PRESS_INTERVAL )
        {
            gKeyClicked[i].longPressClicked = 1;
        }

        if( (gKeyInfo[i].event & KEY_EVENT_LONG_PRESSED) && gKeyClicked[i].longPressClicked )
        {
            printf("[soon] CommonEventHandler long pressd()\n");
            gKeyInfo[i].callback(gKeyClicked[i].keyid, KEY_EVENT_LONG_PRESSED);
            gKeyClicked[i].longPressClicked = 0;
            gKeyClicked[i].longPressInterval = LONG_PRESS_END;
        }

        if( (gKeyInfo[i].event & KEY_EVENT_RELEESED) && gKeyClicked[i].releaseClicked )
        {
            printf("[soon] CommonEventHandler releesed()\n");
            gKeyInfo[i].callback(gKeyClicked[i].keyid, KEY_EVENT_RELEESED);
            gKeyClicked[i].releaseClicked = 0;
            gKeyClicked[i].longPressInterval= LONG_PRESS_END;
        }
    }
}

static void* KeyEvent_Task(const char* arg)
{
    (void)arg;

    while( gIsInit )
    {
        CommonEventHandler();

        usleep(LOOP_INTERVAL);
    }
    
    return NULL;
}


int KeyEvent_Init(void)
{
    int ret = (int)hi_gpio_init();

    if( ret == (int)HI_ERR_GPIO_REPEAT_INIT )
    {
        ret = 0;
    }

    if( !ret && !gIsInit )
    {
        int i = 0;
        osThreadAttr_t attr = {0};

        for(i=0; i<HI_GPIO_IDX_MAX; i++)
        {
            gKeyClicked[i].longPressInterval = LONG_PRESS_END;
        }

        attr.name = "KeyEvent_Task";
        attr.attr_bits = 0U;
        attr.cb_mem = NULL;
        attr.cb_size = 0U;
        attr.stack_mem = NULL;
        attr.stack_size = KEY_EVENT_STACK_SIZE;
        attr.priority = osPriorityNormal;

        ret += (osThreadNew((osThreadFunc_t)KeyEvent_Task, NULL, &attr) == NULL);

        gIsInit = (ret == 0);
    }

    return ret;
}

int KeyEvent_Connect(const char* name, PKeyEventCallback callback, unsigned int event)
{
    int ret = -1;
    int gpio = name ? GetGpioIndex(name) : INDEX_ERR;

    if( (HI_GPIO_IDX_0 <= gpio) && (gpio < HI_GPIO_IDX_MAX) && callback && event)
    {   
        ret  = hi_io_set_func((hi_gpio_idx)gpio, 0);
        ret += hi_gpio_set_dir((hi_gpio_idx)gpio, HI_GPIO_DIR_IN);
        ret += hi_io_set_pull((hi_gpio_idx)gpio, HI_IO_PULL_UP);
        ret += hi_gpio_register_isr_function((hi_gpio_idx)gpio,
                            HI_INT_TYPE_EDGE,
                            HI_GPIO_EDGE_FALL_LEVEL_LOW,
                            OnKeyPressed, (char*)gpio);
    }

    if( ret == 0 )
    {
        gKeyInfo[gpio].name = name;
        gKeyInfo[gpio].gpio = gpio;
        gKeyInfo[gpio].event = event;
        gKeyInfo[gpio].callback = callback;
    }

    return ret;
}

void KeyEvent_Disconnect(const char* name)
{
    int gpio = name ? GetGpioIndex(name) : INDEX_ERR;
    
    if( gpio != INDEX_ERR )
    {
        gKeyInfo[gpio].name = 0;
        gKeyInfo[gpio].gpio = 0;
        gKeyInfo[gpio].event = 0;
        gKeyInfo[gpio].callback = 0;
    }
}

void KeyEvent_Close(void)
{
    gIsInit = 0;
}
