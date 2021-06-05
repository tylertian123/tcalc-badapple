#include <string>
#include <iostream>
#include <fstream>
#include <cstdint>
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
    char buf;
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
void process_frame(cv::Mat in, cv::Mat out) {
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

/*
 * Compress & encode the video and write to the stream.
 *
 * This method divides the frame into 16 regions.
 */
void encode_video_16(cv::VideoCapture &cap, std::ostream &out) {
    cv::Mat frame;
    cv::Mat processed;
    // First frame is special
    cap.set(cv::CAP_PROP_POS_MSEC, 0);
    if (!cap.read(frame)) {
        std::cerr << "Cannot encode video: Nothing to read\n";
        return;
    }
    process_frame(frame, processed);
    // Write frame size
    uint8_t width = processed.cols;
    uint8_t height = processed.rows;
    out.put(width);
    out.put(height);

    // Run-length encode first frame
    bool val;
    unsigned int repeat = 0;

    for (unsigned int count = 1; ; count ++) {
        cap.set(cv::CAP_PROP_POS_MSEC, count * FRAME_INTERVAL);
        if (!cap.read(frame)) {
            std::cout << "All frames read.\n";
            return;
        }

        // cv::imshow("Processed frame", proc);
        // int key = cv::waitKey();
        // if (key == 'q') {
        //     std::cout << "Quit\n";
        //     return;
        // }
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

    encode_video_16(cap, out_file);
}
