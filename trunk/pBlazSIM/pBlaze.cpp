
#include "pBlaze.h"
#include "mainwindow.h"


Picoblaze::~Picoblaze( void ) {
    for ( int io = 0 ; io < 256 ; io += 1 )
        if ( IO[ io ].device != NULL )
            IO[ io ].device->IODevice::~IODevice() ;
}

Picoblaze::Picoblaze( void ) {
    for ( int address = 0 ; address < MAXMEM ; address += 1 ) {
        Code[ address ].code = 0 ;
        Code[ address ].line = 0 ;
        Code[ address ].breakpoint = false ;
        Code[ address ].item = NULL ;
    }

    for ( int io = 0 ; io < 256 ; io += 1 ) {
        IO[ io ].device = NULL ;
        IO[ io ].item = NULL ;
    }

    for ( int scr = 0 ; scr < MAXSCR ; scr += 1 ) {
        scratchpad[ scr ].value = 0;
        scratchpad[ scr ].item = NULL ;
    }
}

void Picoblaze::updateScratchPad( void ) {
    for ( int scr = 0 ; scr < MAXSCR ; scr += 1 ) {
        int n = scratchpad[ scr ].value ;
        QString s = QString("%1").arg( n, 2, 16 ).toUpper() ;
        scratchpad[ scr ].item->setData( s, Qt::DisplayRole ) ;
    }
}

void Picoblaze::updateStack( void ) {
    for ( int sp = 0 ; sp < 32 ; sp += 1 ) {
        int n = stack[ sp ].pc ;
        QString s = QString("%1").arg( n, 5, 16 ).toUpper() ;
        stack[ sp ].item->setData( s, Qt::DisplayRole ) ;
    }
}

void Picoblaze::updateRegisters( void ) {
    for ( int reg = 0 ; reg < 16 ; reg += 1 ) {
        int n = registers[ reg ].value ;
        QString s = QString("%1").arg( n, 4, 16 ).toUpper() ;
        registers[ reg ].item->setData( s, Qt::DisplayRole ) ;
    }
}

void Picoblaze::updateState( void ) {
    QString s = QString("%1").arg( pc, 4, 16 ).toUpper() ;
    stateItems[ 0 ]->setData( s, Qt::DisplayRole ) ;
    s = QString("%1").arg( sp, 4 ) ;
    stateItems[ 1 ]->setData( s, Qt::DisplayRole ) ;
    s = QString("%1").arg( (int)zero, 4 ) ;
    stateItems[ 2 ]->setData( s, Qt::DisplayRole ) ;
    s = QString("%1").arg( (int)carry, 4 ) ;
    stateItems[ 3 ]->setData( s, Qt::DisplayRole ) ;
    s = QString("%1").arg( (int)enable, 4 ) ;
    stateItems[ 4 ]->setData( s, Qt::DisplayRole ) ;
}

void Picoblaze::updateData( void ) {
    updateScratchPad() ;
    updateStack() ;
    updateRegisters() ;
}

void Picoblaze::resetPB6 ( void ) {
    pc = 0 ;
    sp = 0 ;
    zero = false ;
    carry = false ;
    enable = false ;

    CycleCounter = 0ll ;
}

