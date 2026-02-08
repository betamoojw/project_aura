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
#include <stddef.h>
#include <string.h>

enum { UI_KNOWN_SCREEN_COUNT = 13 };
enum { UI_PAGE_SLOT_COUNT = (int)(offsetof(objects_t, label_boot_ver) / sizeof(lv_obj_t *)) };

#if defined(__cplusplus)
#define UI_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define UI_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

UI_STATIC_ASSERT(UI_PAGE_SLOT_COUNT == UI_KNOWN_SCREEN_COUNT,
                 "EEZ page layout changed: update ui_runtime screen tables.");
UI_STATIC_ASSERT(SCREEN_ID_PAGE_MAIN_PRO == UI_KNOWN_SCREEN_COUNT,
                 "Expected MAIN_PRO to be last screen id; update ui_runtime mapping.");

enum { UI_MAX_SCREEN_ID = UI_KNOWN_SCREEN_COUNT };

static int16_t currentScreen = -1;
static uint8_t createdScreens[UI_MAX_SCREEN_ID + 1];

static enum ScreensEnum normalizeScreenId(enum ScreensEnum screenId) {
    // Old MAIN (id=3) is deprecated; keep compatibility by redirecting to MAIN_PRO.
    if (screenId == SCREEN_ID_PAGE_MAIN) {
        return SCREEN_ID_PAGE_MAIN_PRO;
    }
    return screenId;
}

static bool isScreenIdValid(enum ScreensEnum screenId) {
    return screenId >= SCREEN_ID_PAGE_BOOT_LOGO && screenId <= UI_MAX_SCREEN_ID;
}

static lv_obj_t *getLvglObjectFromScreenId(enum ScreensEnum screenId) {
    if (!isScreenIdValid(screenId)) {
        return 0;
    }
    lv_obj_t **page_slots = (lv_obj_t **)&objects;
    return page_slots[screenId - 1];
}

typedef void (*create_screen_func_t)(void);

static const create_screen_func_t screen_create_funcs[UI_MAX_SCREEN_ID + 1] = {
    NULL,
    create_screen_page_boot_logo,
    create_screen_page_boot_diag,
    create_screen_page_main,
    create_screen_page_settings,
    create_screen_page_wifi,
    create_screen_page_theme,
    create_screen_page_clock,
    create_screen_page_co2_calib,
    create_screen_page_auto_night_mode,
    create_screen_page_backlight,
    create_screen_page_mqtt,
    create_screen_page_sensors_info,
    create_screen_page_main_pro,
};

static void createScreenById(enum ScreensEnum screenId) {
    if (!isScreenIdValid(screenId)) {
        return;
    }
    create_screen_func_t create_fn = screen_create_funcs[screenId];
    if (create_fn) {
        create_fn();
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
