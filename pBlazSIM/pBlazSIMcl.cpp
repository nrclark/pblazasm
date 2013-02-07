/*
 *  Copyright © 2013 : Maarten Brock <sourceforge.brock@dse.nl>
 *
 *  This file is part of pBlazSIMcl.
 *
 *  pBlazSIMcl is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pBlazSIMcl is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with pBlazASM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pBlaze.h"

class UARTDevice : public IODevice
{
public:
    virtual void setValue ( uint32_t address, uint32_t value )
    {
        putchar ( value ) ;
    }
    virtual uint32_t getValue ( uint32_t address )
    {
        return 0 ;
    }
} ;

void usage(void)
{
    fprintf(stderr, "pBlazSIMcl <filename[.lst]>\n");
}

bool isHexN(const char * Line, int nChars)
{
    int i;
    for (i=0; i<nChars; i++)
    {
        if (!isxdigit(Line[i]))
            return false;
    }
    return isspace(Line[i]);
}

int main(int argc, char *argv[])
{
    Picoblaze pBlaze;     // our simulated core
    FILE * file;

    if (argc != 2)
    {
        usage();
        return EXIT_FAILURE;
    }

    char * filename = (char *)malloc(strlen(argv[1]) + 8);
    strcpy(filename, argv[1]);
    char * dot = strrchr(filename, '.');
    if (!dot || strcmp(dot, ".lst") != 0)
        strcat(filename, ".lst");
    dot = strrchr(filename, '.');
    file = fopen(filename, "rt");
    if (!file)
    {
        fprintf(stderr, "Could not open %s\n", filename);
        return EXIT_FAILURE;
    }

    int row = 1;
    size_t linesize = 256;
    char * line = (char *)malloc(linesize);
    while (!feof(file))
    {
        long pos = ftell(file);
        fgets(line, linesize, file);
        while (strlen(line) > linesize - 2)
        {
            linesize *= 2;
            line = (char *)realloc(line, linesize);
            fseek(file, pos, SEEK_SET);
            fgets(line, linesize, file);
        }

        int address, code;
        if (strncmp(line, "PB3", 3) == 0)
            pBlaze.setCore(true);
        else if (strncmp(line, "PB6", 3) == 0)
            pBlaze.setCore(false);
        else if (isHexN(line, 3) && isHexN(&line[4], 5))
        {
            if (sscanf(line, "%3x %5x", &address, &code) == 2)
                pBlaze.setCodeItem(address, code, row++, NULL);
        }
    }
    fclose(file);

    dot[1] = 's';
    dot[2] = 'c';
    dot[3] = 'r';
    // read scratchpad data (.scr) assciated with .lst
    pBlaze.clearScratchpad();
    int addr = -1;
    file = fopen(filename, "rt");
    if (file)
    {
        while (!feof(file))
        {
            fgets(line, linesize, file);
            if (line[0] == '@')
            {
                sscanf(line, "@%x", &addr);
                if (addr >= MAXMEM)
                {
                    fprintf(stderr, "Out of scratchpad memory\n");
                    return EXIT_FAILURE;
                }
                addr &= 0xFF;
            }
            else
            {
                int value;
                sscanf(line, "%x", &value);
                pBlaze.setScratchpadData(addr++, value);
            }
        }
        fclose(file);
    }

    pBlaze.initPB6();
    pBlaze.setIODevice(NULL, 0xEC, 0xED, new UARTDevice());
    pBlaze.setBreakpoint(0);
    while (pBlaze.stepPB6() || pBlaze.onBreakpoint())
    {// run, forest, run
    }
    return EXIT_SUCCESS;
}
