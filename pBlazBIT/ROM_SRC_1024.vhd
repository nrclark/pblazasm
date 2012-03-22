
--
--  Copyright © 2003..2012 : Henk van Kampen <henk@mediatronix.com>
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

library ieee ;
use ieee.std_logic_1164.all ;
use ieee.numeric_std.all ;

library unisim;
use unisim.vcomponents.all;

entity ROM_SRC_1024 is
    generic (
        ROM_NBR : integer := 0
    ) ;
    port (     
        clk : in std_logic ;
		  enable : in std_logic ;

        address : in std_logic_vector( 11 downto 0 ) ;
        instruction : out std_logic_vector( 17 downto 0 ) ;

        scrpad_address : in std_logic_vector( 7 downto 0 ) ;
        scrpad_rdata : out std_logic_vector( 7 downto 0 ) ;
        scrpad_wdata : in std_logic_vector( 7 downto 0 ) ;
        scr_pad_we : in std_logic
    ) ;
end ROM_SRC_1024 ;

architecture mix of ROM_SRC_1024 is   
	type INIT_ARRAY_t is array( 0 to 3 ) of bit_vector( 255 downto 0 ) ;

	-- digests
	constant INIT_ARRAY : INIT_ARRAY_t := (
		X"0000_0000_0000_0000_0000_0000_0000_0000_b4b1_47bc_5228_2873_1f1a_016b_fa72_c073",
		X"0000_0000_0000_0000_0000_0000_0000_0000_96a3_be3c_f272_e017_046d_1b26_74a5_2bd3",
		X"0000_0000_0000_0000_0000_0000_0000_0000_a2ef_406e_2c23_51e0_b9e8_0029_c909_242d",
		X"0000_0000_0000_0000_0000_0000_0000_0000_e45e_e7ce_7e88_149a_f8dd_32b2_7f95_12ce"
	) ;
	constant INITP : bit_vector( 255 downto 0 ) :=
		X"0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_ffff_ffff" ;

	signal  address_a : std_logic_vector( 13 downto 0 ) ;
	signal  data_in_a : std_logic_vector( 35 downto 0 ) ;
	signal data_out_a : std_logic_vector( 35 downto 0 ) ;
	signal      ena_a : std_logic ;
	signal       we_a : std_logic_vector( 3 downto 0 ) ;
	
	signal  address_b : std_logic_vector( 13 downto 0 ) ;
	signal  data_in_b : std_logic_vector( 35 downto 0 ) ;
	signal data_out_b : std_logic_vector( 35 downto 0 ) ;
	signal      ena_b : std_logic ;
	signal       we_b : std_logic_vector( 3 downto 0 ) ;
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
      ena_b    <= enable ;
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

			      INIT_00  => INIT_ARRAY( ROM_NBR ),
			      INITP_00 => INITP
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
end mix ;


