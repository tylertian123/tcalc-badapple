#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

constexpr unsigned int SCREEN_WIDTH = 128;
constexpr unsigned int SCREEN_HEIGHT = 64;

constexpr unsigned int FRAMERATE = 10;
constexpr unsigned int FRAME_INTERVAL = 1000 / FRAMERATE;

// Util class for writing individual bits
class BitStream {
    std::ostream &stream;
    uint8_t buf;
    int count;

public:
    BitStream(std::ostream &stream) : stream(stream), buf(0), count(0) {}

    ~BitStream() {
        stream.put(buf << (8 - count));
    }

    void write(bool bit) {
        buf = (buf << 1) | bit;
        if (++count == 8) {
            stream.put(buf);
            buf = count = 0;
        }
    }

    BitStream& operator<<(bool bit) {
        write(bit);
        return *this;
    }
};

// Util class for run-length encoding bits and writing to a stream.
class RunLengthEncoder {
    BitStream &stream;
    bool val;
    int repeat;

private:
    static constexpr int GROUP_SIZE = 3;
    static const int OFFSETS[];

    void flush() {
        if (repeat < 1) {
            return;
        }
        // A repeat of zero is not possible, so everything is offset by 1
        repeat --;
        // Calculate the number of bits needed
        int groups;
        // Keep increasing the group count
        // while the repeat count is still greater than the limit of the NEXT group count
        for (groups = 1; repeat >= OFFSETS[groups]; groups ++);
        // Subtract the correct offset
        repeat -= OFFSETS[groups - 1];
        // Write the bits
        for (int i = 0; i < groups - 1; i ++) {
            stream << 1;
        }
        stream << 0;
        int mask = 1 << (groups * GROUP_SIZE);
        while (mask >>= 1) {
            stream << (repeat & mask);
        }

        // Flip current value and reset counter
        repeat = 0;
        val = !val;
    }

public:
    RunLengthEncoder(BitStream &stream) : stream(stream), val(false), repeat(-1) {}

    ~RunLengthEncoder() {
        flush();
    }

    void encode(bool bit) {
        // Starting value not set -- write it
        if (repeat == -1) {
            stream << bit;
            val = bit;
            repeat = 1;
        }
        else {
            if (bit != val) {
                flush();
            }
            repeat ++;
        }
    }

    RunLengthEncoder& operator<<(bool bit) {
        encode(bit);
        return *this;
    }
};

const int RunLengthEncoder::OFFSETS[] = {
    0, 8, 72, 584, 4680, 37448
};

/*
 * Perform the necessary resizing and conversion on a frame.
 */
void process_frame(cv::Mat &in, cv::Mat &out) {
    // Resize
    cv::Mat temp;
    int width = in.cols;
    int height = in.rows;
    double scale = std::min(static_cast<double>(SCREEN_WIDTH) / width, static_cast<double>(SCREEN_HEIGHT) / height);
    cv::resize(in, out, cv::Size(), scale, scale, cv::INTER_NEAREST);

    // Convert to monochrome
    cv::cvtColor(out, temp, cv::COLOR_BGR2GRAY);
    cv::threshold(temp, out, 127, 255, cv::THRESH_BINARY_INV);
}

constexpr unsigned int CHUNK_COUNT_X = 8;
constexpr unsigned int CHUNK_COUNT_Y = 8;

/*
 * Compress & encode the video and write to the stream.
 *
 * This method divides the frame into regions.
 */
