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

bool VideoDecoder::read_frame(uint8_t frame[64][16]) {
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

void set_pixel(uint8_t frame[64][16], uint8_t x, uint8_t y, bool val) {
    uint8_t lcd_row = y;
    uint8_t lcd_col = x / 8;
    uint8_t lcd_col_offset = 7 - x % 8;
    if (val) {
        frame[lcd_row][lcd_col] |= 1 << lcd_col_offset;
    }
    else {
        frame[lcd_row][lcd_col] &= ~(1 << lcd_col_offset);
    }
}

bool VideoDecoder::read_frame(uint8_t frame[64][16], uint64_t header) {
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
            set_pixel(frame, x + FRAME_OFFSET_X, y + FRAME_OFFSET_Y, current);
        }
    }
    // Find border colours
    uint8_t l0 = 0, l1 = 0, r0 = 0, r1 = 0;
    for (uint8_t y = 0; y < FRAME_HEIGHT; y ++) {
        (frame[y + FRAME_OFFSET_Y][FRAME_OFFSET_X / 8] & 1 << (7 - FRAME_OFFSET_X % 8) ? l1 : l0) ++;
        (frame[y + FRAME_OFFSET_Y][(FRAME_OFFSET_X + FRAME_WIDTH - 1) / 8] & 1 << (7 - (FRAME_OFFSET_X + FRAME_WIDTH - 1) % 8) ? r1 : r0) ++;
    }
    const uint8_t RIGHT_OFFSET = 128 - FRAME_OFFSET_X - FRAME_WIDTH;
    if (std::abs(l1 - l0) >= 10) {
        for (uint8_t row = 0; row < 64; row ++) {
            for (uint8_t col = 0; col < FRAME_OFFSET_X / 8; col ++) {
                frame[row][col] = l1 > l0 ? 0xFF : 0x00;
            }
            const uint8_t mask = 0xFF << (8 - FRAME_OFFSET_X % 8);
            if (l1 > l0) {
                frame[row][FRAME_OFFSET_X / 8] |= mask;
            }
            else {
                frame[row][FRAME_OFFSET_X / 8] &= ~mask;
            }
        }
    }
    if (std::abs(r1 - r0) >= 10) {
        for (uint8_t row = 0; row < 64; row ++) {
            for (uint8_t col = 16 - RIGHT_OFFSET / 8; col < 16; col ++) {
                frame[row][col] = r1 > r0 ? 0xFF : 0x00;
            }
            const uint8_t mask = 0xFF >> (8 - RIGHT_OFFSET % 8);
            if (r1 > r0) {
                frame[row][(FRAME_OFFSET_X + FRAME_WIDTH + 1) / 8] |= mask;
            }
            else {
                frame[row][(FRAME_OFFSET_X + FRAME_WIDTH + 1) / 8] &= ~mask;
            }
        }
    }
    return true;
}
