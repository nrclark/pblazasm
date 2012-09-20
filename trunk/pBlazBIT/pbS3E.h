
// blockrams en blockram columns

// XC3S100E   4  1
// XC3S250E  12  2
// XC3S500E  20  2
// XC3S1200E 28  2
// XC3S1600E 36  2

int cdXC3S100E[] = {
// xc3s100e static device information.
// Term, Iob,
// Clb, Clb, Empty,
// BramInt, Bram, Empty, Empty,
// Clb, Clb,
// Clock,
// Clb, Clb, Clb, Clb, Clb, Clb,
// Empty,
// Clb, Clb,
// Iob, Term, END };

//  DeviceInfo xc3s100e(667, 29, 23, xc3s100eColumns);
    3, // Clock
    2, // Term
    19, // IO
    19, 19, 19, 19,  19, 19, 19, 19,  19, 19, 19, 19, // Clb
    19, // IO
    2, // Term
    -76, // BRAM
    -19 // Int
} ;

// xc3s250e static device information.
// Term, Iob,
// 2 Clb, Clb,
// BramInt, Bram, Empty, Empty, Empty,
// 7 Clb, Clb, Clb, Clb, Clb, Clb, Clb,
// Clock,
// 7 Clb, Clb, Clb, Clb, Clb, Clb, Clb, Empty,
// BramInt, Bram, Empty, Empty,
// 7 Clb, Clb,
// Iob, Term, END };

//  DeviceInfo xc3s250e(1419, 43, 33, xc3s250eColumns);
int cdXC3S250E[] = {
    3, // Clock
    2, // Term
    19, // IO
    19, 19, 19, 19,  19, 19, 19, 19,  19, 19, 19, 19,  19, 19, 19, 19,
    19, 19, // Clb
    19, // IO
    2, // Term
    -76, 76, // BRAM
    -19, 19 // Int
} ;

// xc3s500e static device information.
// Term, Iob,
// 2 Clb, Clb,
// BramInt, Bram, Empty, Empty,
// 11 Clb, Clb, Clb, Empty, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb,
// Clock,
// 11 Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Empty, Clb, Clb, Clb,
// BramInt, Bram, Empty, Empty, Clb, Clb, Iob, Term, END };

// DeviceInfo xc3s500e(2255, 55, 41, xc3s500eColumns);
int cdXC3S500E[] = {
    3, // Clock
    2, // Term
    19, // IO
    19, 19, 19, 19,  19, 19, 19, 19,  19, 19, 19, 19,  19, 19, 19, 19,
    19, 19, 19, 19,  19, 19, 19, 19,  19, 19, // Clb
    19, // IO
    2, // Term
    -76, 76, // BRAM
    -19, 19 // Int
} ;

// xc3s1200e static device information.
// Term, Iob,
// 2 Clb, Clb,
// BramInt, Bram, Empty, Empty,
// 17 Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Empty, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb,
// ClockLL,
// 17 Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Empty, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb,
// BramInt, Bram, Empty, Empty,
// 2 Clb, Clb,
// Iob, Term, END };

//  DeviceInfo xc3s1200e(3710, 70, 53, xc3s1200eColumns);
int cdXC3S1200E[] = {
    3, // Clock
    2, // Term
    19, // IO
    19, 19, 19, 19,  19, 19, 19, 19,  19, 19, 19, 19,  19, 19, 19, 19,
    19, 19, 19, 19,  19, 19, 19, 19,  19, 19, 19, 19,  19, 19, 19, 19,
    19, 19, 19, 19,  19, 19, // Clb
    19, // IO
    2, // Term
    -76, 76, // BRAM
    -19, 19 // Int
} ;

// xc3s1600e static device information.
// Term, Iob, Clb, Clb, BramInt, Bram, Empty, Empty,
// Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Empty, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb,
// ClockLL,
// Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Empty, Clb, Clb, Clb, Clb, Clb, Clb, Clb, Clb,
// BramInt, Bram, Empty, Empty, Clb, Clb, Iob, Term, END };

//  DeviceInfo xc3s1600e(5720, 88, 65, xc3s1600eColumns);
int cdXC3S1600E[] = {
    3, // Clock
    2, // Term
    19, // IO
    19, 19, 19, 19,  19, 19, 19, 19,  19, 19, 19, 19,  19, 19, 19, 19,
    19, 19, 19, 19,  19, 19, 19, 19,  19, 19, 19, 19,  19, 19, 19, 19,
    19, 19, 19, 19,  19, 19, 19, 19,  19, 19, 19, 19,  19, 19, 19, 19,
    19, 19, 19, 19,  19, 19, 19, 19,  19, 19, // Clb
    19, // IO
    2, // Term
    -76, 76, // BRAM
    -19, 19 // Int
} ;

