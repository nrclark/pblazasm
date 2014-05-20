#include "qmtxlcddisplay.h"

QmtxLCDDisplay::QmtxLCDDisplay(QObject *parent) :
    QGraphicsScene(parent)
{
}

//Virtual 7-Segment Display
//-------------------------

//The PicoTerm Virtual 7-Segment Display is a pop-up window containing a virtual 4-digit,
//7-segment display. The digits and their segments are identified below.


//              Digit 3             Digit 2             Digit 1             Digit 0

//                 a                   a                   a                   a
//                ___                 ___                 ___                 ___
//              |     |
//            f |     | b         f |     | b         f |     | b         f |     | b
//              |  g  |             |  g  |             |  g  |             |  g  |
//                ___                 ___                 ___                 ___

//              |     |             |     |             |     |             |     |
//            e |     | c         e |     | c         e |     | c         e |     | c
//              |  d  |             |  d  |             |  d  |             |  d  |
//                ___   p            ___    p             ___   p             ___   p


//The virtual display is opened and updated using a 'Device Control String' (DCS) sequence. When
//PicoTerm receives the first virtual display DCS sequence it will open the window and display
//the digits with the specified segments 'turned on'. Subsequent virtual display DCS sequences
//will modify the display to reflect the new control values provided. Note that PicoTerm does
//not transmit a DCS sequence back to the COM port.

//The DCS sequence for the virtual 7-Segment display is as follows (please read the 'Device Control
//Strings' section above if you are unfamiliar with this concept)...

//    'DCS'                         (90 hex = 144)
//    '7'                           (37 hex = 55 )
//    digit0_segment_control_byte
//    digit1_segment_control_byte
//    digit2_segment_control_byte
//    digit3_segment_control_byte
//    'ST'                          (9C hex = 156)

//The segments of each digit are controlled by the bits contained in the control bytes. Each
//digit has 7 segments and a decimal point and a segment will be 'turned on' when the corresponding
//bit of the control byte is High (1).

//           Segment  Bit

//              a      0
//              b      1
//              c      2
//              d      3
//              e      4
//              f      5
//              g      6
//              p      7   decimal point

void QmtxLCDDisplay::drawSegment( const QPoint &pos, char segmentNo, QPainter &p, int segLen, bool erase) {
    QPoint ppt;
    QPoint pt = pos;
    int width = segLen/5;

//    const QPalette &pal = palette();
//    QColor lightColor,darkColor,fgColor;
//    if ( erase ){
//        lightColor = pal.color(backgroundRole());
//        darkColor  = lightColor;
//        fgColor    = lightColor;
//    } else {
//        lightColor = pal.light().color();
//        darkColor  = pal.dark().color();
//        fgColor    = pal.color(foregroundRole());
//    }

#define LINETO(X,Y) addPoint(a, QPoint(pt.x() + (X),pt.y() + (Y)))
#define LIGHT
#define DARK

        QPolygon a(0);
        //The following is an exact copy of the switch below.
        //don't make any changes here
        switch (segmentNo) {
        case 0 :
            ppt = pt;
            LIGHT;
            LINETO(segLen - 1,0);
            DARK;
            LINETO(segLen - width - 1,width);
            LINETO(width,width);
            LINETO(0,0);
            break;
        case 1 :
            pt += QPoint(0 , 1);
            ppt = pt;
            LIGHT;
            LINETO(width,width);
            DARK;
            LINETO(width,segLen - width/2 - 2);
            LINETO(0,segLen - 2);
            LIGHT;
            LINETO(0,0);
            break;
        case 2 :
            pt += QPoint(segLen - 1 , 1);
            ppt = pt;
            DARK;
            LINETO(0,segLen - 2);
            LINETO(-width,segLen - width/2 - 2);
            LIGHT;
            LINETO(-width,width);
            LINETO(0,0);
            break;
        case 3 :
            pt += QPoint(0 , segLen);
            ppt = pt;
            LIGHT;
            LINETO(width,-width/2);
            LINETO(segLen - width - 1,-width/2);
            LINETO(segLen - 1,0);
            DARK;
            if (width & 1) {            // adjust for integer division error
                LINETO(segLen - width - 3,width/2 + 1);
                LINETO(width + 2,width/2 + 1);
            } else {
                LINETO(segLen - width - 1,width/2);
                LINETO(width,width/2);
            }
            LINETO(0,0);
            break;
        case 4 :
            pt += QPoint(0 , segLen + 1);
            ppt = pt;
            LIGHT;
            LINETO(width,width/2);
            DARK;
            LINETO(width,segLen - width - 2);
            LINETO(0,segLen - 2);
            LIGHT;
            LINETO(0,0);
            break;
        case 5 :
            pt += QPoint(segLen - 1 , segLen + 1);
            ppt = pt;
            DARK;
            LINETO(0,segLen - 2);
            LINETO(-width,segLen - width - 2);
            LIGHT;
            LINETO(-width,width/2);
            LINETO(0,0);
            break;
        case 6 :
            pt += QPoint(0 , segLen*2);
            ppt = pt;
            LIGHT;
            LINETO(width,-width);
            LINETO(segLen - width - 1,-width);
            LINETO(segLen - 1,0);
            DARK;
            LINETO(0,0);
            break;
        case 7 :
//            if (smallPoint)   // if smallpoint place'.' between other digits
//                pt += QPoint(segLen + width/2 , segLen*2);
//            else
//                pt += QPoint(segLen/2 , segLen*2);
            ppt = pt;
            DARK;
            LINETO(width,0);
            LINETO(width,-width);
            LIGHT;
            LINETO(0,-width);
            LINETO(0,0);
            break;
        case 8 :
            pt += QPoint(segLen/2 - width/2 + 1 , segLen/2 + width);
            ppt = pt;
            DARK;
            LINETO(width,0);
            LINETO(width,-width);
            LIGHT;
            LINETO(0,-width);
            LINETO(0,0);
            break;
        case 9 :
            pt += QPoint(segLen/2 - width/2 + 1 , 3*segLen/2 + width);
            ppt = pt;
            DARK;
            LINETO(width,0);
            LINETO(width,-width);
            LIGHT;
            LINETO(0,-width);
            LINETO(0,0);
            break;
        default :
            qWarning("QLCDNumber::drawSegment: (%s) Illegal segment id: %d\n",
                     q->objectName().toLocal8Bit().constData(), segmentNo);
        }
        // End exact copy
        p.setPen(Qt::NoPen);
        p.setBrush(fgColor);
        p.drawPolygon(a);
        p.setBrush(Qt::NoBrush);

        pt = pos;
}
