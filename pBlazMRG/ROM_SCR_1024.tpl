
--
--  Copyright � 2003..2012 : Henk van Kampen <henk@mediatronix.com>
--
--  This file is part of pBlazBIT.
--
--  pBlazBIT is free software: you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation, either version 3 of the License, or
--  (at your option) any later version.
--
--  pBlazBIT is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with pBlazBIT. If not, see <http://www.gnu.org/licenses/>.
--

-- example blank 1024 word code rom and 256 byte scratchpad
--
-- ROM is preinitialized by a 128 bit digest, depending on the ROM_NBR (range 0 to 3).
-- pBlazBIT finds the digest and places the contents of a MEM file there directly in the bit file.
-- HVK 120320
-- {begin template}

-- Generated by {tool} on {timestamp} from {psmname}

library ieee ;
use ieee.std_logic_1164.all ;
use ieee.numeric_std.all ;

library unisim;
use unisim.vcomponents.all;

entity {name} is
	generic (
		ROM_NBR : integer := 0
	) ;
	port (     
		clk : in std_logic ;


		address : in std_logic_vector( 11 downto 0 ) ;
		instruction : out std_logic_vector( 17 downto 0 ) ;

		scrpad_address : in std_logic_vector( 7 downto 0 ) ;
		scrpad_rdata : out std_logic_vector( 7 downto 0 ) ;
		scrpad_wdata : in std_logic_vector( 7 downto 0 ) ;
		scr_pad_we : in std_logic
	) ;
end {name} ;

architecture structural of {name} is   
	signal  address_a : std_logic_vector( 13 downto 0 ) ;
	signal  data_in_a : std_logic_vector( 35 downto 0 ) ;
	signal data_out_a : std_logic_vector( 35 downto 0 ) ;
	signal      ena_a : std_logic ;
	signal       we_a : std_logic_vector(  3 downto 0 ) ;

	signal  address_b : std_logic_vector( 13 downto 0 ) ;
	signal  data_in_b : std_logic_vector( 35 downto 0 ) ;
	signal data_out_b : std_logic_vector( 35 downto 0 ) ;
	signal      ena_b : std_logic ;
	signal       we_b : std_logic_vector(  3 downto 0 ) ;
