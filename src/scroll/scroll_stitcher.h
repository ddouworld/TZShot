#ifndef SCROLL_STITCHER_H
#define SCROLL_STITCHER_H

#include <QImage>

class ScrollStitcher
{
public:
    void reset();
    bool hasBase() const;

    void setBaseFrame(const QImage &frame);

    // Returns appended pixel height. 0 means no new area appended.
    int appendFrame(const QImage &frame);

    QImage result() const { return m_result; }

private:
    int findBestOverlap(const QImage &prev, const QImage &curr) const;
    static int grayAt(const QImage &img, int x, int y);

    QImage m_lastFrame;
    QImage m_result;
};

#endif // SCROLL_STITCHER_H
