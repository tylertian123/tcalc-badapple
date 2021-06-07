#pragma once

constexpr inline unsigned int SCREEN_WIDTH = 128;
constexpr inline unsigned int SCREEN_HEIGHT = 64;

constexpr inline unsigned int FRAMERATE = 12;
constexpr inline unsigned int FRAME_INTERVAL = 1000 / FRAMERATE;

constexpr inline unsigned int CHUNK_COUNT_X = 8;
constexpr inline unsigned int CHUNK_COUNT_Y = 8;
constexpr inline unsigned int CHUNK_COUNT = CHUNK_COUNT_Y * CHUNK_COUNT_X;

constexpr inline unsigned int FRAME_DIFF_PCT = 8;
constexpr inline unsigned int FRAME_CONST_FACTOR = 5;
