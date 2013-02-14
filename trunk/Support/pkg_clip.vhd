
library ieee ;
use ieee.std_logic_1164.all ;
use ieee.numeric_std.all ;

package pkg_clip is
    function clip( aIn : signed ; len: integer ) return signed ;
    function clip( aIn : std_logic_vector ; len: integer ) return signed ;
    function clip( aIn : std_logic_vector ; len: integer ) return std_logic_vector ;
    function clip( aIn : signed ; len: integer ) return std_logic_vector ;
end package pkg_clip ;

package body pkg_clip is
    function clip( aIn : signed ; len: integer ) return signed is
        constant ZEROES : signed( aIn'high downto 0 ) := ( others => '0' ) ;
        constant ONES : signed( aIn'high downto 0 ) := ( others => '1' ) ;
        variable temp : signed( aIn'length - 1 downto 0 ) ;
    begin
        temp := aIn ;
        if temp( temp'high ) = '0' then
            if temp( temp'high - 1 downto len - 1 ) /= ZEROES( temp'high - 1 downto len - 1 ) then
                return  '0' & ONES( len  -2 downto 0 ) ;
            end if ;
        else
            if temp( temp'high - 1 downto len - 1 ) /= ONES( temp'high - 1 downto len - 1 ) then
                return '1' & ZEROES( len - 2 downto 0 ) ;
            end if ;
        end if ;
        return temp( len - 1 downto 0 ) ;
    end clip ;

    function clip( aIn : std_logic_vector ; len: integer ) return signed is
    begin
        return clip( signed( aIn ), len ) ;
    end clip ;

    function clip( aIn : std_logic_vector ; len: integer ) return std_logic_vector is
    begin
        return std_logic_vector( clip( signed( aIn ), len ) ) ;
    end clip ;

    function clip( aIn : signed ; len: integer ) return std_logic_vector is
    begin
        return std_logic_vector( clip( aIn, len ) ) ;
    end clip ;
end package body pkg_clip ;
