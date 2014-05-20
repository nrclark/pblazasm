#ifndef QMTXLCDDISPLAY_H
#define QMTXLCDDISPLAY_H

#include <QGraphicsScene>
#include <QPolygon>
#include <QPoint>
#include <QPalette>

class QmtxLCDDisplay : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit QmtxLCDDisplay(QObject *parent = 0);

signals:

public slots:

private:
    void drawSegment(const QPoint & pos, char segmentNo, QPainter & p, int segLen, bool erase);

};

#endif // QMTXLCDDISPLAY_H