int * ColumnDefs[ tyLast ] = {
    NULL, //    tyXC3S50,
    NULL, //    tyXC3S200,
    NULL, //    tyXC3S400,
    NULL, //    tyXC3S1000,
    NULL, //    tyXC3S1500,
    NULL, //    tyXC3S2000,
    NULL, //    tyXC3S4000,
    NULL, //    tyXC3S5000,

    cdXC3S100E,  // tyXC3S100E,
    cdXC3S250E,  // tyXC3S250E,
    cdXC3S500E,  // tyXC3S500E,
    cdXC3S1200E, // tyXC3S1200E,
    cdXC3S1600E, // tyXC3S1600E,

    NULL, //    tyXC3S50A,
    NULL, //    tyXC3S200A,
    NULL, //    tyXC3S400A,
    NULL, //    tyXC3S700A,
    NULL, //    tyXC3S1400A,

    NULL, //    tyXC3S50AN,
    NULL, //    tyXC3S200AN,
    NULL, //    tyXC3S400AN,
    NULL, //    tyXC3S700AN,
    NULL, //    tyXC3S1400AN,
    NULL, //    tyXC3SD1800A,
    NULL, //    tyXC3SD3400A,

// Spartan-6
    NULL, //    tyXC6SLX4,
    NULL, //    tyXC6SLX9,
    NULL, //    tyXC6SLX16,
    NULL, //    tyXC6SLX25,
    NULL, //    tyXC6SLX25T,
    NULL, //    tyXC6SLX45,
    NULL, //    tyXC6SLX45T,
    NULL, //    tyXC6SLX75,
    NULL, //    tyXC6SLX75T,
    NULL, //    tyXC6SLX100,
    NULL, //    tyXC6SLX100T,
    NULL, //    tyXC6SLX150,
    NULL, //    tyXC6SLX150T,

// Virtex-4L
    NULL, //    tyXC4VLX15,
    NULL, //    tyXC4VLX25,
    NULL, //    tyXC4VLX40,
    NULL, //    tyXC4VLX60,
    NULL, //    tyXC4VLX80,
    NULL, //    tyXC4VLX100,
    NULL, //    tyXC4VLX160,

// Virtex-4S
    NULL, //    tyXC4VSX25,
    NULL, //    tyXC4VSX35,
    NULL, //    tyXC4VSX55,

// Virtex-4F
    NULL, //    tyXC4VFX12,
    NULL, //    tyXC4VFX20,
    NULL, //    tyXC4VFX40,
    NULL, //    tyXC4VFX60,
    NULL, //    tyXC4VFX100,
    NULL, //    tyXC4VFX140,
    NULL, //    tyXC4VLX200,

// Virtex-5L
    NULL, //    tyXC5VLX30,
    NULL, //    tyXC5VLX50,
    NULL, //    tyXC5VLX85,
    NULL, //    tyXC5VLX110,
    NULL, //    tyXC5VLX155,
    NULL, //    tyXC5VLX220,
    NULL, //    tyXC5VLX330,
    NULL, //    tyXC5VLX20T,
    NULL, //    tyXC5VLX30T,
    NULL, //    tyXC5VLX50T,
    NULL, //    tyXC5VLX85T,
    NULL, //    tyXC5VLX110T,
    NULL, //    tyXC5VLX155T,
    NULL, //    tyXC5VLX220T,
    NULL, //    tyXC5VLX330T,

//    // Virtex-5S
    NULL, //    tyXC5VSX35T,
    NULL, //    tyXC5VSX50T,
    NULL, //    tyXC5VSX95T,
    NULL, //    tyXC5VSX240T,

    // Virtex-5F
    NULL, //    tyXC5VFX30T,
    NULL, //    tyXC5VFX70T,
    NULL, //    tyXC5VFX100T,
    NULL, //    tyXC5VFX130T,
    NULL, //    tyXC5VFX200T,
    NULL, //    tyXC5VTX150T,
    NULL, //    tyXC5VTX240T,

    // Virtex-6
    NULL, //    tyXC6VHX250T,
    NULL, //    tyXC6VHX255T,
    NULL, //    tyXC6VHX380T,
    NULL, //    tyXC6VHX565T,

    // Virtex-6
    NULL, //    tyXC6VLX75T,
    NULL, //    tyXC6VLX130T,
    NULL, //    tyXC6VLX195T,
    NULL, //    tyXC6VLX240T,
    NULL, //    tyXC6VLX365T,
    NULL, //    tyXC6VLX550T,
    NULL, //    tyXC6VLX760,

    // Virtex-6
    NULL, //    tyXC6VSX315T,
    NULL, //    tyXC6VSX475T,
    NULL, //    tyXQ6VLX130T,
    NULL, //    tyXQ6VLX240T,
    NULL, //    tyXQ6VLX550T,
    NULL, //    tyXQ6VSX315T,
    NULL  //    tyXQ6VSX475T
} ;

