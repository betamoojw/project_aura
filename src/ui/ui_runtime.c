#if defined(EEZ_FOR_LVGL)
#include <eez/core/vars.h>
#endif

#include "ui.h"
#include "screens.h"
#include "images.h"
#include "actions.h"
#include "vars.h"

#if defined(EEZ_FOR_LVGL)

void ui_init() {
    eez_flow_init(assets, sizeof(assets), (lv_obj_t **)&objects, sizeof(objects), images, sizeof(images), actions);
}

void ui_tick() {
    eez_flow_tick();
    tick_screen(g_currentScreen);
}

#else

#include <stdbool.h>
#include <string.h>

// Keep this mapping in sync with screens.h when EEZ adds/removes pages.
enum { UI_MAX_SCREEN_ID = SCREEN_ID_PAGE_MAIN_PRO };

static int16_t currentScreen = -1;
static uint8_t createdScreens[UI_MAX_SCREEN_ID + 1];

static enum ScreensEnum normalizeScreenId(enum ScreensEnum screenId) {
    if (screenId == SCREEN_ID_PAGE_MAIN) {
        return SCREEN_ID_PAGE_MAIN_PRO;
    }
    return screenId;
}

static bool isScreenIdValid(enum ScreensEnum screenId) {
    return screenId >= SCREEN_ID_PAGE_BOOT_LOGO && screenId <= UI_MAX_SCREEN_ID;
}

static lv_obj_t *getLvglObjectFromScreenId(enum ScreensEnum screenId) {
    switch (screenId) {
        case SCREEN_ID_PAGE_BOOT_LOGO:
            return objects.page_boot_logo;
        case SCREEN_ID_PAGE_BOOT_DIAG:
            return objects.page_boot_diag;
        case SCREEN_ID_PAGE_MAIN:
            return objects.page_main_pro ? objects.page_main_pro : objects.page_main;
        case SCREEN_ID_PAGE_SETTINGS:
            return objects.page_settings;
        case SCREEN_ID_PAGE_WIFI:
            return objects.page_wifi;
        case SCREEN_ID_PAGE_THEME:
            return objects.page_theme;
        case SCREEN_ID_PAGE_CLOCK:
            return objects.page_clock;
        case SCREEN_ID_PAGE_CO2_CALIB:
            return objects.page_co2_calib;
        case SCREEN_ID_PAGE_AUTO_NIGHT_MODE:
            return objects.page_auto_night_mode;
        case SCREEN_ID_PAGE_BACKLIGHT:
            return objects.page_backlight;
        case SCREEN_ID_PAGE_MQTT:
            return objects.page_mqtt;
        case SCREEN_ID_PAGE_SENSORS_INFO:
            return objects.page_sensors_info;
        case SCREEN_ID_PAGE_MAIN_PRO:
            return objects.page_main_pro;
        default:
            return 0;
    }
}

static void createScreenById(enum ScreensEnum screenId) {
    switch (screenId) {
        case SCREEN_ID_PAGE_BOOT_LOGO:
            create_screen_page_boot_logo();
            break;
        case SCREEN_ID_PAGE_BOOT_DIAG:
            create_screen_page_boot_diag();
            break;
        case SCREEN_ID_PAGE_MAIN:
            create_screen_page_main_pro();
            break;
        case SCREEN_ID_PAGE_SETTINGS:
            create_screen_page_settings();
            break;
        case SCREEN_ID_PAGE_WIFI:
            create_screen_page_wifi();
            break;
        case SCREEN_ID_PAGE_THEME:
            create_screen_page_theme();
            break;
        case SCREEN_ID_PAGE_CLOCK:
            create_screen_page_clock();
            break;
        case SCREEN_ID_PAGE_CO2_CALIB:
            create_screen_page_co2_calib();
            break;
        case SCREEN_ID_PAGE_AUTO_NIGHT_MODE:
            create_screen_page_auto_night_mode();
            break;
        case SCREEN_ID_PAGE_BACKLIGHT:
            create_screen_page_backlight();
            break;
        case SCREEN_ID_PAGE_MQTT:
            create_screen_page_mqtt();
            break;
        case SCREEN_ID_PAGE_SENSORS_INFO:
            create_screen_page_sensors_info();
            break;
        case SCREEN_ID_PAGE_MAIN_PRO:
            create_screen_page_main_pro();
            break;
        default:
            break;
    }
}

static bool isScreenEager(enum ScreensEnum screenId) {
    switch (screenId) {
        case SCREEN_ID_PAGE_BOOT_LOGO:
        case SCREEN_ID_PAGE_BOOT_DIAG:
        case SCREEN_ID_PAGE_MAIN_PRO:
        case SCREEN_ID_PAGE_SETTINGS:
            return true;
        default:
            return false;
    }
}

static void markCreatedScreensFromObjects(void) {
    for (int id = SCREEN_ID_PAGE_BOOT_LOGO; id <= UI_MAX_SCREEN_ID; ++id) {
        enum ScreensEnum screenId = (enum ScreensEnum)id;
        if (getLvglObjectFromScreenId(screenId)) {
            createdScreens[id] = 1;
        }
    }
}

static void ensureScreenCreated(enum ScreensEnum screenId) {
    if (!isScreenIdValid(screenId)) {
        return;
    }
    if (createdScreens[screenId]) {
        return;
    }
    if (getLvglObjectFromScreenId(screenId)) {
        createdScreens[screenId] = 1;
        return;
    }
    createScreenById(screenId);
    if (getLvglObjectFromScreenId(screenId)) {
        createdScreens[screenId] = 1;
    }
}

void loadScreen(enum ScreensEnum screenId) {
    screenId = normalizeScreenId(screenId);
    if (!isScreenIdValid(screenId)) {
        return;
    }
    ensureScreenCreated(screenId);
    currentScreen = screenId - 1;
    lv_obj_t *screen = getLvglObjectFromScreenId(screenId);
    if (!screen) {
        return;
    }
    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false);
}

void ui_init() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp,
                                              lv_palette_main(LV_PALETTE_BLUE),
                                              lv_palette_main(LV_PALETTE_RED),
                                              false,
                                              LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    memset(createdScreens, 0, sizeof(createdScreens));
    for (int id = SCREEN_ID_PAGE_BOOT_LOGO; id <= UI_MAX_SCREEN_ID; ++id) {
        enum ScreensEnum screenId = (enum ScreensEnum)id;
        if (isScreenEager(screenId)) {
            createScreenById(screenId);
        }
    }
    markCreatedScreensFromObjects();
    loadScreen(SCREEN_ID_PAGE_BOOT_LOGO);
}

void ui_tick() {
    if (currentScreen < 0) {
        return;
    }
    tick_screen(currentScreen);
}

#endif
