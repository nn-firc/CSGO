#define STEAM_API_EXPORTS

#include "steam_api.h"

//steam_api.h
S_API bool S_CALLTYPE SteamAPI_Init() {
	return true;
}

S_API void S_CALLTYPE SteamAPI_Shutdown() {
	return;
}

S_API bool S_CALLTYPE SteamAPI_RestartAppIfNecessary(uint32 unOwnAppID) {
	return false;
}

S_API void S_CALLTYPE SteamAPI_ReleaseCurrentThreadMemory() {
	return;
}

S_API void S_CALLTYPE SteamAPI_WriteMiniDump(uint32 uStructuredExceptionCode, void* pvExceptionInfo, uint32 uBuildID) {
	return;
}

S_API void S_CALLTYPE SteamAPI_SetMiniDumpComment(const char *pchMsg) {
	return;
}

S_API void S_CALLTYPE SteamAPI_RunCallbacks() {
	return;
}

S_API void S_CALLTYPE SteamAPI_RegisterCallback(class CCallbackBase *pCallback, int iCallback) {
	return;
}

S_API void S_CALLTYPE SteamAPI_UnregisterCallback(class CCallbackBase *pCallback) {
	return;
}

S_API void S_CALLTYPE SteamAPI_RegisterCallResult(class CCallbackBase *pCallback, SteamAPICall_t hAPICall) {
	return;
}

S_API void S_CALLTYPE SteamAPI_UnregisterCallResult(class CCallbackBase *pCallback, SteamAPICall_t hAPICall) {
	return;
}

S_API bool S_CALLTYPE SteamAPI_IsSteamRunning() {
	return false;
}

S_API void Steam_RunCallbacks(HSteamPipe hSteamPipe, bool bGameServerCallbacks) {
	return;
}

S_API void Steam_RegisterInterfaceFuncs(void *hModule) {
	return;
}

S_API HSteamUser Steam_GetHSteamUserCurrent() {
	return 0;
}

S_API const char *SteamAPI_GetSteamInstallPath() {
	return nullptr;
}

S_API HSteamPipe SteamAPI_GetHSteamPipe() {
	return 0;
}

S_API void SteamAPI_SetTryCatchCallbacks(bool bTryCatchCallbacks) {
	return;
}

S_API HSteamPipe GetHSteamPipe() {
	return 0;
}

S_API HSteamUser GetHSteamUser() {
	return 0;
}

//steam_api_internal.h
S_API HSteamUser SteamAPI_GetHSteamUser() {
	return 0;
}

S_API void * S_CALLTYPE SteamInternal_ContextInit(void *pContextInitData) {
	return nullptr;
}

S_API void * S_CALLTYPE SteamInternal_CreateInterface(const char *ver) {
	return nullptr;
}
