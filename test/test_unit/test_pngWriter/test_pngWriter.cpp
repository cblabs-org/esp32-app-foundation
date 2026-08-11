#ifndef ARDUINO
#include <cstdlib>
struct tdefl_compressor {};
typedef int tdefl_status;
#define TDEFL_STATUS_OKAY 0
#define TDEFL_STATUS_DONE 1
#define TDEFL_DEFAULT_MAX_PROBES 128
#define TDEFL_FINISH 0
#define TDEFL_NO_FLUSH 1
#define MALLOC_CAP_SPIRAM 0
static void* heap_caps_malloc(size_t size, uint32_t) { return malloc(size); }
static tdefl_status tdefl_init(tdefl_compressor *, int (*)(const void *, int, void *), void *, int) { return TDEFL_STATUS_OKAY; }
static tdefl_status tdefl_compress_buffer(tdefl_compressor *, const void *, size_t, int) { return TDEFL_STATUS_OKAY; }
#endif
#include <gtest/gtest.h>
#include <vector>

#include "../../../lib/PngWriter/PngWriter.h"
#include "../../../lib/PngWriter/PngWriter.cpp"



// Mock Print class for native environment
class TestPrintSink : public Print {
public:
    std::vector<uint8_t> buffer;
    size_t write(uint8_t c) override {
        buffer.push_back(c);
        return 1;
    }
    size_t write(const uint8_t* buf, size_t size) override {
        buffer.insert(buffer.end(), buf, buf + size);
        return size;
    }
};

static const uint8_t PNG_SIG[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

TEST(PngWriterTest, SignatureTest) {
    TestPrintSink sink;
    uint8_t buf[2] = {0, 0}; // 4x2 needs 1 byte per row => 2 bytes total
    EXPECT_TRUE(PngWriter::writeGrayscale1bpp(sink, 4, 2, buf));
    EXPECT_GE(sink.buffer.size(), sizeof(PNG_SIG));
    for(size_t i=0; i<sizeof(PNG_SIG); ++i) {
        EXPECT_EQ(sink.buffer[i], PNG_SIG[i]);
    }
}

TEST(PngWriterTest, IHDRTest) {
    TestPrintSink sink;
    uint8_t buf[2] = {0, 0}; 
    EXPECT_TRUE(PngWriter::writeGrayscale1bpp(sink, 4, 2, buf));
    
    // IHDR should start at byte 8
    EXPECT_GE(sink.buffer.size(), 33);
    
    // Length (4 bytes) should be 13
    EXPECT_EQ(sink.buffer[8], 0);
    EXPECT_EQ(sink.buffer[9], 0);
    EXPECT_EQ(sink.buffer[10], 0);
    EXPECT_EQ(sink.buffer[11], 13);
    
    // Type (4 bytes) "IHDR"
    EXPECT_EQ(sink.buffer[12], 'I');
    EXPECT_EQ(sink.buffer[13], 'H');
    EXPECT_EQ(sink.buffer[14], 'D');
    EXPECT_EQ(sink.buffer[15], 'R');
    
    // Width (4 bytes)
    EXPECT_EQ(sink.buffer[19], 4); // width=4
    // Height (4 bytes)
    EXPECT_EQ(sink.buffer[23], 2); // height=2
    
    // Bit depth
    EXPECT_EQ(sink.buffer[24], 1);
    // Color type
    EXPECT_EQ(sink.buffer[25], 0);
}

TEST(PngWriterTest, TileVariantTest) {
    TestPrintSink sink;
    uint8_t tileBuf[4] = {0, 0, 0, 0}; 
    EXPECT_TRUE(PngWriter::writeGrayscale1bppFromTiles(sink, 4, 2, tileBuf));
    
    EXPECT_GE(sink.buffer.size(), sizeof(PNG_SIG));
    for(size_t i=0; i<sizeof(PNG_SIG); ++i) {
        EXPECT_EQ(sink.buffer[i], PNG_SIG[i]);
    }
    
    EXPECT_EQ(sink.buffer[19], 4);
    EXPECT_EQ(sink.buffer[23], 2);
    EXPECT_EQ(sink.buffer[24], 1);
    EXPECT_EQ(sink.buffer[25], 0);
}

TEST(PngWriterTest, TileVsRowMajorEquivalence) {
    // 8x8 image
    // Let's create a known pattern
    uint8_t rowBuf[8];
    for (int i = 0; i < 8; i++) {
        rowBuf[i] = i; // just some pattern
    }
    
    // Convert to tile buf manually
    uint8_t tileBuf[8] = {0};
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            bool pixel = (rowBuf[y] >> (7 - (x % 8))) & 1;
            if (pixel) {
                tileBuf[x] |= (1 << (y % 8));
            }
        }
    }
    
    TestPrintSink sink1;
    EXPECT_TRUE(PngWriter::writeGrayscale1bpp(sink1, 8, 8, rowBuf));
    
    TestPrintSink sink2;
    EXPECT_TRUE(PngWriter::writeGrayscale1bppFromTiles(sink2, 8, 8, tileBuf));
    
    EXPECT_EQ(sink1.buffer.size(), sink2.buffer.size());
    for(size_t i = 0; i < sink1.buffer.size(); i++) {
        EXPECT_EQ(sink1.buffer[i], sink2.buffer[i]);
    }
}

TEST(PngWriterTest, StoredBlockGuard) {
    TestPrintSink sink;
    // max size is 65535 for dataLen
    // rowBytes = (width + 7) / 8
    // dataLen = (rowBytes + 1) * height
    // We want dataLen > 65535
    // let's do width = 800, rowBytes = 100
    // (100 + 1) * 700 = 70700 > 65535
    uint8_t dummy[1] = {0};
    EXPECT_FALSE(PngWriter::writeGrayscale1bpp(sink, 800, 700, dummy));
    EXPECT_FALSE(PngWriter::writeGrayscale1bppFromTiles(sink, 800, 700, dummy));
}

// Note: RGB path is not tested in native because it relies on tdefl which is a ROM dependency.


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
