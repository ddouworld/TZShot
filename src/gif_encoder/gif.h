// gif.h - GIF encoder single-header library
// Based on the public domain work by Charlie Tangora (gif-h)
// Modified for Qt integration with RGBA transparency support
//
// Usage:
//   GifWriter g;
//   GifBegin(&g, "output.gif", width, height, delay, transparentIndex);
//   GifWriteFrame(&g, rgba_data, width, height, delay, 8, transparentIndex);
//   GifEnd(&g);
//
// delay is in units of 10ms (so delay=2 means 20ms = 50fps, delay=7 = 70ms ~15fps)

#ifndef GIF_H
#define GIF_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

// ----- Palette/Color quantization -----

static const int kGifTransIndex = 0;

struct GifPalette {
    int bitDepth;
    uint8_t r[256];
    uint8_t g[256];
    uint8_t b[256];
    // k-d tree over RGB space for fast nearest-color lookup
    uint8_t treeSplitElt[255];
    uint8_t treeSplit[255];
};

// walks the k-d tree to find the best-matching color
static int GifGetClosestColor(const GifPalette* pPal, int r, int g, int b) {
    int bestDiff = 1000000;
    int bestInd = 1;
    for (int ii = 1; ii < (1 << pPal->bitDepth); ++ii) {
        int dr = (int)pPal->r[ii] - r;
        int dg = (int)pPal->g[ii] - g;
        int db = (int)pPal->b[ii] - b;
        int diff = dr*dr + dg*dg + db*db;
        if (diff < bestDiff) {
            bestDiff = diff;
            bestInd = ii;
        }
    }
    return bestInd;
}

// simple median-cut palette generation
static void GifMakePalette(const uint8_t* lastFrame, const uint8_t* nextFrame,
                           uint32_t width, uint32_t height,
                           int bitDepth, bool buildForDither,
                           GifPalette* pPal) {
    pPal->bitDepth = bitDepth;

    // histogram
    int numColors = 1 << bitDepth;
    // Build a simple palette from the image data
    // For simplicity we use a fixed 6-6-6 color cube for index 1+
    // and reserve index 0 for transparency

    // 6x6x6 = 216 colors, fits in 8-bit palette (256 entries, index 0 = transparent)
    int ri = 0, gi = 0, bi = 0;
    for (int i = 1; i < numColors && i < 217; ++i) {
        pPal->r[i] = (uint8_t)(ri * 51);
        pPal->g[i] = (uint8_t)(gi * 51);
        pPal->b[i] = (uint8_t)(bi * 51);
        ri++;
        if (ri >= 6) { ri = 0; gi++; }
        if (gi >= 6) { gi = 0; bi++; }
    }
    // fill remaining with greyscale
    for (int i = 217; i < numColors; ++i) {
        int v = (i - 217) * 256 / (numColors - 217);
        pPal->r[i] = pPal->g[i] = pPal->b[i] = (uint8_t)v;
    }
    // index 0 is transparent (set to black)
    pPal->r[0] = pPal->g[0] = pPal->b[0] = 0;

    (void)lastFrame; (void)nextFrame; (void)width; (void)height; (void)buildForDither;
}

// ----- LZW Compression -----

static const int kGifMaxCodeLen = 12;

struct GifLzwNode {
    uint16_t m_next[256];
};

static void GifWriteCode(FILE* f, uint32_t& curCode, uint32_t& curCodeSize,
                         uint8_t* buf, uint32_t& bufLen,
                         uint32_t code) {
    curCode |= code << curCodeSize;
    curCodeSize += (uint32_t)(code >= (1ul << (curCodeSize - 1)) ? 1 : 0);
    // flush byte
    while (curCodeSize >= 8) {
        buf[bufLen++] = (uint8_t)(curCode & 0xff);
        curCode >>= 8;
        curCodeSize -= 8;
        if (bufLen == 255) {
            fputc(255, f);
            fwrite(buf, 1, 255, f);
            bufLen = 0;
        }
    }
}

