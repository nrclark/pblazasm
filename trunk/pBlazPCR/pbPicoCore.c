/*
 *  Copyright � 2003..2012 : Henk van Kampen <henk@mediatronix.com>
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

bool writeVHD6 ( const char * strPSMfile, INST_t * Code, uint32_t * Data, uint64_t inst_map, int code_size, int stack_size, int pad_size, int bank_size, bool want_alu ) {
	FILE * outfile = NULL ;
//    int  pc = 0 ;
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
	          "\t\tconstant CODE_SIZE : integer := %d ; \n"
	          "\t\tconstant STACK_SIZE : integer := %d ; \n"
	          "\t\tconstant PAD_SIZE : integer := %d ; \n"
	          "\t\tconstant BANK_SIZE : integer := %d ; \n"
	          "\t\tconstant WANT_ALU : boolean := %s \n"
	          , code_size, stack_size, pad_size, bank_size, want_alu ? "true" : "false" ) ;
	fprintf ( outfile,
	          "\t ) ; \n"
	          "\tport ( \n"
	          "\t\tclk : in std_logic ; \n"
	          "\t\trst : in std_logic ; \n"

	          "\n"
	          "\t\tPB2I : out t_PB2I ; \n"
	          "\t\tPB2O : in t_PB2O ; \n"

	          "\n"
	          "\t\tPB2_IRQ : in std_logic ; \n"
	          "\t\tPB2_IQA : out std_logic \n"
	          "\t ) ; \n"
	          "end PicoCore ; \n"
	        ) ;

	fprintf ( outfile,
	          "\n"
	          "architecture mix of PicoCore is\n"
	          "\t-- types\n"
	          "\tsubtype PC_t is unsigned( 11 downto 0 ) ; \n"
	          "\tsubtype SP_t is integer range -1 to STACK_SIZE - 1 ; \n"
	          "\tsubtype IN_t is unsigned( 17 downto 0 ) ; \n"

	          "\n"
	          "\t-- state\n"
	          "\tsignal pc : PC_t := ( others => '0' ) ; \n"
	          "\tsignal c : std_logic := '0' ; \n"
	          "\tsignal z : std_logic := '0' ; \n"
	          "\tsignal i : std_logic := '0' ; \n"

	          "\n"
	          "\t-- stack \n"
	          "\tsignal spW : SP_t := 0 ; \n"
	          "\tsignal spR : SP_t := -1 ; \n"
	          "\tsignal nspW : SP_t ; \n"
	          "\tsignal nspR : SP_t ; \n"
	          "\tsignal stackW : std_logic := '0' ; \n"
	          "\tsignal stackT : std_logic_vector( 13 downto 0 ) ; \n"
	          "\talias addrR : std_logic_vector( 11 downto 0 ) is stackT( 11 downto 0 ) ; \n"

	          "\n"
	          "\t-- scratchpad\n"
	          "\tsignal scrO : std_logic_vector( 7 downto 0 ) ; \n"
	          "\tsignal scrW : std_logic := '0' ; \n"

	          "\n"
	          "\t-- data and address paths\n"
	        ) ;
	fprintf ( outfile,
	          "\tsignal inst : IN_t := O\"%06o\" ; \n", Code[ 0 ].code & 0x3FFFF
	        ) ;
	fprintf ( outfile,
	          "\talias addrN : unsigned( 11 downto 0 ) is inst( 11 downto 0 ) ; \n"
	          "\talias regA_addr : unsigned( 3 downto 0 ) is inst( 11 downto 8 ) ; \n"
	          "\talias regB_addr : unsigned( 3 downto 0 ) is inst( 7 downto 4 ) ; \n"
	          "\talias dataK : unsigned( 7 downto 0 ) is inst( 7 downto 0 ) ; \n"

	          "\tsignal regA : std_logic_vector( 7 downto 0 ) := ( others => '0' ) ; \n"
	          "\talias dataA : std_logic_vector( 7 downto 0 ) is regA( 7 downto 0 ) ; \n"
	          "\tsignal regB : std_logic_vector( 7 downto 0 ) := ( others => '0' ) ; \n"
	          "\tsignal dataB : std_logic_vector( 7 downto 0 ) := ( others => '0' ) ; \n"

	          "\tsignal regI : std_logic_vector( 7 downto 0 ) := ( others => '0' ) ; \n"
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
	          "\ttype aluOP_t is ( opMOVE, opAND, opOR, opXOR, opXNOR ) ; \n"
	          "\tsignal aluOP : aluOP_t := opMOVE ; \n"
	          "\tsignal alu : std_logic_vector( 7 downto 0 ) := ( others => '0' ) ; \n"
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

	          "\tPB2_IQA <= '0' ; \n"
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
	          "\t\ttype STACK_t is array( -1 to STACK_SIZE - 1 ) of std_logic_vector( 13 downto 0 ) ; \n"
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
	          "\t\t\t\t\tstack_ram( spW ) <= nstackT ; \n"
	          "\t\t\t\tend if ; \n"
	          "\t\t\tend if ; \n"
	          "\t\tend process ; \n"
	          "\n"
	          "\t\tstackT <= stack_ram( spR ) ; \n"
	          "\tend generate ; \n"
	        ) ;

	fprintf ( outfile,
	          "\n"
	          "\t-- registers\n"
	          "\trb : if BANK_SIZE > 0 generate \n"
	          "\t\ttype REG_t is array( 0 to BANK_SIZE - 1 ) of std_logic_vector( 7 downto 0 ) ; \n"
	          "\t\tsignal register_ram : REG_t := ( others => X\"00\" ) ; \n"
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
	          "\tend generate ; \n"
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
	          "\n\t\t\tothers => ( others => '0' ) \n"
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
	          "\n"
	          "\t-- code rom\n"
	          "\tcb : if CODE_SIZE > 0 generate \n"
	          "\t\ttype CODE_t is array( 0 to CODE_SIZE - 1 ) of IN_t ; \n"
	          "\t\tsignal code_rom : CODE_t := ( "
	        ) ;

	fprintf ( outfile, "\n\t\t\t" ) ;

	for ( i = 0, j = 0 ; i < 4096 ; i += 1 ) {
		if ( Code[ i ].code != 0 ) {
			fprintf ( outfile, "%-4d => O\"%06o\", ", i, Code[ i ].code ) ;
			j += 1 ;
		}
		if ( j > 7 ) {
			fprintf ( outfile, "\n\t\t\t" ) ;
			j = 0 ;
		}
	}

	fprintf ( outfile,
	          "\n\t\t\tothers => ( others => '0' ) \n"
	          "\t\t ) ; \n"
	        ) ;

	fprintf ( outfile,
	          "\tbegin \n"
	          "\t\tinst <= code_rom( to_integer( pc ) ) ; \n"
	          "\tend generate ; \n"
	          "\n"
	        ) ;

	fprintf ( outfile,
	          "\t-- alu\n"
	          "\talu_b : if WANT_ALU generate \n"
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
	          "\t\t\t\tdataA( i ) xor dataB( i )        when aluOP = opXOR else  -- ADD, ADDC, XOR \n"
	          "\t\t\t\tnot( dataA( i ) xor dataB( i ) ) when aluOP = opXNOR else -- SUB, SUBC, COMP, CMPC \n"
	          "\t\t\t\tdataA( i )  or dataB( i )        when aluOP = opOR else   -- OR \n"
	          "\t\t\t\tdataA( i ) and dataB( i )        when aluOP = opAND else  -- AND, TEST, TSTC \n"
	          "\t\t\t\t               dataB( i )        when aluOP = opMOVE ;    -- MOVE \n"
	          "\n"
	          "\t\t\tmask( i ) <= dataA( i ) when aluOP = opXOR or aluOP = opXNOR else '0' ; \n"
	          "\n"
	          "\t\t\tchain( i + 1 ) <=\n"
	          "\t\t\t\tchain( i ) when sum( i ) = '1' else\n"
	          "\t\t\t\tmask( i ) ; \n"
	          "\n"
	          "\t\t\talu( i ) <= sum( i ) xor chain( i ) ; \n"
	          "\t\tend generate ; \n"
	          "\n"
	          "\t\tco <= chain( 8 ) ; \n"
	          "\tend generate ; \n"
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
	          "\t\t\tspW <= 0 ; \n"
	          "\t\t\tspR <= -1 ; \n"

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
//	          "\t\variable varI : std_logic_vector( 7 downto 0 ) ; \n"
	          "\tbegin\n"
	          "\t\t\tnpc <= pc_1 ; \n"
	          "\t\t\tnspW <= spW ; \n"
	          "\t\t\tnspR <= spR ; \n"

	          "\t\t\tregI <= dataB ; \n"
	          "\t\t\taluOP <= opXOR ; \n"
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

	fprintf ( outfile, "\n"
	          "\t\t\tif inst( 12 ) = '1' then \n"
	          "\t\t\t\tdataB <= std_logic_vector( dataK ) ; \n"
	          "\t\t\telse \n"
	          "\t\t\t\tdataB <= regB ; \n"
	          "\t\t\tend if ; \n"
	        ) ;

	fprintf ( outfile,
	          "\n"
	          "\t\t\tcase inst( 17 downto 12 ) is \n"
	        ) ;

	fprintf ( outfile,
	          "\t\t\t-- inst_map: %016I64X \n",
	          inst_map );

	fprintf ( outfile, "\n\t\t\t-- MOVE\ts%X, s%X  \t; \n", DestReg ( 1 ), SrcReg ( 2 ) ) ;
	fprintf ( outfile, "\t\t\t-- MOVE\ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x00 ) ) | ( inst_map & ( 1LLU << 0x01 ) ) ) {
		fprintf ( outfile, "\t\twhen B\"00_0000\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"00_0001\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\taluOP <= opMOVE ; \n"
		          "\t\t\tregI <= alu ; \n"
		          "\t\t\tregW <= '1' ; \n"
		        ) ;
	}
	/*
	        case 0x16000 ... 0x16FFF :
	            fprintf ( outfile, "\t\t\t\t-- STAR\ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	            fprintf ( outfile, "\t\t\t\tassert false ; \n" ) ;
	            break ;
	*/
	fprintf ( outfile, "\n\t\t\t-- AND \ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	fprintf ( outfile, "\t\t\t-- AND \ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x02 ) ) | ( inst_map & ( 1LLU << 0x03 ) ) ) {
		fprintf ( outfile, "\t\twhen B\"00_0010\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"00_0011\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\taluOP <= opAND ; \n"
		          "\t\t\tregI <= alu ; \n"
		          "\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\tnc <= '0' ; \n"
		          "\t\t\tregW <= '1' ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- OR  \ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	fprintf ( outfile, "\t\t\t-- OR  \ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x04 ) ) | ( inst_map & ( 1LLU << 0x05 ) ) ) {
		fprintf ( outfile, "\t\twhen B\"00_0100\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"00_0101\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\taluOP <= opOR ; \n"
		          "\t\t\tregI <= alu ; \n"
		          "\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\tnc <= '0' ; \n"
		          "\t\t\tregW <= '1' ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- XOR \ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	fprintf ( outfile, "\t\t\t-- XOR \ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x06 ) ) | ( inst_map & ( 1LLU << 0x07 ) ) ) {
		fprintf ( outfile, "\t\twhen B\"00_0110\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"00_0111\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\taluOP <= opXOR ; \n"
		          "\t\t\tregI <= alu ; \n"
		          "\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\tnc <= '0' ; \n"
		          "\t\t\tregW <= '1' ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- TEST\ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	fprintf ( outfile, "\t\t\t-- TEST\ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x0C ) ) | ( inst_map & ( 1LLU << 0x0D ) ) ) {
		fprintf ( outfile, "\t\twhen B\"00_1100\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"00_1101\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\taluOP <= opAND ; \n"
		          "\t\t\tregI <= alu ; \n"
		          "\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\tnc <= xor_reduce( regI ) ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- TSTC\ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	fprintf ( outfile, "\t\t\t-- TSTC\ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x0E ) ) | ( inst_map & ( 1LLU << 0x0F ) ) ) {
		fprintf ( outfile, "\t\twhen B\"00_1110\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"00_1111\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\taluOP <= opAND ; \n"
		          "\t\t\tregI <= alu ; \n"
		          "\t\t\tnz <= to_std_logic( regI = X\"00\" ) and z ; \n"
		          "\t\t\tnc <= xor_reduce( regI ) xor c ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- ADD \ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	fprintf ( outfile, "\t\t\t-- ADD \ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x10 ) ) | ( inst_map & ( 1LLU << 0x11 ) ) ) {
		fprintf ( outfile, "\t\twhen B\"01_0000\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"01_0001\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tci <= '0' ; \n"
		          "\t\t\taluOP <= opXOR ; \n"
		          "\t\t\tregI <= alu ; \n"
		          "\t\t\tnc <= co ; \n"
		          "\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\tregW <= '1' ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- ADDC\ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	fprintf ( outfile, "\t\t\t-- ADDC\ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x12 ) ) | ( inst_map & ( 1LLU << 0x13 ) ) ) {
		fprintf ( outfile, "\t\twhen B\"01_0010\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"01_0011\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tci <= c ; \n"
		          "\t\t\taluOP <= opXOR ; \n"
		          "\t\t\tregI <= alu ; \n"
		          "\t\t\tnc <= co ; \n"
		          "\t\t\tnz <= to_std_logic( regI = X\"00\" ) and z ; \n"
		          "\t\t\tregW <= '1' ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- SUB \ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	fprintf ( outfile, "\t\t\t-- SUB \ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x18 ) ) | ( inst_map & ( 1LLU << 0x19 ) ) ) {
		fprintf ( outfile, "\t\twhen B\"01_1000\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"01_1001\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tci <= '1' ; \n"
		          "\t\t\taluOP <= opXNOR ; \n"
		          "\t\t\tregI <= alu ; \n"
		          "\t\t\tnc <= not co ; \n"
		          "\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\tregW <= '1' ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- SUBC\ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	fprintf ( outfile, "\t\t\t-- SUBC\ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x1A ) ) | ( inst_map & ( 1LLU << 0x1B ) ) ) {
		fprintf ( outfile, "\t\twhen B\"01_1010\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"01_1011\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tci <= not c ; \n"
		          "\t\t\taluOP <= opXNOR ; \n"
		          "\t\t\tregI <= alu ; \n"
		          "\t\t\tnc <= not co ; \n"
		          "\t\t\tnz <= to_std_logic( regI = X\"00\" ) and z ; \n"
		          "\t\t\tregW <= '1' ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- COMP\ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	fprintf ( outfile, "\t\t\t-- COMP\ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x1C ) ) | ( inst_map & ( 1LLU << 0x1D ) ) ) {
		fprintf ( outfile, "\t\twhen B\"01_1100\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"01_1101\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tci <= '1' ; \n"
		          "\t\t\taluOP <= opXNOR ; \n"
		          "\t\t\tregI <= alu ; \n"
		          "\t\t\tnc <= not co ; \n"
		          "\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- CMPC\ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	fprintf ( outfile, "\t\t\t-- CMPC\ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x1E ) ) | ( inst_map & ( 1LLU << 0x1F ) ) ) {
		fprintf ( outfile, "\t\twhen B\"01_1110\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"01_1111\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tci <= not c ; \n"
		          "\t\t\taluOP <= opXNOR ; \n"
		          "\t\t\tregI <= alu ; \n"
		          "\t\t\tnc <= not co ; \n"
		          "\t\t\tnz <= to_std_logic( regI = X\"00\" ) and z ; \n"
		        ) ;
	}

	if ( inst_map & ( 1LLU << 0x14 ) ) {
		fprintf ( outfile, "\n\t\twhen B\"01_0100\" => \n" ) ;

		fprintf ( outfile, "\t\t\t\tcase inst( 7 ) & inst( 3 downto 0 ) is \n" ) ;

		fprintf ( outfile, "\n\t\t\t\t-- CORE\ts%X   \t; \n", DestReg ( c ) ) ;
		fprintf ( outfile, "\t\t\twhen B\"1_0000\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\t\tregI <= X\"42\" ; \n"
		          "\t\t\t\tnc <= '1' ; \n"
		          "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\t\tregW <= '1' ; \n"
		        ) ;

		fprintf ( outfile, "\n\t\t\t\t-- RL  \ts%X      \t; \n", DestReg ( c ) ) ;
		fprintf ( outfile, "\t\t\twhen B\"0_0010\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\t\tregI <= dataA( 6 downto 0 ) & dataA( 7 ) ; \n"
		          "\t\t\t\tnc <= dataA( 7 ) ; \n"
		          "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\t\tregW <= '1' ; \n"
		        ) ;

		fprintf ( outfile, "\n\t\t\t\t-- SL0 \ts%X      \t; \n", DestReg ( c ) ) ;
		fprintf ( outfile, "\t\t\twhen B\"0_0110\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\t\tregI <= dataA( 6 downto 0 ) & '0' ; \n"
		          "\t\t\t\tnc <= dataA( 7 ) ; \n"
		          "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\t\tregW <= '1' ; \n"
		        ) ;

		fprintf ( outfile, "\n\t\t\t\t-- SL1 \ts%X      \t; \n", DestReg ( c ) ) ;
		fprintf ( outfile, "\t\t\twhen B\"0_0111\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\t\tregI <= dataA( 6 downto 0 ) & '1' ; \n"
		          "\t\t\t\tnc <= dataA( 7 ) ; \n"
		          "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\t\tregW <= '1' ; \n"
		        ) ;

		fprintf ( outfile, "\n\t\t\t\t-- SLA \ts%X      \t; \n", DestReg ( c ) ) ;
		fprintf ( outfile, "\t\t\twhen B\"0_0000\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\t\tregI <= dataA( 6 downto 0 ) & c ; \n"
		          "\t\t\t\tnc <= dataA( 7 ) ; \n"
		          "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\t\tregW <= '1' ; \n"
		        ) ;

		fprintf ( outfile, "\n\t\t\t\t-- SLX \ts%X      \t; \n", DestReg ( c ) ) ;
		fprintf ( outfile, "\t\t\twhen B\"0_0100\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\t\tregI <= dataA( 6 downto 0 ) & dataA( 0 ) ; \n"
		          "\t\t\t\tnc <= dataA( 7 ) ; \n"
		          "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\t\tregW <= '1' ; \n"
		        ) ;

		fprintf ( outfile, "\n\t\t\t\t-- RR  \ts%X      \t; \n", DestReg ( c ) ) ;
		fprintf ( outfile, "\t\t\twhen B\"0_1100\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\t\tregI <= dataA( 0 ) & dataA( 7 downto 1 ) ; \n"
		          "\t\t\t\tnc <= dataA( 0 ) ; \n"
		          "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\t\tregW <= '1' ; \n"
		        ) ;

		fprintf ( outfile, "\n\t\t\t\t-- SR0 \ts%X      \t; \n", DestReg ( c ) ) ;
		fprintf ( outfile, "\t\t\twhen B\"0_1110\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\t\tregI <= '0' & dataA( 7 downto 1 ) ; \n"
		          "\t\t\t\tnc <= dataA( 0 ) ; \n"
		          "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\t\tregW <= '1' ; \n"
		        ) ;

		fprintf ( outfile, "\n\t\t\t\t-- SR1 \ts%X      \t; \n", DestReg ( c ) ) ;
		fprintf ( outfile, "\t\t\twhen B\"0_1111\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\t\tregI <= '1' & dataA( 7 downto 1 ) ; \n"
		          "\t\t\t\tnc <= dataA( 0 ) ; \n"
		          "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\t\tregW <= '1' ; \n"
		        ) ;

		fprintf ( outfile, "\n\t\t\t\t-- SRA \ts%X      \t; \n", DestReg ( c ) ) ;
		fprintf ( outfile, "\t\t\twhen B\"0_1000\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\t\tregI <= c & dataA( 7 downto 1 ) ; \n"
		          "\t\t\t\tnc <= dataA( 0 ) ; \n"
		          "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\t\tregW <= '1' ; \n"
		        ) ;

		fprintf ( outfile, "\n\t\t\t\t-- SRX \ts%X      \t; \n", DestReg ( c ) ) ;
		fprintf ( outfile, "\t\t\twhen B\"0_1010\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\t\tregI <= dataA( 7 ) & dataA( 7 downto 1 ) ; \n"
		          "\t\t\t\tnc <= dataA( 0 ) ; \n"
		          "\t\t\t\tnz <= to_std_logic( regI = X\"00\" ) ; \n"
		          "\t\t\t\tregW <= '1' ; \n"
		        ) ;

		fprintf ( outfile,
		          "\t\t\twhen others =>\n"
		          "\t\t\t\tnull ; \n"
		        ) ;
		fprintf ( outfile, "\t\t\t\tend case ; \n" ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- JUMP\t0x%03X    \t; \n", Address12 ( c ) ) ;
	if ( inst_map & ( 1LLU << 0x22 ) ) {
		fprintf ( outfile, "\t\twhen B\"10_0010\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tnpc <= addrN ; \n" ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- JUMP\tZ, 0x%03X \t; \n", Address12 ( c ) ) ;
	if ( inst_map & ( 1LLU << 0x32 ) ) {
		fprintf ( outfile, "\t\twhen B\"11_0010\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tif z = '1' then \n"
		          "\t\t\t\tnpc <= addrN ; \n"
		          "\t\t\tend if ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- JUMP\tNZ, 0x%03X\t; \n", Address12 ( c ) ) ;
	if ( inst_map & ( 1LLU << 0x36 ) ) {
		fprintf ( outfile, "\t\twhen B\"11_0110\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tif z = '0' then \n"
		          "\t\t\t\tnpc <= addrN ; \n"
		          "\t\t\tend if ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- JUMP\tC, 0x%03X \t; \n", Address12 ( c ) ) ;
	if ( inst_map & ( 1LLU << 0x3A ) ) {
		fprintf ( outfile, "\t\twhen B\"11_1010\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tif c = '1' then \n"
		          "\t\t\t\tnpc <= addrN ; \n"
		          "\t\t\tend if ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- JUMP\tNC, 0x%03X\t; \n", Address12 ( c ) ) ;
	if ( inst_map & ( 1LLU << 0x3E ) ) {
		fprintf ( outfile, "\t\twhen B\"11_1110\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tif c = '0' then \n"
		          "\t\t\t\tnpc <= addrN ; \n"
		          "\t\t\tend if ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- JUMP\ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	if ( inst_map & ( 1LLU << 0x26 ) ) {
		fprintf ( outfile, "\t\twhen B\"10_0110\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tnpc( 11 downto 8 ) <= unsigned( dataA( 3 downto 0 ) ) ; \n"
		          "\t\t\tnpc( 7 downto 0 ) <= unsigned( regB ) ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- CALL\t0x%03X    \t; \n", Address12 ( c ) ) ;
	if ( inst_map & ( 1LLU << 0x20 ) ) {
		fprintf ( outfile, "\t\twhen B\"10_0000\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tnpc <= addrN ; \n"

		          "\t\t\tnspW <= spW + 1 ; \n"
		          "\t\t\tnspR <= spW ; \n"
		          "\t\t\tstackW <= '1' ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- CALL\tZ, 0x%03X \t; \n", Address12 ( c ) ) ;
	if ( inst_map & ( 1LLU << 0x30 ) ) {
		fprintf ( outfile, "\t\twhen B\"11_0000\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tif z = '1' then \n"
		          "\t\t\t\tnpc <= addrN ; \n"

		          "\t\t\t\tnspW <= spW + 1 ; \n"
		          "\t\t\t\tnspR <= spW ; \n"
		          "\t\t\t\tstackW <= '1' ; \n"
		          "\t\t\tend if ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- CALL\tNZ, 0x%03X\t; \n", Address12 ( c ) ) ;
	if ( inst_map & ( 1LLU << 0x34 ) ) {
		fprintf ( outfile, "\t\twhen B\"11_0100\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tif z = '0' then \n"
		          "\t\t\t\tnpc <= addrN ; \n"

		          "\t\t\t\tnspW <= spW + 1 ; \n"
		          "\t\t\t\tnspR <= spW ; \n"
		          "\t\t\t\tstackW <= '1' ; \n"
		          "\t\t\tend if ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- CALL\tC, 0x%03X \t; \n", Address12 ( c ) ) ;
	if ( inst_map & ( 1LLU << 0x38 ) ) {
		fprintf ( outfile, "\t\twhen B\"11_1000\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tif c = '1' then \n"
		          "\t\t\t\tnpc <= addrN ; \n"

		          "\t\t\t\tnspW <= spW + 1 ; \n"
		          "\t\t\t\tnspR <= spW ; \n"
		          "\t\t\t\tstackW <= '1' ; \n"
		          "\t\t\tend if ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- CALL\tNC, 0x%03X\t; \n", Address12 ( c ) ) ;
	if ( inst_map & ( 1LLU << 0x3C ) ) {
		fprintf ( outfile, "\t\twhen B\"11_1100\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tif c = '0' then \n"
		          "\t\t\t\tnpc <= addrN ; \n"

		          "\t\t\t\tnspW <= spW + 1 ; \n"
		          "\t\t\t\tnspR <= spW ; \n"
		          "\t\t\t\tstackW <= '1' ; \n"
		          "\t\t\tend if ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- CALL\ts%X, s%X  \t;  \n", DestReg ( c ), SrcReg ( c ) ) ;
	if ( inst_map & ( 1LLU << 0x24 ) ) {
		fprintf ( outfile, "\t\twhen B\"10_0100\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tnpc( 11 downto 8 ) <= unsigned( dataA( 3 downto 0 ) ) ; \n"
		          "\t\t\tnpc( 7 downto 0 ) <= unsigned( regB ) ; \n"

		          "\t\t\tnspW <= spW + 1 ; \n"
		          "\t\t\tnspR <= spW ; \n"
		          "\t\t\tstackW <= '1' ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- RET \t         \t; \n" ) ;
	if ( inst_map & ( 1LLU << 0x25 ) ) {
		fprintf ( outfile, "\t\twhen B\"10_0101\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tnpc <= unsigned( addrR ) ; \n"

		          "\t\t\tnspW <= spR ; \n"
		          "\t\t\tnspR <= spR - 1 ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- RET \t Z        \t; \n" ) ;
	if ( inst_map & ( 1LLU << 0x31 ) ) {
		fprintf ( outfile, "\t\twhen B\"11_0001\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tif z = '1' then \n"
		          "\t\t\t\tnpc <= unsigned( addrR ) ; \n"

		          "\t\t\t\tnspW <= spR ; \n"
		          "\t\t\t\tnspR <= spR - 1 ; \n"
		          "\t\t\tend if ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- RET \t NZ       \t; \n" ) ;
	if ( inst_map & ( 1LLU << 0x35 ) ) {
		fprintf ( outfile, "\t\twhen B\"11_0101\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tif z = '0' then \n"
		          "\t\t\t\tnpc <= unsigned( addrR ) ; \n"

		          "\t\t\t\tnspW <= spR ; \n"
		          "\t\t\t\tnspR <= spR - 1 ; \n"
		          "\t\t\tend if ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- RET \t C        \t; \n" ) ;
	if ( inst_map & ( 1LLU << 0x39 ) ) {
		fprintf ( outfile, "\t\twhen B\"11_1001\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tif c = '1' then \n"
		          "\t\t\t\tnpc <= unsigned( addrR ) ; \n"

		          "\t\t\t\tnspW <= spR ; \n"
		          "\t\t\t\tnspR <= spR - 1 ; \n"
		          "\t\t\tend if ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- RET \t NC       \t; \n" ) ;
	if ( inst_map & ( 1LLU << 0x3D ) ) {
		fprintf ( outfile, "\t\twhen B\"11_1101\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tif c = '0' then \n"
		          "\t\t\t\tnpc <= unsigned( addrR ) ; \n"

		          "\t\t\t\tnspW <= spR ; \n"
		          "\t\t\t\tnspR <= spR - 1 ; \n"
		          "\t\t\tend if ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- RET \ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( inst_map & ( 1LLU << 0x21 ) ) {
		fprintf ( outfile, "\t\twhen B\"10_0001\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tnpc <= unsigned( addrR ) ; \n"

		          "\t\t\tnspW <= spR ; \n"
		          "\t\t\tnspR <= spR - 1 ; \n"
		          "\t\t\tregI <= std_logic_vector( inst( 7 downto 0 ) ) ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- ST  \ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	fprintf ( outfile, "\t\t\t-- ST  \ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x2E ) ) | ( inst_map & ( 1LLU << 0x2F ) ) ) {
		fprintf ( outfile, "\t\twhen B\"10_1110\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"10_1111\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tscrW <= '1' ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- LD  \ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	fprintf ( outfile, "\t\t\t-- LD  \ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x0A ) ) | ( inst_map & ( 1LLU << 0x0B ) ) ) {
		fprintf ( outfile, "\t\twhen B\"00_1010\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"00_1011\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tregI <= scrO ; \n"
		          "\t\t\tregW <= '1' ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- OUT \ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	fprintf ( outfile, "\t\t\t-- OUT \ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x2C ) ) | ( inst_map & ( 1LLU << 0x2D ) ) ) {
		fprintf ( outfile, "\t\twhen B\"10_1100\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"10_1101\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tnioW <= '1' ; \n"
		        ) ;
	}

	/*
	        case 0x2B000 ... 0x2BFFF :
	            fprintf ( outfile, "\t\t\t\t-- OUTK\t0x%02X, 0x%X\t; \n", ( c >> 4 ) & 0xFF, c & 0xF ) ;
	            fprintf ( outfile,
	                      "\t\t\t\nwr <= '1' ; \n" ) ;
	            break ;
	*/

	fprintf ( outfile, "\n\t\t\t-- IN  \ts%X, s%X  \t; \n", DestReg ( c ), SrcReg ( c ) ) ;
	fprintf ( outfile, "\t\t\t-- IN  \ts%X, 0x%02X\t; \n", DestReg ( c ), Constant ( c ) ) ;
	if ( ( inst_map & ( 1LLU << 0x08 ) ) | ( inst_map & ( 1LLU << 0x09 ) ) ) {
		fprintf ( outfile, "\t\twhen B\"00_1000\" => \n" ) ;
		fprintf ( outfile, "\t\twhen B\"00_1001\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tnioR <= '1' ; \n"
		          "\t\t\tregI <= PB2O.da ; \n"
		          "\t\t\tregW <= ioR ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- DINT\t \t; \n" ) ;
	fprintf ( outfile, "\t\t\t-- EINT\t \t; \n" ) ;
	if ( inst_map & ( 1LLU << 0x28 ) ) {
		fprintf ( outfile, "\t\twhen B\"10_1000\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tni <= inst( 0 ) ; \n"
		        ) ;
	}

	fprintf ( outfile, "\n\t\t\t-- RETI\t DISABLE\t; \n" ) ;
	fprintf ( outfile, "\t\t\t-- RETI\t ENABLE\t; \n" ) ;
	if ( ( inst_map & ( 1LLU << 0x29 ) ) != 0llu ) {
		fprintf ( outfile, "\t\twhen B\"10_1001\" => \n" ) ;
		fprintf ( outfile,
		          "\t\t\tni <= inst( 0 ) ; \n"
		          "\t\t\tnpc <= unsigned( addrR ) ; \n"

		          "\t\t\tnc <= stackT( 12 ) ; \n"
		          "\t\t\tnz <= stackT( 13 ) ; \n"

		          "\t\t\tnspW <= spR ; \n"
		          "\t\t\tnspR <= spR - 1 ; \n"
		        ) ;
	}


	/*
	        case 0x37000 :
	            fprintf ( outfile, "\t\t\t\t-- BANK\tA\t; \n" ) ;
	            fprintf ( outfile, "\t\t\t\tassert false ; \n" ) ;
	            break ;
	        case 0x37001 :
	            fprintf ( outfile, "\t\t\t\t-- BANK\tB\t; \n" ) ;
	            fprintf ( outfile, "\t\t\t\tassert false ; \n" ) ;
	            break ;
	*/

    // postamble
	fprintf ( outfile,
	          "\t\twhen others =>\n"
	          "\t\t\tnull ; \n"
	        ) ;

	fprintf ( outfile,
	          "\t\tend case ; \n"
	          "\tend process ; \n"
	          "end mix ; \n"
	        ) ;

	fclose ( outfile ) ;
	return true ;
}