bool Picoblaze::stepPB6 ( void ) {
    uint32_t c, t ;
    int addr = 0 ;

    c = Code[ pc ].code & 0x3FFFF ;
    npc = pc + 1 ;

    switch ( c ) {
    case 0x00000 ... 0x00FFF : // MOVE sX, sY
        registers[ DestReg ( c ) ].value = registers[ SrcReg ( c ) ].value ;
        break ;
    case 0x01000 ... 0x01FFF : // MOVE sX, K
        registers[ DestReg ( c ) ].value = Constant ( c ) ;
        break ;
    case 0x16000 ... 0x16FFF : // STAR
        break ;

    case 0x02000 ... 0x02FFF : // AND sX, sY
        registers[ DestReg ( c ) ].value = registers[ DestReg ( c ) ].value & registers[ SrcReg ( c ) ].value ;
        zero = registers[ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;
    case 0x03000 ... 0x03FFF : // AND sX, K
        registers[ DestReg ( c ) ].value = registers[ DestReg ( c ) ].value & Constant ( c ) ;
        zero = registers[ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;

    case 0x04000 ... 0x04FFF : // OR sX, sY
        registers[ DestReg ( c ) ].value = registers[ DestReg ( c ) ].value | registers[ SrcReg ( c ) ].value ;
        zero = registers[ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;
    case 0x05000 ... 0x05FFF : // OR sX, K
        registers[ DestReg ( c ) ].value = registers[ DestReg ( c ) ].value | Constant ( c ) ;
        zero = registers[ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;

    case 0x06000 ... 0x06FFF : // XOR sX, sY
        registers[ DestReg ( c ) ].value = registers[ DestReg ( c ) ].value ^ registers[ SrcReg ( c ) ].value ;
        zero = registers[ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;
    case 0x07000 ... 0x07FFF : // XOR sX, K
        registers[ DestReg ( c ) ].value = registers[ DestReg ( c ) ].value ^ Constant ( c ) ;
        zero = registers[ DestReg ( c ) ].value == 0 ;
        carry = false ;
        break ;

    case 0x0C000 ... 0x0CFFF : // TEST sX, sY
        t = registers[ DestReg ( c ) ].value & registers[ SrcReg ( c ) ].value ;
        zero = ( t & 0xFF ) == 0 ;
        t ^= t >> 4 ;
        t ^= t >> 2 ;
        t ^= t >> 1 ;
        carry = ( t & 1 ) == 1 ;
        break ;
    case 0x0D000 ... 0x0DFFF : // TEST sX, K
        t = registers[ DestReg ( c ) ].value & Constant ( c ) ;
        zero = ( t & 0xFF ) == 0 ;
        t ^= t >> 4 ;
        t ^= t >> 2 ;
        t ^= t >> 1 ;
        carry = ( t & 1 ) == 1 ;
        break ;
    case 0x0E000 ... 0x0EFFF : // TSTC sX, sY
        t = registers[ DestReg ( c ) ].value & registers[ SrcReg ( c ) ].value ;
        zero = ( t & 0xFF ) == 0 && zero ;
        t ^= t >> 4 ;
        t ^= t >> 2 ;
        t ^= t >> 1;
        carry = ( ( t & 1 ) == 1 ) ^ carry ;
        break ;
    case 0x0F000 ... 0x0FFFF : // TSTC sX, K
        t = registers[ DestReg ( c ) ].value & Constant ( c ) ;
        zero = ( t & 0xFF ) == 0 && zero ;
        t ^= t >> 4 ;
        t ^= t >> 2 ;
        t ^= t >> 1 ;
        carry = ( ( t & 1 ) == 1 ) ^ carry ;
        break ;

    case 0x10000 ... 0x10FFF : // ADD sX, sY
        t = registers[ DestReg ( c ) ].value + registers[ SrcReg ( c ) ].value ;
        registers[ DestReg ( c ) ].value = t & 0xFF ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x11000 ... 0x11FFF : // ADD sX, K
        t = registers[ DestReg ( c ) ].value + Constant ( c ) ;
        registers[ DestReg ( c ) ].value = t & 0xFF ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;

    case 0x12000 ... 0x12FFF : // ADDC sX, sY
        t = registers[ DestReg ( c ) ].value + registers[ SrcReg ( c ) ].value + ( carry ? 1 : 0 ) ;
        registers[ DestReg ( c ) ].value = t & 0xFF ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x13000 ... 0x13FFF : // ADDC sX, K
        t = registers[ DestReg ( c ) ].value + Constant ( c ) + ( carry ? 1 : 0 ) ;
        registers[ DestReg ( c ) ].value = t & 0xFF ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;

    case 0x18000 ... 0x18FFF : // SUB sX, sY
        t = registers[ DestReg ( c ) ].value - registers[ SrcReg ( c ) ].value ;
        registers[ DestReg ( c ) ].value = t & 0xFF ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x19000 ... 0x19FFF : // SUB sX, K
        t = registers[ DestReg ( c ) ].value - Constant ( c ) ;
        registers[ DestReg ( c ) ].value = t & 0xFF ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;

    case 0x1A000 ... 0x1AFFF : // SUBC sX, sY
        t = registers[ DestReg ( c ) ].value - registers[ SrcReg ( c ) ].value - ( carry ? 1 : 0 )  ;
        registers[ DestReg ( c ) ].value = t & 0xFF ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x1B000 ... 0x1BFFF : // SUBC sX, K
        t = registers[ DestReg ( c ) ].value - Constant ( c ) - ( carry ? 1 : 0 ) ;
        registers[ DestReg ( c ) ].value = t & 0xFF ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;

    case 0x1C000 ... 0x1CFFF : // COMP sX, sY
        t = registers[ DestReg ( c ) ].value - registers[ SrcReg ( c ) ].value ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x1D000 ... 0x1DFFF : // COMP sX, K
        t = registers[ DestReg ( c ) ].value - Constant ( c ) ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x1E000 ... 0x1EFFF : // CMPC sX, sY
        t = registers[ DestReg ( c ) ].value - registers[ SrcReg ( c ) ].value - ( carry ? 1 : 0 )  ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;
    case 0x1F000 ... 0x1FFFF : // CMPC sX, K
        t = registers[ DestReg ( c ) ].value - Constant ( c ) - ( carry ? 1 : 0 ) ;
        zero = ( t & 0xFF ) == 0 ;
        carry = ( ( t >> 8 ) & 1 ) == 1 ;
        break ;

    case 0x14000 ... 0x14FFF : // SHIFTs
        if ( c & 0xF0 ) {
            registers[ DestReg ( c ) ].value = 0x42 ;
        } else
            switch ( c & 0xF ) {
            case 0x2 : // RL sX
                t = registers[ DestReg ( c ) ].value ;
                registers[ DestReg ( c ) ].value = ( ( t << 1 ) | ( t >> 7 ) ) & 0xFF ;
                zero = registers[ DestReg ( c ) ].value == 0 ;
                carry = ( ( t >> 7 ) & 1 ) == 1 ;
                break ;
            case 0x6 : // SL0 sX
                t = registers[ DestReg ( c ) ].value ;
                registers[ DestReg ( c ) ].value = ( ( t << 1 ) | 0 ) & 0xFF ;
                zero = registers[ DestReg ( c ) ].value == 0 ;
                carry = ( ( t >> 7 ) & 1 ) == 1 ;
                break ;
            case 0x7 : // SL1 sX
                t = registers[ DestReg ( c ) ].value ;
                registers[ DestReg ( c ) ].value = ( ( t << 1 ) | 1 ) & 0xFF ;
                zero = registers[ DestReg ( c ) ].value == 0 ;
                carry = ( ( t >> 7 ) & 1 ) == 1 ;
                break ;
            case 0x0 : // SLA sX
                t = registers[ DestReg ( c ) ].value ;
                registers[ DestReg ( c ) ].value = ( t << 1 ) | ( carry ? 1 : 0 ) ;
                zero = registers[ DestReg ( c ) ].value == 0 ;
                carry = ( ( t >> 7 ) & 1 ) == 1 ;
                break ;
            case 0x4 : // SLX sX
                t = registers[ DestReg ( c ) ].value ;
                registers[ DestReg ( c ) ].value = ( t << 1 ) | ( t & 1 ) ;
                zero = registers[ DestReg ( c ) ].value == 0 ;
                carry = ( ( t >> 7 ) & 1 ) == 1 ;
                break ;

            case 0xC : // RR sX
                t = registers[ DestReg ( c ) ].value ;
                registers[ DestReg ( c ) ].value = ( ( t >> 1 ) | ( t << 7 ) ) & 0xFF ;
                zero = registers[ DestReg ( c ) ].value == 0 ;
                carry = ( t & 1 ) == 1 ;
                break ;
            case 0xE : // SR0 sX
                t = registers[ DestReg ( c ) ].value ;
                registers[ DestReg ( c ) ].value = ( ( t >> 1 ) | ( 0 << 7 ) ) & 0xFF ;
                zero = registers[ DestReg ( c ) ].value == 0 ;
                carry = ( t & 1 ) == 1 ;
                break ;
            case 0xF : // SR1 sX
                t = registers[ DestReg ( c ) ].value ;
                registers[ DestReg ( c ) ].value = ( ( t >> 1 ) | ( 1 << 7 ) ) & 0xFF ;
                zero = registers[ DestReg ( c ) ].value == 0 ;
                carry = ( t & 1 ) == 1 ;
                break ;
            case 0x8 : // SRA sX
                t = registers[ DestReg ( c ) ].value ;
                registers[ DestReg ( c ) ].value = ( t >> 1 ) | ( carry ? ( 1 << 7 ) : 0 ) ;
                zero = registers[ DestReg ( c ) ].value == 0 ;
                carry = ( t & 1 ) == 1 ;
                break ;
            case 0xA : // SRX sX
                t = registers[ DestReg ( c ) ].value ;
                registers[ DestReg ( c ) ].value = ( t >> 1 ) | ( t & 0x80 ) ;
                zero = registers[ DestReg ( c ) ].value == 0 ;
                carry = ( t  & 1 ) == 1 ;
                break ;

            default :
                return false ;
            }
        break ;

    case 0x22000 ... 0x22FFF :
        npc = Address12 ( c ) ;
        break ;
    case 0x32000 ... 0x32FFF :
        if ( zero )
            npc = Address12 ( c ) ;
        break ;
    case 0x36000 ... 0x36FFF :
        if ( !zero )
            npc = Address12 ( c ) ;
        break ;
    case 0x3A000 ... 0x3AFFF :
        if ( carry )
            npc = Address12 ( c ) ;
        break ;
    case 0x3E000 ... 0x3EFFF :
        if ( !carry )
            npc = Address12 ( c ) ;
        break ;
    case 0x26000 ... 0x26FFF :
        break ;

    case 0x20000 ... 0x20FFF :
        stack[ sp++ ].pc = npc ;
        npc = Address12 ( c ) ;
        break ;
    case 0x30000 ... 0x30FFF :
        if ( zero ) {
            stack[ sp++ ].pc = npc ;
            npc = Address12 ( c ) ;
        }
        break ;
    case 0x34000 ... 0x34FFF :
        if ( ! zero ) {
            stack[ sp++ ].pc = npc ;
            npc = Address12 ( c ) ;
        }
        break ;
    case 0x38000 ... 0x38FFF :
        if ( carry ) {
            stack[ sp++ ].pc = npc ;
            npc = Address12 ( c ) ;
        }
        break ;
    case 0x3C000 ... 0x3CFFF :
        if ( ! carry ) {
            stack[ sp++ ].pc = npc ;
            npc = Address12 ( c ) ;
        }
        break ;
    case 0x24000 ... 0x24FFF :
        break ;

    case 0x25000 ... 0x25FFF :
        if ( sp <= 0 )
            return false ;
        npc = stack[ --sp ].pc ;
        break ;
    case 0x31000 ... 0x31FFF :
        if ( zero ) {
            if ( sp <= 0 )
                return false ;
            npc = stack[ --sp ].pc ;
        }
        break ;
    case 0x35000 ... 0x35FFF :
        if ( ! zero ) {
                if ( sp <= 0 )
                    return false ;
            npc = stack[ --sp ].pc ;
        }
        break ;
    case 0x39000 ... 0x39FFF :
        if ( carry ) {
            if ( sp <= 0 )
                return false ;
            npc = stack[ --sp ].pc ;
        }
        break ;
    case 0x3D000 ... 0x3DFFF :
        if ( ! carry ) {
            if ( sp <= 0 )
                return false ;
            npc = stack[ --sp ].pc ;
        }
        break ;
    case 0x21000 ... 0x21FFF :
        if ( sp <= 0 )
            return false ;
        npc = stack[ --sp ].pc ;
        registers[ DestReg ( c ) ].value = Constant ( c ) ;
        break ;


    case 0x2E000 ... 0x2EFFF : // ST sX, sY
        scratchpad[ registers[ SrcReg ( c ) ].value ].value = registers[ DestReg ( c ) ].value & 0xFF ;
        break ;
    case 0x2F000 ... 0x2FFFF : // ST sX, K
        scratchpad[ Constant ( c ) ].value = registers[ DestReg ( c ) ].value & 0xFF ;
        break ;

    case 0x0A000 ... 0x0AFFF : // LD sX, sY
        registers[ DestReg ( c ) ].value = scratchpad[ registers[ SrcReg ( c ) ].value ].value & 0xFF ;
        break ;
    case 0x0B000 ... 0x0BFFF : // LD sX, K
        registers[ DestReg ( c ) ].value = scratchpad[ Constant ( c ) ].value & 0xFF ;
        break ;

    case 0x2C000 ... 0x2CFFF : // OUT sX, sY
        addr = registers[ SrcReg ( c ) ].value ;
        if ( IO[ addr ].device == NULL )
            return false ;
         IO[ addr ].device->setValue( addr, registers[ DestReg ( c ) ].value & 0xFF ) ;
        break ;
    case 0x2D000 ... 0x2DFFF : // OUT sX, K
        addr = Constant ( c ) ;
        if ( IO[ addr ].device == NULL )
            return false ;
        IO[ addr ].device->setValue( addr, registers[ DestReg ( c ) ].value & 0xFF ) ;
        break ;
    case 0x2B000 ... 0x2BFFF :
        break ;

    case 0x08000 ... 0x08FFF : // IN sX, sY
        addr = registers[ SrcReg ( c ) ].value ;
        if ( IO[ addr ].device == NULL )
            return false ;
        registers[ DestReg ( c ) ].value = IO[ addr ].device->getValue( addr ) & 0xFF ;
        break ;
    case 0x09000 ... 0x09FFF : // IN sX, K
        addr = Constant ( c ) ;
        if ( IO[ addr ].device == NULL )
            return false ;
        registers[ DestReg ( c ) ].value = IO[ addr ].device->getValue( addr ) & 0xFF ;
        break ;

    case 0x28000 :
        enable = false ;
        break ;
    case 0x28001 :
        enable = true ;
        break ;
    case 0x29000 :
        if ( sp <= 0 )
            return false ;
        npc = stack[ --sp ].pc ;
        zero = stack[ sp ].zero ;
        carry = stack[ sp ].carry ;
        enable = false ;
        break ;
    case 0x29001 :
        if ( sp <= 0 )
            return false ;
        npc = stack[ --sp ].pc ;
        zero = stack[ sp ].zero ;
        carry = stack[ sp ].carry ;
        enable = true ;
        break ;

    case 0x37000 :
        break ;
    case 0x37001 :
        break ;

    default :
      return false ;
    }

    pc = npc ;          // only update when no error
    CycleCounter += 2 ; // 2 clock cycles per instruction
    return true ;
}

// IO devices

// UART
uint32_t UART::getValue ( uint32_t address ) {
    if ( ( address & 1 ) == 0 )
        return ( (MainWindow *)w )->getUARTstatus() ;
    else
        return ( (MainWindow *)w )->getUARTdata() ;
}

void UART::setValue ( uint32_t address, uint32_t value ) {
    ( (MainWindow *)w )->setUARTdata( value ) ;
}

// Cyclecounter CC
uint32_t CC::getValue(uint32_t address)
{
    if ( ( address & 0x08 ) == 0 ) {
    uint64_t c = TimeStamp ;

        switch ( address & 0x07 ) {
        case 7 : c >>= 8 ;
        case 6 : c >>= 8 ;
        case 5 : c >>= 8 ;
        case 4 : c >>= 8 ;
        case 3 : c >>= 8 ;
        case 2 : c >>= 8 ;
        case 1 : c >>= 8 ;
        case 0 : ;
        }
        return c & 0xFF ;
    } else {
        uint64_t c = TimeDelta ;

        switch ( address & 0x07 ) {
        case 7 : c >>= 8 ;
        case 6 : c >>= 8 ;
        case 5 : c >>= 8 ;
        case 4 : c >>= 8 ;
        case 3 : c >>= 8 ;
        case 2 : c >>= 8 ;
        case 1 : c >>= 8 ;
        case 0 : ;
        }
        return c & 0xFF ;
    }
}

void CC::setValue(uint32_t address, uint32_t value)
{
    TimeDelta = pBlaze->CycleCounter - TimeStamp ;
    TimeStamp = pBlaze->CycleCounter ;
}

// SBOX, for Rijndael
uint32_t SBOX::getValue(uint32_t address)
{
    static const unsigned char SBox[256] = {
       0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
       0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
       0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
       0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
       0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
       0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
       0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
       0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
       0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
       0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
       0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
       0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
       0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
       0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
       0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
       0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
    } ;
    return SBox[ index & 0xFF ] ;
}

void SBOX::setValue( uint32_t address, uint32_t value )
{
    index = value ;
}


