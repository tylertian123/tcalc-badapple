#include <iostream>
#include <fstream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <type_traits>
#include <stdint.h>

#include "common.h"

#define SHOW_UNCHANGED_REGIONS

struct bit_reader {
	bit_reader(std::istream& from) : from(from) {
		val = from.get();
	}

	bool operator()() {
		bool result = val & (1 << --idx);
		if (idx == 0) {
			idx = 8;
			val = from.get();
		}
		return result;
	}
private:
	std::istream &from;
	uint8_t val; size_t idx = 8;
};

size_t read_num(size_t length, bit_reader& from) {
	size_t val = 0;
	for (int i = 0; i < length; ++i) {
		val = (val << 1) | (from() ? 1 : 0);
	}
	return val;
}

size_t read_count(bit_reader& from) {
	static const size_t OFFSETS[] = {
		0, 8, 72, 584, 4680, 37448
	};
	static const size_t GROUP_SIZE = 3;

	size_t groups = 0;
	do {
		groups++;
	} while (from() != 0);

	size_t result = read_num(groups*GROUP_SIZE, from) + 
		OFFSETS[groups - 1] + 1;
	return result;
}

int main(int argc, char ** argv) {
	if (argc < 2) {
		std::cerr << "Please provide a filename.\n";
		return 1;
	}

	std::ifstream in_file(argv[1], std::ios::binary);
	
	// Read the frame size
	size_t width = in_file.get();
	size_t height = in_file.get();

	// Setup a bit reader
	bit_reader br(in_file);
	
	// Setup chunk size
    const unsigned int CHUNK_WIDTH = (width - 1) / CHUNK_COUNT_X + 1;
    const unsigned int CHUNK_HEIGHT = (height - 1) / CHUNK_COUNT_Y + 1;

	std::cout << "h " << height << " w " << width << " ch " << CHUNK_HEIGHT << " cw " << CHUNK_WIDTH << "\n";

	// Setup a buffer
	cv::Mat frame = cv::Mat(height, width, CV_8UC1);
	cv::Mat framescaled;

	// Make a helper for reading a frame
	auto read_frame = [&](size_t cmask){
		if (!cmask) return;

		bool current = br();
		size_t repeat = read_count(br);

#ifdef SHOW_UNCHANGED_REGIONS
		for (unsigned int x = 0; x < width; ++x) {
			for (unsigned int y = 0; y < height; ++y) {
				if (frame.at<uint8_t>(y, x) == 0x00) frame.at<uint8_t>(y, x) = 0x28;
				else if (frame.at<uint8_t>(y, x) == 0xff) frame.at<uint8_t>(y, x) = 0xc8;
			}
		}
#endif

		for (unsigned int x = 0; x < width; ++x) {
			for (unsigned int y = 0; y < height; ++y) {
				// Skip chunks
                if (y % CHUNK_HEIGHT == 0) {
                    unsigned int cx = std::min(x / CHUNK_WIDTH, CHUNK_COUNT_X - 1);
                    unsigned int cy = std::min(y / CHUNK_HEIGHT, CHUNK_COUNT_Y - 1);
                    // Check that the chunk is changed
                    if (!(cmask & (1ull << (cx * CHUNK_COUNT_Y + cy)))) {
                        // Offset 1 for the loop
                        y += CHUNK_HEIGHT - 1;
                        continue;
                    }
                }
				if (!repeat) {
					// update next
					current = !current;
					repeat = read_count(br);
				}
				// Consume repeat
				repeat--;
				frame.at<uint8_t>(y, x) = current ? 0x00 : 0xff;
			}
		}
	};

	// Read the first frame
	read_frame(~0ull); // all chunks

	// Show frame
	cv::resize(frame, framescaled, cv::Size(), 4, 4, cv::INTER_NEAREST);
	cv::imshow("img", framescaled);
	// Wait
	cv::waitKey(FRAME_INTERVAL);
	// Show all frames
	while (in_file) {
		read_frame(read_num(CHUNK_COUNT_X * CHUNK_COUNT_Y, br));
		// Show frame
		cv::resize(frame, framescaled, cv::Size(), 4, 4, cv::INTER_NEAREST);
		cv::imshow("img", framescaled);
		// Wait
		cv::waitKey(FRAME_INTERVAL);
	}
}
