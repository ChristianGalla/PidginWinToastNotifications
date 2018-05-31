#pragma once

#ifdef _WIN32
#ifdef PIDGINWINTOASTLIB_EXPORTS
#define PIDGINWINTOASTLIB_API __declspec(dllexport)
#else
#define PIDGINWINTOASTLIB_API __declspec(dllimport)
#endif
#else
#define PIDGINWINTOASTLIB_API
#endif

#ifdef __cplusplus
extern "C" {
#endif
	int PIDGINWINTOASTLIB_API pidginWinToastLibInit(void(*clickCallback)(void *conv) = NULL);
#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus
extern "C" {
#endif
	int PIDGINWINTOASTLIB_API pidginWinToastLibShowMessage(const char * sender, const char * message, const char * imagePath = NULL, const char * protocolName = NULL, void *conv = NULL);
#ifdef __cplusplus
} // extern "C"
#endif