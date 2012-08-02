/*
 *  Copyright © 2003..2012 : Henk van Kampen <henk@mediatronix.com>
 *
 *  This file is part of pBlazASM.
 *
 *  pBlazMRG is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  pBlazASM is distributed in the hope that it will be useful,
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
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include "pbTypes.h"
#include "pbPicoCore.h"

inline static uint32_t DestReg ( const int code )
{
    return ( code >> 8 ) & 0xF ;
}

inline static uint32_t SrcReg ( const int code )
{
    return ( code >> 4 ) & 0xF ;
}

inline static uint32_t Constant ( const int code )
{
    return code & 0xFF ;
}

inline static uint32_t Address12 ( const int code )
{
    return code & 0xFFF ;
}

bool writeVHD6 ( const char * strPSMfile, INST_t * Code, uint32_t * Data, int stack_size, int pad_size, int bank_size, int want_alu )
{
    FILE * outfile = NULL ;
    int  pc = 0 ;
    int i, j ;
    uint32_t c = 0 ;

    outfile = fopen ( strPSMfile, "w" ) ;
    if ( outfile == NULL ) {
        fprintf ( stderr, " ? Unable to open output file '%s'", strPSMfile ) ;
        return false ;
    }

    fprintf ( outfile,
              "\n"
              "library ieee; \n"
              "use ieee.std_logic_1164.all; \n"
              "use ieee.numeric_std.all; \n"

              "\n"
              "use work.pb2_pkg.all ; \n"
            ) ;

    fprintf ( outfile,
              "\n"
              "entity PicoCore is \n"
              "\tgeneric ( \n"
            ) ;
    fprintf ( outfile,
              "\t\tconstant STACK_SIZE : integer := '%d' ; \n"
              "\t\tconstant PAD_SIZE : integer := '%d' ; \n"
              "\t\tconstant BANK_SIZE : integer := '%d' ; \n"
              "\t\tconstant WANT_ALU : boolean := '%d' ; \n"
              stack_size, pad_size, bank_size, want_alu ) ;
    fprintf ( outfile,
              "\t ) \n"
              "\tport ( \n"
              "\t\tclk : in std_logic ; \n"
              "\t\trst : in std_logic ; \n"

              "\n"
              "\t\tPB2I : out t_PB2I ; \n"
              "\t\tPB2O : in t_PB2O ; \n"

              "\n"
              "\t\tPB2_IRQ : in std_logic ; \n"
              "\t\tPB2_IQA : out std_logic\n"
              "\t ) ; \n"
              "end PicoCore ; \n"
            ) ;

    fprintf ( outfile,
              "\n"
              "architecture mix of PicoCore is\n"
              "\t-- types\n"
              "\tsubtype PC_t is unsigned( 11 downto 0 ) ; \n"
              "\tsubtype SP_t is unsigned( 4 downto 0 ) ; \n"
              "\tsubtype IN_t is unsigned( 19 downto 0 ) ; \n"

              "\n"
              "\t-- state\n"
              "\tsignal pc : PC_t := ( others => '0' ) ; \n"
              "\tsignal c : std_logic := '0' ; \n"
              "\tsignal z : std_logic := '0' ; \n"
              "\tsignal i : std_logic := '0' ; \n"

              "\n"
              "\t-- stack \n"
              "\tsignal spW : SP_t := ( others => '0' ) ; \n"
              "\tsignal spR : SP_t := ( others => '1' ) ; \n"
              "\tsignal nspW : SP_t ; \n"
              "\tsignal nspR : SP_t ; \n"
              "\tsignal stackW : std_logic := '0' ; \n"
              "\tsignal stackT : std_logic_vector( 13 downto 0 ) ; \n"
              "\talias addrR : std_logic_vector is stackT( 11 downto 0 ) ; \n"

              "\n"
              "\t-- scratchpad\n"
              "\tsignal scrO : std_logic_vector( 7 downto 0 ) ; \n"
              "\tsignal scrW : std_logic := '0' ; \n"

              "\n"
              "\t-- data and address paths\n"
            ) ;
    fprintf ( outfile,
              "\tsignal inst : IN_t := X\"%05X\" ; \n", Code[ 0 ].code & 0x3FFFF
            ) ;
    fprintf ( outfile,
              "\talias addrN : unsigned is inst( 11 downto 0 ) ; \n"
              "\talias regA_addr : unsigned is inst( 11 downto 8 ) ; \n"
              "\talias regB_addr : unsigned is inst( 7 downto 4 ) ; \n"
              "\talias dataK : unsigned is inst( 7 downto 0 ) ; \n"

              "\tsignal regA : std_logic_vector( 7 downto 0 ) ; \n"
              "\talias dataA : std_logic_vector is regA( 7 downto 0 ) ; \n"
              "\tsignal regB : std_logic_vector( 7 downto 0 ) ; \n"
              "\tsignal dataB : std_logic_vector( 7 downto 0 ) ; \n"

              "\tsignal regI : std_logic_vector( 7 downto 0 ) ; \n"
              "\tsignal regW : std_logic := '0' ; \n"

              "\n"
              "\t-- next state \n"
              "\tsignal pc_1, npc : PC_t ; \n"
              "\tsignal nc, nz, ni : std_logic ; \n"
              "\tsignal nrst : std_logic ; \n"

              "\n"
              "\t-- io control \n"
              "\tsignal nioR : std_logic ; \n"
              "\tsignal nioW : std_logic ; \n"
              "\tsignal ioR : std_logic := '0' ; \n"
              "\tsignal ioW : std_logic := '0' ; \n"

              "\n"
              "\t-- alu \n"
              "\tsignal aluOP : std_logic_vector( 2 downto 0 ) ; \n"
              "\tsignal alu : std_logic_vector( 7 downto 0 ) ; \n"
              "\tsignal ci, co : std_logic ; \n"
            ) ;

    fprintf ( outfile,
              "\n"
              "\tfunction to_std_logic( l : boolean ) return std_ulogic is \n"
              "\tbegin \n"
              "\t\tif l then \n"
              "\t\t\treturn '1' ; \n"
              "\t\telse \n"
              "\t\t\treturn '0' ; \n"
              "\t\tend if ; \n"
              "\tend function to_std_logic ; \n"
            ) ;

    fprintf ( outfile,
              "\n"
              "\tfunction xor_reduce( arg : std_logic_vector ) return std_logic is \n"
              "\t\tvariable result : std_logic ; \n"
              "\tbegin \n"
              "\t\tresult := '0' ; \n"
              "\t\tfor i in arg'range loop \n"
              "\t\t\tresult := result xor arg( i ) ; \n"
              "\t\tend loop ; \n"
              "\t\treturn result ; \n"
              "\tend ; \n"
            ) ;

    fprintf ( outfile,
              "begin\n"
              "\t-- io connections\n"
              "\tPB2I.ck <= clk ; \n"
              "\tPB2I.rs <= rst ; \n"
              "\tPB2I.da <= dataA ; \n"
              "\tPB2I.ad <= dataB ; \n"

              "\tPB2I.wr <= ioW ; \n"
              "\tPB2I.rd <= ioR ; \n"
            ) ;

    fprintf ( outfile,
              "\n"
              "\t-- program counter\n"
              "\tpc_1 <= pc + 1 ; \n"
            ) ;

    fprintf ( outfile,
              "\n"
              "\t-- stack\n"
              "\tstack_b : if STACK_SIZE > 0 generate \n"
              "\t\ttype STACK_t is array( 0 to STACK_SIZE - 1 ) of std_logic_vector( 13 downto 0 ) ; \n"
              "\t\tsignal stack_ram : STACK_t ; \n"
              "\t\tsignal nstackT : std_logic_vector( 13 downto 0 ) ; \n"
              "\tbegin\n"
              "\t\tnstackT( 13 ) <= z ; \n"
              "\t\tnstackT( 12 ) <= c ; \n"
              "\t\tnstackT( 11 downto 0 ) <= std_logic_vector( pc + 1 ) ; \n"
              "\t\tprocess ( clk ) is \n"
              "\t\tbegin \n"
              "\t\t\tif rising_edge( clk ) then \n"
              "\t\t\t\tif stackW = '1' then \n"
              "\t\t\t\t\tstack_ram( to_integer( spW ) ) <= nstackT ; \n"
              "\t\t\t\tend if ; \n"
              "\t\t\tend if ; \n"
              "\t\tend process ; \n"
              "\n"
              "\t\tstackT <= stack_ram( to_integer( spR ) ) ; \n"
              "\tend generate ; \n"
            ) ;

    fprintf ( outfile,
              "\n"
              "\t-- registers\n"
              "\trb : if REG_SIZE > 0 is\n"
              "\t\ttype REG_t is array( 0 to BANK_SIZE - 1 ) of std_logic_vector( 7 downto 0 ) ; \n"
              "\t\tsignal register_ram : REG_t ; \n"
              "\tbegin\n"
              "\t\tprocess ( clk ) is\n"
              "\t\tbegin\n"
              "\t\t\tif rising_edge( clk ) then\n"
              "\t\t\t\tif regW = '1' then\n"
              "\t\t\t\t\tregister_ram( to_integer( regA_addr ) ) <= regI ; \n"
              "\t\t\t\tend if ; \n"
              "\t\tend if ; \n"
              "\t\tend process ; \n"

              "\n"
              "\t\tregA <= register_ram( to_integer( regA_addr ) ) ; \n"
              "\t\tregB <= register_ram( to_integer( regB_addr ) ) ; \n"
              "\tend block ; \n"
            ) ;

    fprintf ( outfile,
              "\n"
              "\t-- scratchpad\n"
              "\tsb : if PAD_SIZE > 0 generate \n"
              "\t\ttype SCRATCH_t is array( 0 to PAD_SIZE - 1 ) of std_logic_vector( 7 downto 0 ) ; \n"
              "\t\tsignal scratch_ram : SCRATCH_t := ( "
            ) ;
    fprintf ( outfile, "\n\t\t\t" ) ;
    for ( i = 0, j = 0 ; i < 256 ; i += 1 ) {
        if ( Data[ i ] != 0 ) {
            fprintf ( outfile, "%-3d => X\"%02X\", ", i, Data[ i ] ) ;
            j += 1 ;
        }
        if ( j > 7 ) {
            fprintf ( outfile, "\n\t\t\t" ) ;
            j = 0 ;
        }
    }
    fprintf ( outfile,
              "\n\t\t\tothers => X\"00\" \n"
              "\t\t ) ; \n"
            ) ;

    fprintf ( outfile,
              "\tbegin \n"
              "\t\tprocess ( clk ) is \n"
              "\t\tbegin \n"
              "\t\t\tif rising_edge( clk ) then \n"
              "\t\t\t\tif scrW = '1' then \n"
              "\t\t\t\t\tscratch_ram( to_integer( unsigned( dataB ) ) ) <= dataA ; \n"
              "\t\t\t\tend if ; \n"
              "\t\t\tend if ; \n"
              "\t\tend process ; \n"
              "\n"
              "\t\tscrO <= scratch_ram( to_integer( unsigned( dataB ) ) ) ; \n"
              "\tend generate ; \n"
              "\n"
            ) ;

    fprintf ( outfile,
              "\t-- alu\n"
              "\talu_b : if WANT_ALU is\n"
              "\t\tsignal mask : std_logic_vector( 7 downto 0 ) ; \n"
              "\t\tsignal sum : std_logic_vector( 7 downto 0 ) ; \n"
              "\t\tsignal chain : std_logic_vector( 8 downto 0 ) ; \n"
              "\tbegin\n"
              "\t\tchain( 0 ) <= ci ; \n"
              "\n"
              "\t\t-- Manchester carry chain\n"
              "\t\talu_g : for i in 0 to 7 generate\n"
              "\t\tbegin\n"
              "\t\t\tsum( i ) <=\n"
              "\t\t\t\tdataA( i ) xor dataB( i )        when aluOP = \"111\" else -- ADD, ADDC \n"
              "\t\t\t\tnot( dataA( i ) xor dataB( i ) ) when aluOP = \"110\" else -- SUB, SUBC, COMP, CMPC \n"
              "\t\t\t\tdataA( i ) xor dataB( i )        when aluOP = \"101\" else -- ADD, ADDC \n"
              "\t\t\t\tnot( dataA( i ) xor dataB( i ) ) when aluOP = \"100\" else -- SUB, SUBC, COMP, CMPC \n"
              "\n"
              "\t\t\t\tdataA( i ) xor dataB( i )        when aluOP = \"011\" else -- XOR \n"
              "\t\t\t\tdataA( i )  or dataB( i )        when aluOP = \"010\" else -- OR \n"
              "\t\t\t\tdataA( i ) and dataB( i )        when aluOP = \"001\" else -- AND \n"
              "\t\t\t\t               dataB( i )        when aluOP = \"000\" ;    -- MOVE \n"
              "\n"
              "\t\t\tmask( i ) <= dataA( i ) and aluOP( 2 ) ; \n"
              "\n"
              "\t\t\tchain( i + 1 ) <=\n"
              "\t\t\t\tchain( i ) when sum( i ) = '1' else\n"
              "\t\t\t\tmask( i ) ; \n"
              "\n"
              "\t\t\talu( i ) <= sum( i ) xor chain( i ) ; \n"
              "\t\tend generate ; \n"
              "\n"
              "\t\tco <= chain( 8 ) ; \n"
              "\tend block ; \n"
              "\n"
            ) ;

    fprintf ( outfile,
              "\t-- new state\n"
              "\tprocess ( rst, clk ) is \n"
              "\tbegin\n"

              "\t\tif rst = '1' then \n"
              "\t\t\tc <= '0' ; \n"
              "\t\t\tz <= '0' ; \n"
              "\t\t\ti <= '0' ; \n"
              "\t\t\tnrst <= '1' ; \n"
              "\t\t\tioR <= '0' ; \n"
              "\t\t\tioW <= '0' ; \n"

              "\n"
              "\t\t\tpc <= ( others => '0' ) ; \n"
              "\t\t\tspW <= ( others => '0' ) ; \n"
              "\t\t\tspR <= ( others => '1' ) ; \n"

              "\t\telsif rising_edge( clk ) then\n"
              "\t\t\tif nioR = '1' and ioR = '0' then \n"
              "\t\t\t\tioR <= '1' ; \n"
              "\t\t\telsif nioW = '1' and ioW = '0' then \n"
              "\t\t\t\tioW <= '1' ; \n"
              "\t\t\telsif nrst = '0' then \n"

              "\t\t\t\tc <= nc ; \n"
              "\t\t\t\tz <= nz ; \n"
              "\t\t\t\ti <= ni ; \n"

              "\n"
              "\t\t\t\tpc <= npc ; \n"
              "\t\t\t\tspW <= nspW ; \n"
              "\t\t\t\tspR <= nspR ; \n"

              "\t\t\t\tioR <= '0' ; \n"
              "\t\t\t\tioW <= '0' ; \n"
              "\t\t\tend if ; \n"
              "\t\t\tnrst <= '0' ; \n"
              "\t\tend if ; \n"
              "\tend process ; \n"
              "\n"
            ) ;

    fprintf ( outfile,
              "\t-- next state\n"
              "\tprocess ( PB2O, ioR, ioW, pc, pc_1, spW, spR, stackT, scrO, regI, dataA, regB, inst, dataB, alu, co, ci, c, z, i ) is\n"
              "\tbegin\n"
              "\t\t\tnpc <= pc_1 ; \n"
              "\t\t\tnspW <= spW ; \n"
              "\t\t\tnspR <= spR ; \n"

              "\t\t\tregI <= dataB ; \n"
              "\t\t\taluOP <= ( others => '0' ) ; \n"
              "\t\t\tci <= '0' ; \n"

              "\n"
              "\t\t\tnc <= c ; \n"
              "\t\t\tnz <= z ; \n"
              "\t\t\tni <= i ; \n"

              "\n"
              "\t\t\tnioR <= '0' ; \n"
              "\t\t\tnioW <= '0' ; \n"
              "\t\t\tscrW <= '0' ; \n"
              "\t\t\tregW <= '0' ; \n"
              "\t\t\tstackW <= '0' ; \n"
            ) ;

    fprintf ( outfile,
              "\n"
              "\t\t\t-- program\n"
              "\t\t\tcase pc is\n"
            ) ;

    for ( pc = 0 ; pc < MAXMEM ; pc += 1 ) {
        c = Code[ pc ].code & 0x3FFFF ;

        if ( c != 0 ) {
            fprintf ( outfile, "\t\t\twhen X\"%03X\" => \n", pc ) ;

            fprintf ( outfile, "\t\t\t\tinst <= X\"%05X\" ; \n", c ) ;
            if ( ( c & 0x1000 ) != 0 ) {
                fprintf ( outfile, "\t\t\t\tdataB <= std_logic_vector( dataK ) ; \n" ) ;
            } else {
                fprintf ( outfile, "\t\t\t\tdataB <= regB ; \n" ) ;
            }
        } else {
            continue ;
        }

        switch ( c ) {
        case 0x00000 :
            break ;
        case 0x00001 ... 0x00FFF :
            fprintf ( outfile, "\t\t\t\t-- MOVE\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\taluOP <= B\"000\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tregW <= '1' ; \n"
                    ) ;
            break ;
        case 0x01000 ... 0x01FFF :
            fprintf ( outfile, "\t\t\t\t-- MOVE\ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\taluOP <= B\"000\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tregW <= '1' ; \n"
                    ) ;
            break ;
            /*
                    case 0x16000 ... 0x16FFF :
                        fprintf ( outfile, "\t\t\t\t-- STAR\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
                        fprintf ( outfile, "\t\t\t\tassert false ; \n" ) ;
                        break ;
            */
        case 0x02000 ... 0x02FFF :
            fprintf ( outfile, "\t\t\t\t-- AND \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\taluOP <= B\"001\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                      "\t\t\t\tnc <= '0' ; \n"
                      "\t\t\t\tregW <= '1' ; \n"
                    ) ;
            break ;
        case 0x03000 ... 0x03FFF :
            fprintf ( outfile, "\t\t\t\t-- AND \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\taluOP <= B\"001\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                      "\t\t\t\tnc <= '0' ; \n"
                      "\t\t\t\tregW <= '1' ; \n"
                    ) ;
            break ;

        case 0x04000 ... 0x04FFF :
            fprintf ( outfile, "\t\t\t\t-- OR  \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\taluOP <= B\"010\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                      "\t\t\t\tnc <= '0' ; \n"
                      "\t\t\t\tregW <= '1' ; \n"
                    ) ;
            break ;
        case 0x05000 ... 0x05FFF :
            fprintf ( outfile, "\t\t\t\t-- OR  \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\taluOP <= B\"010\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                      "\t\t\t\tnc <= '0' ; \n"
                      "\t\t\t\tregW <= '1' ; \n"
                    ) ;
            break ;

        case 0x06000 ... 0x06FFF :
            fprintf ( outfile, "\t\t\t\t-- XOR \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\taluOP <= B\"011\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                      "\t\t\t\tnc <= '0' ; \n"
                      "\t\t\t\tregW <= '1' ; \n"
                    ) ;
            break ;
        case 0x07000 ... 0x07FFF :
            fprintf ( outfile, "\t\t\t\t-- XOR \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\taluOP <= B\"011\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                      "\t\t\t\tnc <= '0' ; \n"
                      "\t\t\t\tregW <= '1' ; \n"
                    ) ;
            break ;

        case 0x0C000 ... 0x0CFFF :
            fprintf ( outfile, "\t\t\t\t-- TEST\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\taluOP <= B\"001\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                      "\t\t\t\tnc <= xor_reduce( regI ) ; \n"
                    ) ;
            break ;
        case 0x0D000 ... 0x0DFFF :
            fprintf ( outfile, "\t\t\t\t-- TEST\ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\taluOP <= B\"001\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                      "\t\t\t\tnc <= xor_reduce( regI ) ; \n"
                    ) ;
            break ;
        case 0x0E000 ... 0x0EFFF :
            fprintf ( outfile, "\t\t\t\t-- TSTC\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\taluOP <= B\"001\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnz <= ( regI = X\"00\" ) and z ; \n"
                      "\t\t\t\tnc <= xor_reduce( regI ) xor c ; \n"
                    ) ;
            break ;
        case 0x0F000 ... 0x0FFFF :
            fprintf ( outfile, "\t\t\t\t-- TSTC\ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\taluOP <= B\"001\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnz <= ( regI = X\"00\" ) and z ; \n"
                      "\t\t\t\tnc <= xor_reduce( regI ) xor c ; \n"
                    ) ;
            break ;

        case 0x10000 ... 0x10FFF :
            fprintf ( outfile, "\t\t\t\t-- ADD \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tci <= '0' ; \n"
                      "\t\t\t\taluOP <= B\"101\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnc <= co ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                      "\t\t\t\tregW <= '1' ;"
                    ) ;
            break ;
        case 0x11000 ... 0x11FFF :
            fprintf ( outfile, "\t\t\t\t-- ADD \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tci <= '0' ; \n"
                      "\t\t\t\taluOP <= B\"101\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnc <= co ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                      "\t\t\t\tregW <= '1' ; \n"
                    ) ;
            break ;

        case 0x12000 ... 0x12FFF :
            fprintf ( outfile, "\t\t\t\t-- ADDC\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tci <= c ; \n"
                      "\t\t\t\taluOP <= B\"101\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnc <= co ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) and z ; \n"
                      "\t\t\t\tregW <= '1' ;"
                    ) ;
            break ;
        case 0x13000 ... 0x13FFF :
            fprintf ( outfile, "\t\t\t\t-- ADDC\ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tci <= c ; \n"
                      "\t\t\t\taluOP <= B\"101\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnc <= co ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) and z  ; \n"
                      "\t\t\t\tregW <= '1' ; \n"
                    ) ;
            break ;

        case 0x18000 ... 0x18FFF :
            fprintf ( outfile, "\t\t\t\t-- SUB \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tci <= '1' ; \n"
                      "\t\t\t\taluOP <= B\"100\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnc <= not co ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                      "\t\t\t\tregW <= '1' ;"
                    ) ;
            break ;
        case 0x19000 ... 0x19FFF :
            fprintf ( outfile, "\t\t\t\t-- SUB \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tci <= '1' ; \n"
                      "\t\t\t\taluOP <= B\"100\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnc <= not co ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                      "\t\t\t\tregW <= '1' ; \n"
                    ) ;
            break ;

        case 0x1A000 ... 0x1AFFF :
            fprintf ( outfile, "\t\t\t\t-- SUBC\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tci <= not c ; \n"
                      "\t\t\t\taluOP <= B\"100\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnc <= not co ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) and z ; \n"
                      "\t\t\t\tregW <= '1' ;"
                    ) ;
            break ;
        case 0x1B000 ... 0x1BFFF :
            fprintf ( outfile, "\t\t\t\t-- SUBC\ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tci <= not c ; \n"
                      "\t\t\t\taluOP <= B\"100\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnc <= not co ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) and z ; \n"
                      "\t\t\t\tregW <= '1' ; \n"
                    ) ;
            break ;

        case 0x1C000 ... 0x1CFFF :
            fprintf ( outfile, "\t\t\t\t-- COMP\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tci <= '1' ; \n"
                      "\t\t\t\taluOP <= B\"100\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnc <= not co ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                    ) ;
            break ;
        case 0x1D000 ... 0x1DFFF :
            fprintf ( outfile, "\t\t\t\t-- COMP\ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tci <= '1' ; \n"
                      "\t\t\t\taluOP <= B\"100\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnc <= not co ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                    ) ;
            break ;
        case 0x1E000 ... 0x1EFFF :
            fprintf ( outfile, "\t\t\t\t-- CMPC\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tci <= not c ; \n"
                      "\t\t\t\taluOP <= B\"100\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnc <= not co ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) and z ; \n"
                    ) ;
            break ;
        case 0x1F000 ... 0x1FFFF :
            fprintf ( outfile, "\t\t\t\t-- CMPC\ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tci <= not c ; \n"
                      "\t\t\t\taluOP <= B\"100\" ; \n"
                      "\t\t\t\tregI <= alu ; \n"
                      "\t\t\t\tnc <= not co ; \n"
                      "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) and z ; \n"
                    ) ;
            break ;

        case 0x14000 ... 0x14FFF :
            if ( ( c & 0xF0 ) == 0x80 ) {
                fprintf ( outfile, "\t\t\t\t-- CORE\ts%X   \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                fprintf ( outfile,
                          "\t\t\t\tregI <= X\"42\" ; \n"
                          "\t\t\t\tnc <= '1' ; \n"
                          "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                          "\t\t\t\tregW <= '1' ; \n"
                        ) ;
            } else
                switch ( c & 0xF ) {
                case 0x2 :
                    fprintf ( outfile, "\t\t\t\t-- RL  \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tregI <= dataA( 6 downto 0 ) & dataA( 7 ) ; \n"
                              "\t\t\t\tnc <= dataA( 7 ) ; \n"
                              "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                              "\t\t\t\tregW <= '1' ; \n"
                            ) ;
                    break ;
                case 0x6 :
                    fprintf ( outfile, "\t\t\t\t-- SL0 \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tregI <= dataA( 6 downto 0 ) & '0' ; \n"
                              "\t\t\t\tnc <= dataA( 7 ) ; \n"
                              "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                              "\t\t\t\tregW <= '1' ; \n"
                            ) ;
                    break ;
                case 0x7 :
                    fprintf ( outfile, "\t\t\t\t-- SL1 \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tregI <= dataA( 6 downto 0 ) & '1' ; \n"
                              "\t\t\t\tnc <= dataA( 7 ) ; \n"
                              "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                              "\t\t\t\tregW <= '1' ; \n"
                            ) ;
                    break ;
                case 0x0 :
                    fprintf ( outfile, "\t\t\t\t-- SLA \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tregI <= dataA( 6 downto 0 ) & c ; \n"
                              "\t\t\t\tnc <= dataA( 7 ) ; \n"
                              "\t\t\t\tregW <= '1' ; \n"
                            ) ;
                    break ;
                case 0x4 :
                    fprintf ( outfile, "\t\t\t\t-- SLX \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tregI <= dataA( 6 downto 0 ) & dataA( 0 ) ; \n"
                              "\t\t\t\tnc <= dataA( 7 ) ; \n"
                              "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                              "\t\t\t\tregW <= '1' ; \n"
                            ) ;
                    break ;

                case 0xC :
                    fprintf ( outfile, "\t\t\t\t-- RR  \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tregI <= dataA( 0 ) & dataA( 7 downto 1 ) ; \n"
                              "\t\t\t\tnc <= dataA( 0 ) ; \n"
                              "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                              "\t\t\t\tregW <= '1' ; \n"
                            ) ;
                    break ;
                case 0xE :
                    fprintf ( outfile, "\t\t\t\t-- SR0 \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tregI <= '0' & dataA( 7 downto 1 ) ; \n"
                              "\t\t\t\tnc <= dataA( 0 ) ; \n"
                              "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                              "\t\t\t\tregW <= '1' ; \n"
                            ) ;
                    break ;
                case 0xF :
                    fprintf ( outfile, "\t\t\t\t-- SR1 \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tregI <= '1' & dataA( 7 downto 1 ) ; \n"
                              "\t\t\t\tnc <= dataA( 0 ) ; \n"
                              "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                              "\t\t\t\tregW <= '1' ; \n"
                            ) ;
                    break ;
                case 0x8 :
                    fprintf ( outfile, "\t\t\t\t-- SRA \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tregI <= c & dataA( 7 downto 1 ) ; \n"
                              "\t\t\t\tnc <= dataA( 0 ) ; \n"
                              "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                              "\t\t\t\tregW <= '1' ; \n"
                            ) ;
                    break ;
                case 0xA :
                    fprintf ( outfile, "\t\t\t\t-- SRX \ts%X      \t; %03X : %05X \n", DestReg ( c ), pc, c ) ;
                    fprintf ( outfile,
                              "\t\t\t\tregI <= dataA( 7 ) & dataA( 7 downto 1 ) ; \n"
                              "\t\t\t\tnc <= dataA( 0 ) ; \n"
                              "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
                              "\t\t\t\tregW <= '1' ; \n"
                            ) ;
                    break ;

                default :
                    fprintf ( outfile, "\t\t\t\t-- INST\t0x%05X\t; %03X : %05X \n", c, pc, c ) ;
                    fprintf ( outfile, "\t\t\t\tassert false ; \n" ) ;
                }
            break ;

        case 0x22000 ... 0x22FFF :
            fprintf ( outfile, "\t\t\t\t-- JUMP\t0x%03X    \t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tnpc <= addrN ; \n" ) ;
            break ;
        case 0x32000 ... 0x32FFF :
            fprintf ( outfile, "\t\t\t\t-- JUMP\tZ, 0x%03X \t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif z = '1' then \n"
                      "\t\t\t\t\tnpc <= addrN ; \n"
                      "\t\t\t\tend if ; \n"
                    ) ;
            break ;
        case 0x36000 ... 0x36FFF :
            fprintf ( outfile, "\t\t\t\t-- JUMP\tNZ, 0x%03X\t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif z = '0' then \n"
                      "\t\t\t\t\tnpc <= addrN ; \n"
                      "\t\t\t\tend if ; \n"
                    ) ;
            break ;
        case 0x3A000 ... 0x3AFFF :
            fprintf ( outfile, "\t\t\t\t-- JUMP\tC, 0x%03X \t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif c = '1' then \n"
                      "\t\t\t\t\tnpc <= addrN ; \n"
                      "\t\t\t\tend if ; \n"
                    ) ;
            break ;
        case 0x3E000 ... 0x3EFFF :
            fprintf ( outfile, "\t\t\t\t-- JUMP\tNC, 0x%03X\t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif c = '0' then \n"
                      "\t\t\t\t\tnpc <= addrN ; \n"
                      "\t\t\t\tend if ; \n"
                    ) ;
            break ;
        case 0x26000 ... 0x26FFF :
            fprintf ( outfile, "\t\t\t\t-- JUMP\ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tnpc( 11 downto 8 ) <= unsigned( dataA( 3 downto 0 ) ) ; \n"
                      "\t\t\t\tnpc( 7 downto 0 ) <= unsigned( regB ) ; \n"
                    ) ;
            break ;

        case 0x20000 ... 0x20FFF :
            fprintf ( outfile, "\t\t\t\t-- CALL\t0x%03X    \t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tnpc <= addrN ; \n"

                      "\t\t\t\tnspW <= spW + 1 ; \n"
                      "\t\t\t\tnspR <= spW ; \n"
                      "\t\t\t\tstackW <= '1' ; \n"
                    ) ;
            break ;
        case 0x30000 ... 0x30FFF :
            fprintf ( outfile, "\t\t\t\t-- CALL\tZ, 0x%03X \t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif z = '1' then \n"
                      "\t\t\t\t\tnpc <= addrN ; \n"

                      "\t\t\t\t\tnspW <= spW + 1 ; \n"
                      "\t\t\t\t\tnspR <= spW ; \n"
                      "\t\t\t\t\tstackW <= '1' ; \n"
                      "\t\t\t\tend if ; \n"
                    ) ;
            break ;
        case 0x34000 ... 0x34FFF :
            fprintf ( outfile, "\t\t\t\t-- CALL\tNZ, 0x%03X\t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif z = '0' then \n"
                      "\t\t\t\t\tnpc <= addrN ; \n"

                      "\t\t\t\t\tnspW <= spW + 1 ; \n"
                      "\t\t\t\t\tnspR <= spW ; \n"
                      "\t\t\t\t\tstackW <= '1' ; \n"
                      "\t\t\t\tend if ; \n"
                    ) ;
            break ;
        case 0x38000 ... 0x38FFF :
            fprintf ( outfile, "\t\t\t\t-- CALL\tC, 0x%03X \t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif c = '1' then \n"
                      "\t\t\t\t\tnpc <= addrN ; \n"

                      "\t\t\t\t\tnspW <= spW + 1 ; \n"
                      "\t\t\t\t\tnspR <= spW ; \n"
                      "\t\t\t\t\tstackW <= '1' ; \n"
                      "\t\t\t\tend if ; \n"
                    ) ;
            break ;
        case 0x3C000 ... 0x3CFFF :
            fprintf ( outfile, "\t\t\t\t-- CALL\tNC, 0x%03X\t; %03X : %05X \n", Address12 ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif c = '0' then \n"
                      "\t\t\t\t\tnpc <= addrN ; \n"

                      "\t\t\t\t\tnspW <= spW + 1 ; \n"
                      "\t\t\t\t\tnspR <= spW ; \n"
                      "\t\t\t\t\tstackW <= '1' ; \n"
                      "\t\t\t\tend if ; \n"
                    ) ;
            break ;
        case 0x24000 ... 0x24FFF :
            fprintf ( outfile, "\t\t\t\t-- CALL\ts%X, s%X  \t; %03X : %05X  \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tnpc( 11 downto 8 ) <= unsigned( dataA( 3 downto 0 ) ) ; \n"
                      "\t\t\t\tnpc( 7 downto 0 ) <= unsigned( regB ) ; \n"

                      "\t\t\t\tnspW <= spW + 1 ; \n"
                      "\t\t\t\tnspR <= spW ; \n"
                      "\t\t\t\tstackW <= '1' ; \n"
                    ) ;
            break ;

        case 0x25000 ... 0x25FFF :
            fprintf ( outfile, "\t\t\t\t-- RET \t         \t; %03X : %05X \n", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tnpc <= unsigned( addrR ) ; \n"

                      "\t\t\t\tnspW <= spR ; \n"
                      "\t\t\t\tnspR <= spR - 1 ; \n"
                    ) ;
            break ;
        case 0x31000 ... 0x31FFF :
            fprintf ( outfile, "\t\t\t\t-- RET \t Z        \t; %03X : %05X \n", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif z = '1' then \n"
                      "\t\t\t\t\tnpc <= unsigned( addrR ) ; \n"

                      "\t\t\t\t\tnspW <= spR ; \n"
                      "\t\t\t\t\tnspR <= spR - 1 ; \n"
                      "\t\t\t\tend if ; \n"
                    ) ;
            break ;
        case 0x35000 ... 0x35FFF :
            fprintf ( outfile, "\t\t\t\t-- RET \t NZ       \t; %03X : %05X \n", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif z = '0' then \n"
                      "\t\t\t\t\tnpc <= unsigned( addrR ) ; \n"

                      "\t\t\t\t\tnspW <= spR ; \n"
                      "\t\t\t\t\tnspR <= spR - 1 ; \n"
                      "\t\t\t\tend if ; \n"
                    ) ;
            break ;
        case 0x39000 ... 0x39FFF :
            fprintf ( outfile, "\t\t\t\t-- RET \t C        \t; %03X : %05X \n", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif c = '1' then \n"
                      "\t\t\t\t\tnpc <= unsigned( addrR ) ; \n"

                      "\t\t\t\t\tnspW <= spR ; \n"
                      "\t\t\t\t\tnspR <= spR - 1 ; \n"
                      "\t\t\t\tend if ; \n"
                    ) ;
            break ;
        case 0x3D000 ... 0x3DFFF :
            fprintf ( outfile, "\t\t\t\t-- RET \t NC       \t; %03X : %05X \n", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tif c = '0' then \n"
                      "\t\t\t\t\tnpc <= unsigned( addrR ) ; \n"

                      "\t\t\t\t\tnspW <= spR ; \n"
                      "\t\t\t\t\tnspR <= spR - 1 ; \n"
                      "\t\t\t\tend if ; \n"
                    ) ;
            break ;
        case 0x21000 ... 0x21FFF :
            fprintf ( outfile, "\t\t\t\t-- RET \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tnpc <= unsigned( addrR ) ; \n"

                      "\t\t\t\tnspW <= spR ; \n"
                      "\t\t\t\tnspR <= spR - 1 ; \n"
                      "\t\t\t\tregI <= inst( 7 downto 0 ) ; \n"
                    ) ;
            break ;


        case 0x2E000 ... 0x2EFFF :
            fprintf ( outfile, "\t\t\t\t-- ST  \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tscrW <= '1' ; \n"
                    ) ;
            break ;
        case 0x2F000 ... 0x2FFFF :
            fprintf ( outfile, "\t\t\t\t-- ST  \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tscrW <= '1' ; \n"
                    ) ;
            break ;

        case 0x0A000 ... 0x0AFFF :
            fprintf ( outfile, "\t\t\t\t-- LD  \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tregI <= scrO ; \n"
                      "\t\t\t\tregW <= '1' ; \n"
                    ) ;
            break ;
        case 0x0B000 ... 0x0BFFF :
            fprintf ( outfile, "\t\t\t\t-- LD  \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tregI <= scrO ; \n"
                      "\t\t\t\tregW <= '1' ; \n"
                    ) ;
            break ;

        case 0x2C000 ... 0x2CFFF :
            fprintf ( outfile, "\t\t\t\t-- OUT \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tnioW <= '1' ; \n"
                    ) ;
            break ;
        case 0x2D000 ... 0x2DFFF :
            fprintf ( outfile, "\t\t\t\t-- OUT \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tnioW <= '1' ; \n"
                    ) ;
            break ;
            /*
                    case 0x2B000 ... 0x2BFFF :
                        fprintf ( outfile, "\t\t\t\t-- OUTK\t0x%02X, 0x%X\t; %03X : %05X \n", ( c >> 4 ) & 0xFF, c & 0xF, pc, c ) ;
                        fprintf ( outfile,
                                  "\t\t\t\nwr <= '1' ; \n" ) ;
                        break ;
            */

        case 0x08000 ... 0x08FFF :
            fprintf ( outfile, "\t\t\t\t-- IN  \ts%X, s%X  \t; %03X : %05X \n", DestReg ( c ), SrcReg ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tnioR <= '1' ; \n"
                      "\t\t\t\tregI <= PB2O.da ; \n"
                      "\t\t\t\tregW <= ioR ; \n"
                    ) ;
            break ;
        case 0x09000 ... 0x09FFF :
            fprintf ( outfile, "\t\t\t\t-- IN  \ts%X, 0x%02X\t; %03X : %05X \n", DestReg ( c ), Constant ( c ), pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tnioR <= '1' ; \n"
                      "\t\t\t\tregI <= PB2O.da ; \n"
                      "\t\t\t\tregW <= ioR ; \n"
                    ) ;
            break ;

        case 0x28000 :
            fprintf ( outfile, "\t\t\t\t-- DINT\t \t; %03X : %05X \n", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tni <= inst( 0 ) ; \n"
                    ) ;
            break ;
        case 0x28001 :
            fprintf ( outfile, "\t\t\t\t-- EINT\t \t; %03X : %05X \n", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tni <= inst( 0 ) ; \n"
                    ) ;
            break ;
        case 0x29000 :
            fprintf ( outfile, "\t\t\t\t-- RETI\tDISABLE\t; %03X : %05X \n", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tni <= inst( 0 ) ; \n"
                      "\t\t\t\tnpc <= unsigned( addrR ) ; \n"

                      "\t\t\t\tnc <= stackT( 12 ) ; \n"
                      "\t\t\t\tnz <= stackT( 13 ) ; \n"

                      "\t\t\t\tnspW <= spR ; \n"
                      "\t\t\t\tnspR <= spR - 1 ; \n"
                    ) ;
            break ;
        case 0x29001 :
            fprintf ( outfile, "\t\t\t\t-- RETI\tENABLE\t; %03X : %05X \n", pc, c ) ;
            fprintf ( outfile,
                      "\t\t\t\tni <= inst( 0 ) ; \n"
                      "\t\t\t\tnpc <= unsigned( addrR ) ; \n"

                      "\t\t\t\tnc <= stackT( 12 ) ; \n"
                      "\t\t\t\tnz <= stackT( 13 ) ; \n"

                      "\t\t\t\tnspW <= spR ; \n"
                      "\t\t\t\tnspR <= spR - 1 ; \n"
                    ) ;
            break ;

            /*
                    case 0x37000 :
                        fprintf ( outfile, "\t\t\t\t-- BANK\tA\t; %03X : %05X \n", pc, c ) ;
                        fprintf ( outfile, "\t\t\t\tassert false ; \n" ) ;
                        break ;
                    case 0x37001 :
                        fprintf ( outfile, "\t\t\t\t-- BANK\tB\t; %03X : %05X \n", pc, c ) ;
                        fprintf ( outfile, "\t\t\t\tassert false ; \n" ) ;
                        break ;
            */
        default :
            fprintf ( outfile, "\t\t\t\t-- INST\t0x%05X\t; %03X : %05X \n", pc, c, c ) ;
        }
    }

// postamble
    fprintf ( outfile,
              "\t\t\twhen others =>\n"
              "\t\t\t\tinst <= ( others => '0' ) ; \n"
              "\t\t\t\tdataB <= regB ; \n"
            ) ;

    fprintf ( outfile,
              "\t\t\tend case ; \n"
              "\n"
              "\tend process ; \n"
              "end mix ; \n"
            ) ;

    fclose ( outfile ) ;
    return true ;
}
