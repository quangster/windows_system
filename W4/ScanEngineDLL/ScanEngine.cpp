#include "pch.h"
#include "ScanEngine.h"
#include <windows.h>
#include <stdio.h>

int CheckFilePathSafety(const char* filePath)
{
	char actualPath[MAX_PATH];

	if (filePath == nullptr || strlen(filePath) == 0)
	{
		return RESULT_ERROR;
	}
	strcpy_s(actualPath, MAX_PATH, filePath);

	DWORD attr = GetFileAttributesA(actualPath);
	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		return RESULT_ERROR;
	}

	char rootPath[4];
	if (strlen(actualPath) > 3 && actualPath[1] == ':')
	{
		rootPath[0] = actualPath[0];
		rootPath[1] = actualPath[1];
		rootPath[2] = '\\';
		rootPath[3] = '\0';
	}
	else
	{
		return RESULT_ERROR;
	}

	UINT driveType = GetDriveTypeA(rootPath);

	if (driveType == DRIVE_FIXED)
	{
		return RESULT_SAFE;
	}
	else if (driveType == DRIVE_REMOTE)
	{
		return RESULT_WARNING;
	}
	else if (driveType == DRIVE_REMOVABLE)
	{
		return RESULT_DANGER;
	}
	return RESULT_DANGER;
}