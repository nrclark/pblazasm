
library ieee;
use ieee.std_logic_1164.all;

package pb2_pkg is
	-- PB2 bus	
	type t_PB2I is
		record
        ck : std_logic ;
        rs : std_logic ;
        da : std_logic_vector( 7 downto 0 ) ;
        ad : std_logic_vector( 7 downto 0 ) ;
        wr : std_logic ;
        rd : std_logic ;
		end record ;
	type t_PB2O is
		record
        da : std_logic_vector( 7 downto 0 ) ;
        oe : std_logic ;
		end record ;
end pb2_pkg;

package body pb2_pkg is 
end pb2_pkg;
