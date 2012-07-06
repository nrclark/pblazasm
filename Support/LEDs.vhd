----------------------------------------------------------------------------------
-- Company: 			Mediatronix BV
-- Engineer: 			Henk van Kampen
-- 
-- Module Name:		LEDRegister - mix 
-- Project Name:		USBM2
----------------------------------------------------------------------------------

library ieee ;
use ieee.std_logic_1164.all ;
use ieee.numeric_std.all ;

entity LEDRegister is
	generic (
		WIDTH : integer := 8  
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
			bZERO2 := count( 23 downto 16 ) = ONES ;
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
	
	LEDs <= iLEDs ;
end mix ;

