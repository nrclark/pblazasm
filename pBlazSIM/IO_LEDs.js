LEDs.addRack( 0, 0, 0 ) ;

function setData( port, value ) {
    LEDs.stValue( port, value ) ;
    print( "setData-> port: " + port + ", value: " + value ) ;
}

function getData( port ) {
    print( "getData-> port: " + port ) ;
    return port ;
}