// Full LZW encoder
static void GifLzwCompress(FILE* f, const uint8_t* image, uint32_t width, uint32_t height,
                            int bitDepth, int transparent) {
    int clearCode = 1 << bitDepth;
    int eofCode   = clearCode + 1;
    int maxCode   = eofCode + 1;
    int codeSize  = bitDepth + 1;

    fputc(bitDepth, f);  // LZW minimum code size

    // allocate trie
    GifLzwNode* codetree = (GifLzwNode*)calloc(4096, sizeof(GifLzwNode));

    uint32_t curCode = 0, curCodeSize = (uint32_t)codeSize;
    uint8_t buf[256];
    uint32_t bufLen = 0;

    // emit clear code
    curCode |= (uint32_t)clearCode << curCodeSize;
    curCodeSize += (uint32_t)codeSize;
    while (curCodeSize >= 8) {
        buf[bufLen++] = (uint8_t)(curCode & 0xff);
        curCode >>= 8; curCodeSize -= 8;
        if (bufLen == 255) { fputc(255, f); fwrite(buf, 1, 255, f); bufLen = 0; }
    }

    int curEntry = -1;
    for (uint32_t yy = 0; yy < height; ++yy) {
        for (uint32_t xx = 0; xx < width; ++xx) {
            const uint8_t* pixel = image + (yy * width + xx) * 4;
            uint8_t alpha = pixel[3];
            int nextValue;
            if (alpha < 128) {
                nextValue = kGifTransIndex; // transparent
            } else {
                // find best palette entry (skip index 0 which is transparent)
                int r = pixel[0], g = pixel[1], b = pixel[2];
                // Simple 6-6-6 cube lookup
                int ri2 = r / 51; if (ri2 > 5) ri2 = 5;
                int gi2 = g / 51; if (gi2 > 5) gi2 = 5;
                int bi2 = b / 51; if (bi2 > 5) bi2 = 5;
                nextValue = 1 + ri2 + gi2 * 6 + bi2 * 36;
                if (nextValue >= (1 << bitDepth)) nextValue = (1 << bitDepth) - 1;
                if (nextValue == kGifTransIndex) nextValue = 1;
            }

            if (curEntry < 0) {
                curEntry = nextValue;
                continue;
            }

            if (codetree[curEntry].m_next[nextValue]) {
                curEntry = codetree[curEntry].m_next[nextValue];
            } else {
                // emit code for curEntry
                int emitCode = curEntry;
                curCode |= (uint32_t)emitCode << curCodeSize;
                curCodeSize += (uint32_t)codeSize;
                while (curCodeSize >= 8) {
                    buf[bufLen++] = (uint8_t)(curCode & 0xff);
                    curCode >>= 8; curCodeSize -= 8;
                    if (bufLen == 255) { fputc(255, f); fwrite(buf, 1, 255, f); bufLen = 0; }
                }

                if (maxCode == (1 << codeSize)) codeSize++;

                if (maxCode == 4095) {
                    // emit clear code to reset
                    curCode |= (uint32_t)clearCode << curCodeSize;
                    curCodeSize += (uint32_t)codeSize;
                    while (curCodeSize >= 8) {
                        buf[bufLen++] = (uint8_t)(curCode & 0xff);
                        curCode >>= 8; curCodeSize -= 8;
                        if (bufLen == 255) { fputc(255, f); fwrite(buf, 1, 255, f); bufLen = 0; }
                    }
                    // reset trie
                    memset(codetree, 0, 4096 * sizeof(GifLzwNode));
                    maxCode = eofCode + 1;
                    codeSize = bitDepth + 1;
                } else {
                    codetree[curEntry].m_next[nextValue] = (uint16_t)maxCode++;
                }
                curEntry = nextValue;
            }
        }
    }

    // emit final code
    if (curEntry >= 0) {
        curCode |= (uint32_t)curEntry << curCodeSize;
        curCodeSize += (uint32_t)codeSize;
        while (curCodeSize >= 8) {
            buf[bufLen++] = (uint8_t)(curCode & 0xff);
            curCode >>= 8; curCodeSize -= 8;
            if (bufLen == 255) { fputc(255, f); fwrite(buf, 1, 255, f); bufLen = 0; }
        }
    }

    // emit EOF
    curCode |= (uint32_t)eofCode << curCodeSize;
    curCodeSize += (uint32_t)codeSize;
    while (curCodeSize >= 8) {
        buf[bufLen++] = (uint8_t)(curCode & 0xff);
        curCode >>= 8; curCodeSize -= 8;
        if (bufLen == 255) { fputc(255, f); fwrite(buf, 1, 255, f); bufLen = 0; }
    }

    // flush remaining bits
    if (curCodeSize > 0) {
        buf[bufLen++] = (uint8_t)(curCode & 0xff);
    }
    if (bufLen > 0) {
        fputc((int)bufLen, f);
        fwrite(buf, 1, bufLen, f);
    }

    fputc(0, f);  // block terminator

    free(codetree);
    (void)transparent;
}

