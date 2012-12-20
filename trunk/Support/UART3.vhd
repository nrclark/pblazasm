
library ieee ;
use ieee.std_logic_1164.all ;
use ieee.numeric_std.all ;

use work.pb2_pkg.all ;

entity UART3 is
    generic (
        constant CLOCK : integer := 0 ;
        constant LOC : std_logic_vector ( 7 downto 0 ) := X"00" ;
        constant BAUD : integer := 0
    ) ;
    port ( 
        PB2I : in t_PB2I ;
        PB2O : out t_PB2O ;

        tx : out std_logic ;
        rx : in std_logic
    ) ;
end UART3 ;

architecture mix of UART3 is
    constant UART_CS : std_logic_vector( 3 downto 0 ) := X"C" ;
    constant UART_DA : std_logic_vector( 3 downto 0 ) := X"D" ;
    
    signal rds, rdd, wr, baudclk : std_logic ;
    signal status : std_logic_vector( 7 downto 0 ) := ( others => '0' ) ;
    signal rxdata : std_logic_vector( 7 downto 0 ) ;
begin
    rds <= PB2I.rd when PB2I.ad( 7 downto 4 ) = LOC( 7 downto 4 ) and PB2I.AD( 3 downto 0 ) = UART_CS else '0' ;
    rdd <= PB2I.rd when PB2I.ad( 7 downto 4 ) = LOC( 7 downto 4 ) and PB2I.AD( 3 downto 0 ) = UART_DA else '0' ;
    wr  <= PB2I.wr when PB2I.ad( 7 downto 4 ) = LOC( 7 downto 4 ) and PB2I.AD( 3 downto 0 ) = UART_DA else '0' ;
                        
    PB2O.oe <= rds or rdd ;
    
    PB2O.da <= 
        status when rds = '1' else
        rxdata when rdd = '1' else
        ( others => '0' ) ;

    status( 6 ) <= '0' ;    
    status( 7 ) <= '1' when status( 6 downto 0 ) /= "0000000" else '0' ;    

    process ( PB2I.ck, PB2I.rs ) is
        variable count, ncount : integer ;
        variable bZERO : boolean ;
    begin
        if PB2I.rs = '1' then
            baudclk <= '0' ;
        elsif rising_edge( PB2I.ck ) then
            bZERO := count < 0 ;
            ncount := count - BAUD * 16 ;
            baudclk <= '0' ;
            if bZERO then
                baudclk <= '1' ;
                ncount := count + CLOCK - BAUD * 16 ;
            end if ;
            count := ncount ;
            end if ;
    end process ;
        
    recv : entity work.uart_rx( macro_level_definition )
        port map (
            serial_in => rx,                                                
            data_out => rxdata,                                
            read_buffer => rdd,                                
            reset_buffer => PB2I.rs,                               
            en_16_x_baud => baudclk,                           
 
                buffer_data_present => status( 5 ),                
            buffer_full => status( 4 ),                        
            buffer_half_full => status( 3 ), 
                
            clk => PB2I.ck                                         
        ) ;

    xmit : entity work.uart_tx( macro_level_definition )
        port map (
            data_in => PB2I.da,                         
            write_buffer => wr,                    
            reset_buffer => PB2I.rs,                   
            en_16_x_baud => baudclk,               
            serial_out => tx,                      

--            buffer_data_present => status( 2 ),    
            buffer_full => status( 0 ),            
            buffer_half_full => status( 1 ), 
                
            clk => PB2I.ck                             
        ) ;               
            status( 2 ) <= '0' ;
                
end mix ;
