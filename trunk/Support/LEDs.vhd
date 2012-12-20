--
-- Copyright © 2003..2012 : Henk van Kampen <henk@mediatronix.com>
--
-- This file is part of pBlazASM.
--
-- pBlazASM is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- pBlazASM is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with pBlazASM.  If not, see <http://www.gnu.org/licenses/>.
--

library ieee ;
use ieee.std_logic_1164.all ;
use ieee.numeric_std.all ;

entity LEDRegister is
    generic (
        WIDTH : integer := 8 ;
		  INVERTED : boolean := false 
    ) ;
    port ( 
        clk : in std_logic ;
        reset : in std_logic ;

        LEDs : out std_logic_vector( WIDTH - 1 downto 0 )
    ) ;
end LEDRegister ;

architecture mix of LEDRegister is
    signal iLEDs : std_logic_vector( WIDTH - 1 downto 0 ) := ( 0 => '1', others => '0' ) ;
begin
    process ( clk, reset, iLEDs ) is
        variable count : unsigned( 23 downto 0 ) ;
        constant ONES : unsigned( 7 downto 0 ) := ( others => '1' ) ;
        variable nLEDs : std_logic_vector( WIDTH - 1 downto 0 ) ;
        variable bZERO0, bZERO1, bZERO2, direction : boolean ;
    begin
        if reset = '1' then
            iLEDs <= ( 0 => '1', others => '0' ) ;
            count := ( others => '0' ) ;
            direction := False ;
        elsif rising_edge( clk ) then
            if bZERO2 and bZERO1 and bZERO0 then
                iLEDs <= nLEDs ;
            end if ;
            bZERO0 := count( 7 downto 0 ) = ONES ;
            bZERO1 := count( 15 downto 8 ) = ONES ;
            bZERO2 := count( 21 downto 16 ) = ONES( 5 downto 0 ) ;
            count := count + 1 ;
            
            if iLEDs( 0 ) = '1' then
                direction := False ;
            elsif iLEDS( WIDTH - 1 ) = '1' then
                direction := True ;
            end if ;
        end if ;
    
        if direction then
            nLEDs := iLEDs( 0 ) & iLEDs( WIDTH - 1 downto 1 ) ;
        else
            nLEDs := iLEDs( WIDTH - 2 downto 0 ) & iLEDs( WIDTH - 1 ) ;
        end if ;
    end process ;
    
	 g0 : if not INVERTED generate
		 LEDs <= iLEDs ;
	 end generate ;
	 g1 : if INVERTED generate
		 LEDs <= not iLEDs ;
	 end generate ;
end mix ;

