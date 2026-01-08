#pragma once

#ifdef SAFETYFILEDLL_EXPORTS
#define SAFETYFILE_API __declspec(dllexport)
#else
#define SAFETYFILE_API __declspec(dllimport)
#endif

#define RESULT_SAFE 0
#define RESULT_WARNING 1
#define RESULT_DANGER 2
#define RESULT_ERROR -1

extern "C" SAFETYFILE_API int CheckFilePathSafety(const char* filePath);