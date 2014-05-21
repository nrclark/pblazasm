//
// script example
//

// allocate 4 LED bars at 0x04..0x07
LEDs.addRack( 0, 0x04, 0 ) ;
LEDs.addRack( 1, 0x05, 1 ) ;
LEDs.addRack( 2, 0x06, 2 ) ;
LEDs.addRack( 3, 0x07, 3 ) ;

// assume an UART at 0x00..0x01
// UART is a link to the virtual terminal

function setData( port, value ) {
    switch ( port ) {
    case 0 :
        break ;
    case 1 : 
        UART.setData( value ) ;
        break ;
    case 4 : case 5 : case 6 : case 7 :
        LEDs.setValue( port, value ) ;
        break ;
    default :
        print( "setData-> port: " + port + ", value: " + value ) ;
    }
}

function getData( port ) {
    switch ( port ) {
    case 0 : 
        return UART.getStatus() ;
    case 1 : 
        return UART.getData() ;
    case 4 : case 5 : case 6 : case 7 :
        return LEDs.getValue( port ) ;
    default :
        print( "getData-> port: " + port ) ;
    }
}
