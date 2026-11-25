#pragma once

constexpr uintptr_t kVtblSun = 0x2395208;
constexpr uintptr_t kVtblSunBomb = 0x23c96b0;
constexpr uintptr_t kOnLandSun = 0x652758;
constexpr uintptr_t kOnLandBomb = 0xa805d0;

constexpr unsigned  kSlotCollect = 51;
constexpr unsigned  kSlotOnLand = 56;

constexpr uintptr_t kFieldState = 0x90;
constexpr uintptr_t kFieldBombMode = 0x1c8;
constexpr int32_t   kStateCollected = 1;

constexpr uintptr_t OFF_G_DisplayInfo = 0x2593760;

constexpr uintptr_t OFF_PersistSave = 0x14E37EC;
constexpr uintptr_t OFF_PersistReadBool = 0x14E3CC8;
constexpr uintptr_t OFF_PersistManager = 0x2599998;

constexpr uintptr_t OFF_SettingsCreate = 0xA0FF78;
constexpr uintptr_t OFF_CheckboxCreate = 0xA10688;
constexpr uintptr_t OFF_SettingsAddWidget = 0xA10244;
constexpr uintptr_t OFF_SettingsDispatch = 0xA145AC;
constexpr uintptr_t OFF_SettingsDialogPopulate = 0xA0EC6C;
constexpr uintptr_t OFF_SettingsContentCreate = 0x12B9E5C;

constexpr uintptr_t SETTINGS_PAGE_OWNER = 192;   // 0xC0
constexpr uintptr_t SETTINGS_PAGE_DIRTY = 292;   // 0x124

constexpr uintptr_t SETTINGS_PAGE_CONTAINER = 240;

constexpr uintptr_t OFF_SettingsStringCreate = 0x5B5B84;