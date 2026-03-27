#ifndef MAGNIFIER_WIDGET_H
#define MAGNIFIER_WIDGET_H

#include <QColor>
#include <QPixmap>
#include <QRect>
#include <QWidget>

class MagnifierWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MagnifierWidget(QWidget *parent = nullptr);

    void setSnapshot(const QPixmap &snapshot, const QRect &virtualGeometry);
    void updateView(const QPoint &localPoint, const QPoint &globalPoint, const QColor &pixelColor, const QSize &overlaySize);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap m_snapshot;
    QRect m_virtualGeometry;
    QPoint m_localPoint;
    QPoint m_globalPoint;
    QColor m_pixelColor = Qt::black;
};

#endif // MAGNIFIER_WIDGET_H