// ----- GIF Writer -----

struct GifWriter {
    FILE* f;
    uint32_t width;
    uint32_t height;
    int      bitDepth;
    bool     firstFrame;
    int      transparentIndex;  // palette index reserved for transparency (-1 = none)
};

static bool GifBegin(GifWriter* writer, const char* filename,
                     uint32_t width, uint32_t height,
                     uint32_t delay,
                     int bitDepth = 8,
                     bool dither = false) {
    writer->f = fopen(filename, "wb");
    if (!writer->f) return false;

    writer->width  = width;
    writer->height = height;
    writer->bitDepth = bitDepth;
    writer->firstFrame = true;
    writer->transparentIndex = kGifTransIndex;

    // GIF header
    fputs("GIF89a", writer->f);

    // Logical screen descriptor
    uint16_t w16 = (uint16_t)width;
    uint16_t h16 = (uint16_t)height;
    fwrite(&w16, 2, 1, writer->f);
    fwrite(&h16, 2, 1, writer->f);

    // Global color table flag=0 (we use local), color res=7, sort=0, size=0
    fputc(0x70, writer->f);   // packed field: no global CT
    fputc(0,    writer->f);   // background color index
    fputc(0,    writer->f);   // pixel aspect ratio

    // Netscape loop extension (loop forever)
    fputc(0x21, writer->f);   // Extension introducer
    fputc(0xff, writer->f);   // Application extension
    fputc(11,   writer->f);   // block size
    fputs("NETSCAPE2.0", writer->f);
    fputc(3,    writer->f);   // sub-block size
    fputc(1,    writer->f);   // sub-block ID
    fputc(0,    writer->f);   // loop count low byte  (0 = infinite)
    fputc(0,    writer->f);   // loop count high byte
    fputc(0,    writer->f);   // block terminator

    (void)delay; (void)dither;
    return true;
}

static bool GifWriteFrame(GifWriter* writer, const uint8_t* image,
                          uint32_t width, uint32_t height,
                          uint32_t delay,
                          int bitDepth = 8,
                          bool dither = false) {
    if (!writer->f) return false;

    FILE* f = writer->f;
    GifPalette pal;
    GifMakePalette(nullptr, image, width, height, bitDepth, dither, &pal);

    // Graphic Control Extension (for transparency + delay)
    fputc(0x21, f);  // extension introducer
    fputc(0xf9, f);  // graphic control label
    fputc(0x04, f);  // block size

    // packed: reserved=0, disposal=1(leave in place), user input=0, transparent=1
    fputc(0x05, f);  // disposal=1, transparent flag=1

    uint16_t delay16 = (uint16_t)delay;
    fwrite(&delay16, 2, 1, f);   // delay in 1/100 sec units

    fputc((uint8_t)kGifTransIndex, f);  // transparent color index
    fputc(0, f);  // block terminator

    // Image descriptor
    fputc(0x2c, f);  // image separator
    uint16_t zero = 0;
    fwrite(&zero, 2, 1, f);   // left
    fwrite(&zero, 2, 1, f);   // top
    uint16_t w16 = (uint16_t)width;
    uint16_t h16 = (uint16_t)height;
    fwrite(&w16, 2, 1, f);
    fwrite(&h16, 2, 1, f);

    // local color table: flag=1, interlace=0, sort=0, reserved=0, size=(bitDepth-1)
    fputc(0x80 | (bitDepth - 1), f);

    // write palette
    for (int ii = 0; ii < (1 << bitDepth); ++ii) {
        fputc(pal.r[ii], f);
        fputc(pal.g[ii], f);
        fputc(pal.b[ii], f);
    }

    // compress image data
    GifLzwCompress(f, image, width, height, bitDepth, kGifTransIndex);

    writer->firstFrame = false;
    (void)dither;
    return true;
}

static bool GifEnd(GifWriter* writer) {
    if (!writer->f) return false;
    fputc(0x3b, writer->f);  // GIF trailer
    fclose(writer->f);
    writer->f = nullptr;
    return true;
}

#endif // GIF_H
