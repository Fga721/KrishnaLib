#include "KrishnaLib.h"
#include "MemUtils.h"
#include "offsets.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

typedef void (*OnLand)(uintptr_t self);
OnLand oSunOnLand = nullptr;
OnLand oSunBombOnLand = nullptr;
std::atomic<bool>      g_enabled{ false };
std::atomic<bool>      g_loaded{ false };
constexpr const char* kAutoSunPrefKey = "AutoSunCollect";
std::atomic<uintptr_t> g_base{ 0 };
typedef long (*SaveBool)(uintptr_t, uintptr_t, char);
typedef long (*ReadBool)(uintptr_t, uintptr_t, uintptr_t);

struct GKey { uint64_t flag, size, heap; };   // libc++ std::string, 24B

void makeKey(GKey& s, const char* key) {
	char* buf = (char*)operator new(0x20);
	s.flag = 0x21;
	s.size = strlen(key);
	s.heap = (uintptr_t)buf;
	strcpy(buf, key);
}
void freeKey(GKey& s) { if (s.flag & 1) operator delete((void*)s.heap); }

inline int32_t stateOf(uintptr_t o)
{
	return *(int32_t*)(o + kFieldState);
}

void loadSetting() {
	uintptr_t base = g_base.load(std::memory_order_acquire);
	if (!base) base = KrishnaGetLibraryAddress("libPVZ2.so");
	uintptr_t app = base ? *(uintptr_t*)(base + OFF_G_DisplayInfo) : 0;
	if (!app) return;                       // se reintenta en la pr�xima consulta

	GKey k; makeKey(k, kAutoSunPrefKey);
	uint8_t v = 0;
	long hit = ((ReadBool)(base + OFF_PersistReadBool))(app, (uintptr_t)&k, (uintptr_t)&v);
	freeKey(k);

	g_enabled.store(hit ? (v != 0) : false, std::memory_order_release);   // por defecto: desactivado
	g_loaded.store(true, std::memory_order_release);
}

void collect(uintptr_t self, const char* tag)
{
	uintptr_t vt = *(uintptr_t*)self;
	auto fn = *(void (**)(uintptr_t))(vt + kSlotCollect * sizeof(void*));
	fn(self);
}

bool AutoSunIsEnabled() {
	if (!g_loaded.load(std::memory_order_acquire)) loadSetting();
	return g_enabled.load(std::memory_order_acquire);
}

void hkSunOnLand(uintptr_t self)
{
	oSunOnLand(self);
	if (AutoSunIsEnabled() == true && stateOf(self) != kStateCollected)
	{
		collect(self, "sun");
	}
}

void hkBombOnLand(uintptr_t self)
{
	oSunBombOnLand(self);
	if (AutoSunIsEnabled() == true && stateOf(self) != kStateCollected && *(int32_t*)(self + kFieldBombMode) == 0)
	{
		collect(self, "sun_bomb");
	}
}

void AutoSunSetEnabled(bool v) {
	g_enabled.store(v, std::memory_order_release);
	g_loaded.store(true, std::memory_order_release);

	uintptr_t base = g_base.load(std::memory_order_acquire);
	if (!base) return;
	uintptr_t mgr = *(uintptr_t*)(base + OFF_PersistManager);
	if (mgr) {
		GKey k; makeKey(k, kAutoSunPrefKey);
		((SaveBool)(base + OFF_PersistSave))(mgr, (uintptr_t)&k, v ? 1 : 0);
		freeKey(k);
	}
}

// Settings Hook

typedef uintptr_t(*CreateTab)(uintptr_t, uint32_t, uintptr_t, uintptr_t, uintptr_t);
typedef long      (*AddWidget)(uintptr_t, uintptr_t, uint8_t, float);
typedef int       (*Dispatch)(uintptr_t, uint32_t, char);
typedef uintptr_t(*CreateCB)(uintptr_t, uint32_t, uintptr_t, char, int);
typedef void      (*StrCreate)(uintptr_t, const wchar_t*, uint32_t);

static CreateTab oCreateTab;
static AddWidget oAddWidget;
static Dispatch  oDispatch;
static CreateCB  oCreateCB;

static bool      g_injected;
static uintptr_t g_hiddenCB;

static constexpr uint32_t kAutoSunId = 30;

static uintptr_t hkCreateTab(uintptr_t page, uint32_t id, uintptr_t title,
    uintptr_t iconN, uintptr_t iconS) {
    if (id == 3) { // change order in settings
        g_injected = false;
        g_hiddenCB = 0;
    }

    if (!g_injected && id == 6 && oCreateCB && oAddWidget) {
        g_injected = true;
        uintptr_t base = KrishnaGetLibraryAddress("libPVZ2.so");
        uintptr_t cont = *(uintptr_t*)(page + SETTINGS_PAGE_CONTAINER);

        // Auto Sun Collect
        alignas(16) uint8_t tb_sun[24] = {};
        ((StrCreate)(base + OFF_SettingsStringCreate))((uintptr_t)tb_sun, L"Auto Sun Collect", 16);
        uintptr_t cb_sun = oCreateCB(page, kAutoSunId, (uintptr_t)tb_sun, AutoSunIsEnabled() ? 1 : 0, 0);
        oAddWidget(cont, cb_sun, 0, 0.0f);
    }
    return oCreateTab(page, id, title, iconN, iconS);
}


static uintptr_t hkCreateCB(uintptr_t page, uint32_t id, uintptr_t label, char init, int p5) {
    if (id == 48) init = 0; // disable slib full lawn since it's buggy
    uintptr_t cb = oCreateCB(page, id, label, init, p5);
    if (id == 48 && cb) g_hiddenCB = cb; // hide slib full lawn, same reason
    return cb;
}

static long hkAddWidget(uintptr_t cont, uintptr_t w, uint8_t centered, float s) {
    if (!w || w == g_hiddenCB) return 0;
    return oAddWidget(cont, w, centered, s);
}

static int hkDispatch(uintptr_t page, uint32_t id, char checked) {
    if (id == kAutoSunId) {
		AutoSunSetEnabled(checked != 0);
        *(uint8_t*)(page + SETTINGS_PAGE_DIRTY) = 1;
        return 0;
    }

    if (id == 4) return 0;
    return oDispatch ? oDispatch(page, id, checked) : 0;
}

__attribute__((constructor))
void libKrishna_main()
{
	KrishnaHookFunction(OFF_SettingsAddWidget, (void*)hkAddWidget, (void**)&oAddWidget);
	KrishnaHookFunction(OFF_SettingsCreate, (void*)hkCreateTab, (void**)&oCreateTab);
	KrishnaHookFunction(OFF_SettingsDispatch, (void*)hkDispatch, (void**)&oDispatch);
	KrishnaHookFunction(OFF_CheckboxCreate, (void*)hkCreateCB, (void**)&oCreateCB);

	g_base.store(g_libAddress, std::memory_order_release);
	PatchVTableSlot(KrishnaGetActualOffset(kVtblSun) + kSlotOnLand * 8, KrishnaGetActualOffset(kOnLandSun), (void*)hkSunOnLand, (void**)&oSunOnLand);
	PatchVTableSlot(KrishnaGetActualOffset(kVtblSunBomb) + kSlotOnLand * 8, KrishnaGetActualOffset(kOnLandBomb), (void*)hkBombOnLand, (void**)&oSunBombOnLand);
}