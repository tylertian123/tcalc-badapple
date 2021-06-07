#include "decoder.h"

#include <algorithm>

constexpr uint8_t CHUNK_COUNT_X = 8;
constexpr uint8_t CHUNK_COUNT_Y = 8;
constexpr uint8_t CHUNK_COUNT = CHUNK_COUNT_Y * CHUNK_COUNT_X;

VideoDecoder::VideoDecoder() : FRAME_WIDTH(read_bits(8)), FRAME_HEIGHT(read_bits(8)),
    FRAME_OFFSET_X((128 - FRAME_WIDTH) / 2), FRAME_OFFSET_Y((64 - FRAME_HEIGHT) / 2),
    CHUNK_WIDTH((FRAME_WIDTH - 1) / CHUNK_COUNT_X + 1), CHUNK_HEIGHT((FRAME_HEIGHT - 1) / CHUNK_COUNT_Y + 1) {}

bool VideoDecoder::read_bit() {
    bool bit = viddata[byte_idx] & (1 << --bit_idx);
    if (bit_idx == 0) {
        bit_idx = 8;
        byte_idx ++;
    }
    return bit;
}

uint64_t VideoDecoder::read_bits(uint8_t count) {
    uint64_t out = 0;
    for (uint8_t i = 0; i < count; i ++) {
        out = out << 1 | read_bit();
    }
    return out;
}

uint16_t VideoDecoder::read_repeat_count() {
    static const uint16_t OFFSETS[] = {
		0, 8, 72, 584, 4680, 37448
	};
	static const uint8_t GROUP_SIZE = 3;
    // Find number of groups first
    uint8_t groups = 1;
    while (read_bit()) {
        groups ++;
    }
    // Read the right number of bits
    uint16_t repeat;
    read_bits(groups * GROUP_SIZE, repeat);
    // Add the correct offsets
    return repeat + OFFSETS[groups - 1] + 1;
}

bool VideoDecoder::read_frame(uint16_t frame[32][16]) {
    // Read the entire thing for the first frame
    if (first_frame) {
        first_frame = false;
        return read_frame(frame, ~0ull);
    }
    else {
        // Read the frame header
        uint64_t header;
        if (!read_bits(CHUNK_COUNT, header)) {
            return false;
        }
        return read_frame(frame, header);
    }
}

bool VideoDecoder::read_frame(uint16_t frame[32][16], uint64_t header) {
    // Return if no chunks changed
    if (!header) {
        return true;
    }
    // Read starting bit value and first repeat count
    bool current = read_bit();
    uint16_t repeat = read_repeat_count();

    for (uint8_t x = 0; x < FRAME_WIDTH; x ++) {
        for (uint8_t y = 0; y < FRAME_HEIGHT; y ++) {
            // Skip chunks
            if (y % CHUNK_HEIGHT == 0) {
                uint8_t cx = std::min(x / CHUNK_WIDTH, CHUNK_COUNT_X - 1);
                uint8_t cy = std::min(y / CHUNK_HEIGHT, CHUNK_COUNT_Y - 1);
                // Check that the chunk is changed
                if (!(header & (1ull << (cx * CHUNK_COUNT_Y + cy)))) {
                    // Offset 1 for the loop
                    y += CHUNK_HEIGHT - 1;
                    continue;
                }
            }
            // Get new repeat value if needed
            if (repeat == 0) {
                current = !current;
                repeat = read_repeat_count();
            }
            repeat --;

            // Update LCD pixel
            uint8_t lcd_row = y + FRAME_OFFSET_Y;
            uint8_t lcd_col = (x + FRAME_OFFSET_X) / 16;
            uint8_t lcd_col_offset = 15 - (x + FRAME_OFFSET_X) % 16;
            // Bottom 32 rows
            if (lcd_row >= 32) {
                lcd_row -= 32;
                lcd_col += 128 / 16;
            }
            if (current) {
                frame[lcd_row][lcd_col] |= 1 << lcd_col_offset;
            }
            else {
                frame[lcd_row][lcd_col] &= ~(1 << lcd_col_offset);
            }
        }
    }
    return true;
}
