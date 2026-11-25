#pragma once
#include <unwind.h>
#include <dlfcn.h>
#include <unistd.h>
#include <cstdlib>
#include <stdio.h>
#include "Logging.h"
#include <cstdint>
#include <type_traits>

extern uintptr_t g_libAddress;
// Get base address of a library (.so) loaded in memory.
uintptr_t KrishnaGetLibraryAddress(const char* libName);
// Get actual offset of address inside libPVZ2.so
uintptr_t KrishnaGetActualOffset(uintptr_t offset);
// Hook a function in libPVZ2
void KrishnaHookFunction(uintptr_t offset, void* replace, void** result);
// Copy vftable from memory
void* CopyVFTable(uintptr_t vftableAddr, int64_t numVFuncs);
// Copy vftable from memory with destination
void CopyVFTable(void* dest, intptr_t vftableAddr, int64_t numVFuncsToCopy);
// Patch a virtual function without the need to hook it
void PatchVFTable(void* vftable, void* funcAddr, int64_t index);
// Patch an existent vtable
bool PatchVTableSlot(uintptr_t slotAddr, uintptr_t expected, void* hook, void** orig);
// Set an RtClass to the vftable in a ctor
void SetVFTable(void* obj, uintptr_t newVftablePtr);
// Get a virtual function from vftable
void* GetVirtualFunc(void* obj, int index);
// Call a function from libPVZ2.so. It supports multiple parameters, so it's very flexible and shortens the time of decompiling functions
template<typename R, typename... Args>
R CallFunc(uintptr_t funcOffset, Args... args)
{
    typedef R(*func)(Args...);
    func castedFunc = (func)KrishnaGetActualOffset(funcOffset);
    return castedFunc(args...);
}
// Call a virtual function from libPVZ2.so. The vfunc index should be correct (ex: it usually should be divided by 4 in 32 bit and 8 in 64 bit)
template<typename R, typename... Args>
R CallVirtualFunc(void* obj, int index, Args... args)
{
    void* func = GetVirtualFunc(obj, index);
    auto castedFunc = reinterpret_cast<R(*)(void*, Args...)>(func);
    return castedFunc(obj, args...);
}