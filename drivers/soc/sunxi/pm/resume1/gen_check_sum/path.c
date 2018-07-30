/*
 **********************************************************************************************************************
 *
 *						           the Embedded Secure Bootloader System
 *
 *
 *						       Copyright(C), 2006-2014, Allwinnertech Co., Ltd.
 *                                           All Rights Reserved
 *
 * File    :
 *
 * By      :
 *
 * Version : V2.00
 *
 * Date	  :
 *
 * Descript:
 **********************************************************************************************************************
 */
#include "common.h"
#include <ctype.h>
#include <unistd.h>

static int __IsFullName(const char *FilePath)
{
	if (FilePath[0] == '/')
		return 1;
	else
		return 0;
}

void GetFullPath(char *dName, const char *sName)
{
	char Buffer[MAX_PATH];

	if (__IsFullName(sName)) {
		strcpy(dName, sName);
		return ;
	}

	/* Get the current working directory: */
	if (getcwd(Buffer, MAX_PATH) == NULL) {
		perror("getcwd error");
		return ;
	}
	sprintf(dName, "%s/%s", Buffer, sName);
}