void encode_video(cv::VideoCapture &cap, std::ostream &out, int frame_limit = std::numeric_limits<int>::max()) {
    cv::Mat frame;
    cv::Mat processed;
    cv::Mat previous;
    // First frame is special
    cap.set(cv::CAP_PROP_POS_MSEC, 0);
    if (!cap.read(frame)) {
        std::cerr << "Cannot encode video: Nothing to read\n";
        return;
    }
    process_frame(frame, processed);
    // Write frame size
    const unsigned int fwidth = processed.cols;
    const unsigned int fheight = processed.rows;
    out.put(fwidth);
    out.put(fheight);
    // Find the chunk size
    // Divide and round up; the right & bottom chunks are a little smaller
    // Makes the code a little cleaner later on
    const unsigned int CHUNK_WIDTH = (fwidth - 1) / CHUNK_COUNT_X + 1;
    const unsigned int CHUNK_HEIGHT = (fheight - 1) / CHUNK_COUNT_Y + 1;

    BitStream out_bits(out);
    // Run-length encode first frame
    {
        RunLengthEncoder encoder(out_bits);
        for (auto it = processed.begin<uint8_t>(); it != processed.end<uint8_t>(); ++it) {
            encoder << static_cast<bool>(*it);
        }
    }

    // Keep track of previous frame to do frame diffs
    previous = processed;
    for (unsigned int count = 1; ; count ++) {
        // Hard stop for debugging purposes
        if (count > frame_limit) {
            std::cout << "Frame limit reached.\n";
            return;
        }

        if (count % 50 == 0) {
            std::cout << "Encoded " << (static_cast<double>(count) / FRAMERATE) << " seconds\n";
        }

        cap.set(cv::CAP_PROP_POS_MSEC, count * FRAME_INTERVAL);
        if (!cap.read(frame)) {
            std::cout << "All frames read.\n";
            return;
        }
        process_frame(frame, processed);

        // Find the chunks that changed
        uint32_t mask = 1;
        uint32_t changed_chunks = 0;
        for (unsigned int cx = 0; cx < CHUNK_COUNT_X; cx ++) {
            for (unsigned int cy = 0; cy < CHUNK_COUNT_Y; cy ++) {
                for (unsigned int x = cx * CHUNK_WIDTH; x < std::min((cx + 1) * CHUNK_WIDTH, fwidth); x ++) {
                    for (unsigned int y = cy * CHUNK_HEIGHT; y < std::min((cy + 1) * CHUNK_HEIGHT, fheight); y ++) {
                        if (static_cast<bool>(processed.at<uint8_t>(y, x)) != static_cast<bool>(previous.at<uint8_t>(y, x))) {
                            changed_chunks |= mask;
                            goto next_chunk;
                        }
                    }
                }
next_chunk:
                mask <<= 1;
            }
        }

        // Write frame header
        uint32_t header = changed_chunks;
        for (unsigned int i = 0; i < CHUNK_COUNT_X * CHUNK_COUNT_Y; i ++) {
            out_bits << (header & 1);
            header >>= 1;
        }

        // Encode the frame, skipping unchanged chunks
        RunLengthEncoder encoder(out_bits);
        for (unsigned int x = 0; x < fheight; x ++) {
            for (unsigned int y = 0; y < fheight; y ++) {
                // Entered new chunk
                if (y % CHUNK_HEIGHT == 0) {
                    unsigned int cx = std::min(x / CHUNK_WIDTH, CHUNK_COUNT_X - 1);
                    unsigned int cy = std::min(y / CHUNK_HEIGHT, CHUNK_COUNT_Y - 1);
                    // Check that the chunk is changed
                    if (!(changed_chunks & (1 << (cx * CHUNK_COUNT_Y + cy)))) {
                        // Skip chunk if unchanged
                        // Offset 1 for the loop
                        y += CHUNK_HEIGHT - 1;
                        continue;
                    }
                }
                encoder << static_cast<bool>(processed.at<uint8_t>(y, x));
            }
        }

        previous = processed;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Please provide a filename.\n";
        return 1;
    }
    std::cout << "Using file " << argv[1] << "\n";
    std::string out_filename = argc > 2 ? argv[2] : "video.bin";
    std::cout << "Outputting to file " << out_filename << " (pass command line argument to override)\n";
    int frame_limit = std::numeric_limits<int>::max();
    if (argc > 3) {
        frame_limit = std::atoi(argv[3]);
        std::cout << "Only encoding the first " << frame_limit << " frames.\n";
    }

    cv::VideoCapture cap;
    if (!cap.open(argv[1], cv::CAP_ANY)) {
        std::cerr << "Can't open video source file.\n";
        return 1;
    }

    std::ofstream out_file;
    out_file.open(out_filename, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
    if (!out_file) {
        std::cerr << "Can't open output file for writing.\n";
        return 1;
    }

    encode_video(cap, out_file, frame_limit);
    std::cout << "Done.\n";
}