begin

	-- code map
  address_a    <= address( 9 downto 0 ) & B"0000" ;
  instruction  <= data_out_a( 33 downto 32 ) & data_out_a( 15 downto 0 ) ;
  data_in_a    <= B"00_0000_0000_0000_0000_0000_0000_0000_0000" & address( 11 downto 10 ) ;
      ena_a    <= enable ;
       we_a    <= B"0000" ;

   -- scratchpad map at 0x380 in the code map ( would be 0x700 for data map )
  address_b    <= B"111" & scrpad_address & B"000" ;
  data_in_b    <= B"0000_0000_0000_0000_0000_0000_0000" & scrpad_wdata( 7 downto 0 ) ;
  scrpad_rdata <= data_out_b( 7 downto 0 ) ;
      ena_b    <= not enable ;
       we_b    <= scr_pad_we & scr_pad_we & scr_pad_we & scr_pad_we ;

	iROM_SRC : RAMB16BWER
		generic map ( 
			DATA_WIDTH_A => 18,
			DOA_REG => 0,
			EN_RSTRAM_A => FALSE,
			INIT_A => X"000000000",
			RST_PRIORITY_A => "CE",
			SRVAL_A => X"000000000",
			WRITE_MODE_A => "WRITE_FIRST",

			DATA_WIDTH_B => 9,
			DOB_REG => 0,
			EN_RSTRAM_B => FALSE,
			INIT_B => X"000000000",
			RST_PRIORITY_B => "CE",
			SRVAL_B => X"000000000",
			WRITE_MODE_B => "WRITE_FIRST",

			RSTTYPE => "SYNC",
			INIT_FILE => "NONE",
			SIM_COLLISION_CHECK => "ALL",
			SIM_DEVICE => "SPARTAN6",

			INIT_00 => X"{INIT_00}",
			INIT_01 => X"{INIT_01}",
			INIT_02 => X"{INIT_02}",
			INIT_03 => X"{INIT_03}",
			INIT_04 => X"{INIT_04}",
			INIT_05 => X"{INIT_05}",
			INIT_06 => X"{INIT_06}",
			INIT_07 => X"{INIT_07}",
			INIT_08 => X"{INIT_08}",
			INIT_09 => X"{INIT_09}",
			INIT_0A => X"{INIT_0A}",
			INIT_0B => X"{INIT_0B}",
			INIT_0C => X"{INIT_0C}",
			INIT_0D => X"{INIT_0D}",
			INIT_0E => X"{INIT_0E}",
			INIT_0F => X"{INIT_0F}",
			INIT_10 => X"{INIT_10}",
			INIT_11 => X"{INIT_11}",
			INIT_12 => X"{INIT_12}",
			INIT_13 => X"{INIT_13}",
			INIT_14 => X"{INIT_14}",
			INIT_15 => X"{INIT_15}",
			INIT_16 => X"{INIT_16}",
			INIT_17 => X"{INIT_17}",
			INIT_18 => X"{INIT_18}",
			INIT_19 => X"{INIT_19}",
			INIT_1A => X"{INIT_1A}",
			INIT_1B => X"{INIT_1B}",
			INIT_1C => X"{INIT_1C}",
			INIT_1D => X"{INIT_1D}",
			INIT_1E => X"{INIT_1E}",
			INIT_1F => X"{INIT_1F}",
			INIT_20 => X"{INIT_20}",
			INIT_21 => X"{INIT_21}",
			INIT_22 => X"{INIT_22}",
			INIT_23 => X"{INIT_23}",
			INIT_24 => X"{INIT_24}",
			INIT_25 => X"{INIT_25}",
			INIT_26 => X"{INIT_26}",
			INIT_27 => X"{INIT_27}",
			INIT_28 => X"{INIT_28}",
			INIT_29 => X"{INIT_29}",
			INIT_2A => X"{INIT_2A}",
			INIT_2B => X"{INIT_2B}",
			INIT_2C => X"{INIT_2C}",
			INIT_2D => X"{INIT_2D}",
			INIT_2E => X"{INIT_2E}",
			INIT_2F => X"{INIT_2F}",
			INIT_30 => X"{INIT_30}",
			INIT_31 => X"{INIT_31}",
			INIT_32 => X"{INIT_32}",
			INIT_33 => X"{INIT_33}",
			INIT_34 => X"{INIT_34}",
			INIT_35 => X"{INIT_35}",
			INIT_36 => X"{INIT_36}",
			INIT_37 => X"{INIT_37}",
			INIT_38 => X"{INIT_38}",
			INIT_39 => X"{INIT_39}",
			INIT_3A => X"{INIT_3A}",
			INIT_3B => X"{INIT_3B}",
			INIT_3C => X"{INIT_3C}",
			INIT_3D => X"{INIT_3D}",
			INIT_3E => X"{INIT_3E}",
			INIT_3F => X"{INIT_3F}",
			INITP_00 => X"{INITP_00}",
			INITP_01 => X"{INITP_01}",
			INITP_02 => X"{INITP_02}",
			INITP_03 => X"{INITP_03}",
			INITP_04 => X"{INITP_04}",
			INITP_05 => X"{INITP_05}",
			INITP_06 => X"{INITP_06}",
			INITP_07 => X"{INITP_07}"
		)
		port map(  
			-- instruction ROM side
			ADDRA => address_a,
			ENA => ena_a,
			CLKA => clk,
			DOA => data_out_a( 31 downto 0 ),
			DOPA => data_out_a( 35 downto 32 ), 
			DIA => data_in_a( 31 downto 0 ),
			DIPA => data_in_a( 35 downto 32 ), 
			WEA => we_a,
			REGCEA => '0',
			RSTA => '0',

			-- scratch pad side
			ADDRB => address_b,
			ENB => ena_b,
			CLKB => clk,
			DOB => data_out_b( 31 downto 0 ),
			DOPB => data_out_b( 35 downto 32 ), 
			DIB => data_in_b( 31 downto 0 ),
			DIPB => data_in_b( 35 downto 32 ), 
			WEB => we_b,
			REGCEB => '0',
			RSTB => '0'
		  ) ;                    
end structural ;

