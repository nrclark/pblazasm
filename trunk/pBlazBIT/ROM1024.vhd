
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

entity ROM1024 is
    generic (
        ROM_NBR : integer := 0
    ) ;
    port (     
        clk : in std_logic ;

        address : in std_logic_vector( 9 downto 0 ) ;
        instruction : out std_logic_vector( 17 downto 0 ) ;

        scrpad_address : in std_logic_vector( 7 downto 0 ) ;
        scrpad_rdata : out std_logic_vector( 7 downto 0 ) ;
        scrpad_wdata : in std_logic_vector( 7 downto 0 ) ;
        scr_pad_we : in std_logic
    ) ;
end ROM1024 ;

architecture structural of ROM1024 is   
    type INIT_ARRAY_t is array( 0 to 3 ) of bit_vector( 255 downto 0 ) ;

    -- digests
    constant INIT_ARRAY : INIT_ARRAY_t := (
        X"0000_0000_0000_0000_0000_0000_0000_0000_b4b1_47bc_5228_2873_1f1a_016b_fa72_c073",
        X"0000_0000_0000_0000_0000_0000_0000_0000_96a3_be3c_f272_e017_046d_1b26_74a5_2bd3",
        X"0000_0000_0000_0000_0000_0000_0000_0000_a2ef_406e_2c23_51e0_b9e8_0029_c909_242d",
        X"0000_0000_0000_0000_0000_0000_0000_0000_e45e_e7ce_7e88_149a_f8dd_32b2_7f95_12ce"
    ) ;
    constant INITP : bit_vector( 255 downto 0 ) := X"0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_0000_ffff_ffff" ;

    signal addra : std_logic_vector( 10 downto 0 ) ;
begin
    Code_ROM_00: RAMB16_S9_S18
        generic map ( 
            INIT_00  => INIT_ARRAY( ROM_NBR ),
            INITP_00 => INITP
        )
    port map ( 
            -- instruction ROM side
            DIB => "0000000000000000",
            DIPB => "00",

            DOB => instruction( 15 downto 0 ),
            DOPB => instruction( 17 downto 16 ),

            ADDRB => address,

            ENB => '1',
            WEB => '0',
            SSRB => '0',
            CLKB => clk,

            -- scratch pad side
            DIA => scrpad_wdata,
            DIPA => B"0",

            DOA => scrpad_rdata,
            DOPA => open,

            ADDRA => addra,

            ENA => '1',
            WEA => scr_pad_we,
            SSRA => '0',
            CLKA => clk
        ) ; 
                    
    addra <= "111" & scrpad_address ;
end structural ;


