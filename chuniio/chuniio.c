#include <windows.h>

#include <process.h>
#include <stdbool.h>
#include <stdint.h>

#include "chuniio.h"
#include "config.h"



extern int c_HIDUSB_Init(void);
extern int c_HIDUSB_Run(void);

extern uint8_t * c_HIDUSB_GetKeyValue(void);
extern uint8_t c_HIDUSB_GetButton(void);
extern uint8_t c_HIDUSB_GetBeams(void);
extern uint16_t c_HIDUSB_GetCoins(void);
extern uint8_t c_HIDUSB_GetCardStatue(unsigned char* id);

extern int c_HIDUSB_SetLedValue(uint8_t * p_buff);
extern int c_HIDUSB_SetReaderLED(uint8_t * p_buff);

extern void c_DelayInit(uint8_t delay);

extern void DataManage_Init(uint8_t* realaimeFlag);

static unsigned int __stdcall chuni_io_slider_thread_proc(void *ctx);

static bool chuni_io_coin;
static uint16_t chuni_io_coins;
static uint8_t chuni_io_hand_pos;
static HANDLE chuni_io_slider_thread;
static bool chuni_io_slider_stop_flag;
static struct chuni_io_config chuni_io_cfg;

HRESULT chuni_io_jvs_init(void)
{
    chuni_io_config_load(&chuni_io_cfg, L".\\segatools.ini");
    c_DelayInit(1);

    if (c_HIDUSB_Init() != 0)
    {
        //return S_FALSE;  测试手台用，避免报错
        return S_OK;
    }
    DataManage_Init(&chuni_io_cfg.RealAime);
    SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_CONTINUOUS);
    return S_OK;
}

void chuni_io_jvs_read_coin_counter(uint16_t *out)
{
    if (out == NULL) {
        return;
    }

    if (GetAsyncKeyState(chuni_io_cfg.vk_coin)) {
        if (!chuni_io_coin) {
            chuni_io_coin = true;
            chuni_io_coins++;
        }
    }
    else {
        chuni_io_coin = false;
    }

    *out = c_HIDUSB_GetCoins() + chuni_io_coins;
}

void chuni_io_jvs_poll(uint8_t *opbtn, uint8_t *beams)
{

    uint8_t button = (c_HIDUSB_GetButton() & 0x03);
    
    if (GetAsyncKeyState(chuni_io_cfg.vk_test)) {
        button |= 0x01; /* Test */
    }

    if (GetAsyncKeyState(chuni_io_cfg.vk_service)) {
        button |= 0x02; /* Service */
    }

    *opbtn = button;
    *beams = c_HIDUSB_GetBeams();

}

void chuni_io_jvs_set_coin_blocker(bool open)
{}

HRESULT chuni_io_slider_init(void)
{
    return S_OK;
}

void chuni_io_slider_start(chuni_io_slider_callback_t callback)
{
    if (chuni_io_slider_thread != NULL) {
        return;
    }

    chuni_io_slider_thread = (HANDLE) _beginthreadex(
            NULL,
            0,
            chuni_io_slider_thread_proc,
            callback,
            0,
            NULL);
}

void chuni_io_slider_stop(void)
{
    if (chuni_io_slider_thread == NULL) {
        return;
    }

    chuni_io_slider_stop_flag = true;

    WaitForSingleObject(chuni_io_slider_thread, INFINITE);
    CloseHandle(chuni_io_slider_thread);
    chuni_io_slider_thread = NULL;
    chuni_io_slider_stop_flag = false;
}

static int irledcount = 0;
#define irmaxcount 255
void chuni_io_slider_set_leds(const uint8_t *rgb)
{
    unsigned char RGB[96] = { 0 };

    // RBG -> RGB?
    // TODO: check out what this does
    for (int i = 0; i < 31; i++)
    {
        RGB[i * 3 + 0] = rgb[i * 3 + 1];
        RGB[i * 3 + 1] = rgb[i * 3 + 2];
        RGB[i * 3 + 2] = rgb[i * 3 + 0];
    }

    RGB[93] = (unsigned char)((chuni_io_cfg.side_R * irledcount) / irmaxcount);
    RGB[94] = (unsigned char)((chuni_io_cfg.side_G * irledcount) / irmaxcount);
    RGB[95] = (unsigned char)((chuni_io_cfg.side_B * irledcount) / irmaxcount);

    c_HIDUSB_SetLedValue(RGB);
}

static unsigned int __stdcall chuni_io_slider_thread_proc(void *ctx)
{
    chuni_io_slider_callback_t callback;

    callback = ctx;

    while (!chuni_io_slider_stop_flag) {
        c_HIDUSB_Run();
        callback(c_HIDUSB_GetKeyValue());

        if (c_HIDUSB_GetBeams() != 0)
        {
            irledcount = irmaxcount;
        }
        else
        {
            if (irledcount > 0)
                irledcount--;
        }

    }

    return 0;
}

