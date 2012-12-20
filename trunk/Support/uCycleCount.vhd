
-- AES SBOX as a Picoblaze I/O device
-- occupies 1 byte in the address map.
-- witing a value sets the address pointer of the ROM
-- reading reads the byte pointed at by the address

library ieee ;
use ieee.std_logic_1164.all; 
use ieee.numeric_std.all; 

use work.pb2_pkg.all ;

entity CycleCount is
    generic (
        constant LOC : std_logic_vector ( 7 downto 0 ) := X"00"
    ) ;
    port ( 
        PB2I : in t_PB2I ;
        PB2O : out t_PB2O
    ) ;
end CycleCount ;

architecture mix of CycleCount is
    signal rd, wr : std_logic ;
    signal counter : unsigned( 63 downto 0 ) ;
    signal t1, t2, delta : unsigned( 63 downto 0 ) ;
    signal slv_t1, slv_t2 : std_logic_vector( 63 downto 0 ) ;
begin
    rd <= PB2I.rd when PB2I.ad( 7 downto 4 ) = LOC( 7 downto 4 ) else '0' ;
    wr  <= PB2I.wr when PB2I.ad( 7 downto 4 ) = LOC( 7 downto 4 ) else '0' ;
                        
    PB2O.oe <= rd ;
    
    slv_t1 <= std_logic_vector( t1 ) ;
    slv_t2 <= std_logic_vector( delta ) ;
    PB2O.da <= 
        slv_t2( 63 downto 56 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"1111" else
        slv_t2( 55 downto 48 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"1110" else
        slv_t2( 47 downto 40 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"1101" else
        slv_t2( 39 downto 32 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"1100" else
        slv_t2( 31 downto 24 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"1011" else
        slv_t2( 23 downto 16 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"1010" else
        slv_t2( 15 downto  8 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"1001" else
        slv_t2(  7 downto  0 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"1000" else
        slv_t1( 63 downto 56 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"0111" else
        slv_t1( 55 downto 48 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"0110" else
        slv_t1( 47 downto 40 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"0101" else
        slv_t1( 39 downto 32 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"0100" else
        slv_t1( 31 downto 24 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"0011" else
        slv_t1( 23 downto 16 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"0010" else
        slv_t1( 15 downto  8 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"0001" else
        slv_t1(  7 downto  0 ) when rd = '1' and PB2I.ad( 3 downto 0 ) = B"0000" else
        ( others => '0' ) ;

    process ( PB2I.ck, PB2I.rs ) is
    begin
        if PB2I.rs = '1' then
            counter <= ( others => '0' ) ;
        elsif rising_edge( PB2I.ck ) then
                counter <= counter + 1 ;
                if wr = '1' then
                    t2 <= counter ;
                    t1 <= t2 ;
                else
                    delta <= t2 - t1 ;
                end if ;
        end if ;
    end process ;
end mix ;

