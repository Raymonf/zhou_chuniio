#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "config.h"

static const int chuni_io_default_cells[] = {
    'L', 'L', 'L', 'L',
    'K', 'K', 'K', 'K',
    'J', 'J', 'J', 'J',
    'H', 'H', 'H', 'H',
    'G', 'G', 'G', 'G',
    'F', 'F', 'F', 'F',
    'D', 'D', 'D', 'D',
    'S', 'S', 'S', 'S',
};

void chuni_io_config_load(
        struct chuni_io_config *cfg,
        const wchar_t *filename)
{
    wchar_t key[16];
    int i;

    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->vk_test = GetPrivateProfileIntW(L"io3", L"test", '1', filename);
    cfg->vk_service = GetPrivateProfileIntW(L"io3", L"service", '2', filename);
    cfg->vk_coin = GetPrivateProfileIntW(L"io3", L"coin", '3', filename);
    cfg->vk_ir = GetPrivateProfileIntW(L"io3", L"ir", VK_SPACE, filename);
    cfg->delay = GetPrivateProfileIntW(L"zhousensor", L"touchdelay", '0', filename);
    cfg->side_R = GetPrivateProfileIntW(L"zhousensor", L"side_red", '0', filename);
    cfg->side_G = GetPrivateProfileIntW(L"zhousensor", L"side_green", '255', filename);
    cfg->side_B = GetPrivateProfileIntW(L"zhousensor", L"side_blue", '0', filename);
    cfg->RealAime = GetPrivateProfileIntW(L"zhousensor", L"RealAime", '0', filename);

    for (i = 0 ; i < 32 ; i++) {
        swprintf_s(key, _countof(key), L"cell%i", i + 1);
        cfg->vk_cell[i] = GetPrivateProfileIntW(
                L"slider",
                key,
                chuni_io_default_cells[i],
                filename);
    }
}
