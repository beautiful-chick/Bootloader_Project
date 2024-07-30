/*********************************************************************************
 *      Copyright:  (C) 2024 Avnet. All Rights Reserved.
 *         Author:  Wenxue Guo <wenxue.guo@avnet.com>
 *
 *    Description:  This file is dump buffer content function API
 *
 ********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dump.h"

#define dump_print(format,args...) xil_printf(format, ##args)

void dump_buf(const char *prompt, const unsigned char *buffer, int length)
{
    size_t i, j;

    if( prompt )
    	dump_print(prompt);

    for (i = 0; i < length; i += 16) {
    	dump_print("%08x: ", i);

        // Print hex representation
        for (j = 0; j < 16; j++) {
            if (i + j < length)
            	dump_print("%02x ", buffer[i + j]);
            else
            	dump_print("   "); // if end of buffer reached, print spaces
        }

        dump_print(" ");

        // Print ASCII representation
        for (j = 0; j < 16; j++) {
            if (i + j < length) {
                unsigned char c = buffer[i + j];
                dump_print("%c", (c >= 32 && c <= 126) ? c : '.');
            } else {
            	dump_print(" ");
            }
        }

        dump_print("\r\n");
    }
}

