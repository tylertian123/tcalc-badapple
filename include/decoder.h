#pragma once

#include <stdint.h>

extern "C" const uint8_t viddata[];
extern "C" const uint32_t viddata_size;

class VideoDecoder {

    uint32_t byte_idx = 0;
    uint8_t bit_idx = 8;

    const uint8_t FRAME_WIDTH, FRAME_HEIGHT;
    const uint8_t FRAME_OFFSET_X, FRAME_OFFSET_Y;
    const uint8_t CHUNK_WIDTH, CHUNK_HEIGHT;

    bool first_frame = true;

    // Read the next bit from the video data
    // Does not perform range checks
    bool read_bit();

    // Read count bits from the video data into out
    // Performs a range check; returns false if there is not enough data
    template <typename T>
    bool read_bits(uint8_t count, T &out) {
        out = 0;
        for (uint8_t i = 0; i < count; i ++) {
            out = out << 1 | read_bit();
            // Check if we've gone past the end
            if (byte_idx >= viddata_size && i < count - 1) {
                return false;
            }
        }
        return true;
    }
    // The no-range-check version of the other one
    uint64_t read_bits(uint8_t count);

    // Read a bit repeat count from the video data
    // Does not perform range checks (relies on frame headers being at least 8 bits long)
    uint16_t read_repeat_count();

public:

    VideoDecoder();

    bool read_frame(uint8_t frame[64][16], uint64_t header);
    // Read a frame and update the frame buffer
    // Returns false if there is no more data
    bool read_frame(uint8_t frame[64][16]);
};
