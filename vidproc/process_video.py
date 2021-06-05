import cv2
import numpy as np
import numpy.typing as npt
from typing import Any, Iterable


def get_frames(path: str, framerate: int) -> Iterable[npt.ArrayLike]:
    count = 0
    cap = cv2.VideoCapture(path)
    sampling_delay = 1000 // framerate
    while True:
        cap.set(cv2.CAP_PROP_POS_MSEC, count * sampling_delay)
        success, img = cap.read()
        if not success:
            cap.release()
            return
        yield img
        count += 1


def resize_frame_crop(img: npt.ArrayLike) -> npt.ArrayLike:
    """
    Resize a frame to 128x64 by resizing and then cropping.
    """

    height, width = img.shape[:2]
    scale = max(128 / width, 64 / height)
    img = cv2.resize(img, None, fx=scale, fy=scale, interpolation=cv2.INTER_NEAREST)
    height, width = img.shape[:2]
    # Image too tall (usual case)
    if height > 64:
        start = (height - 64) // 2
        img = img[start:start + 64]
    # Image too wide
    else:
        start = (width - 128) // 2
        img = img[:, start:start + 128]
    return img


def resize_frame_borders(img: npt.ArrayLike, borders: bool = True) -> npt.ArrayLike:
    """
    Resize a frame to 128x64 by resizing and then adding black borders.
    """

    height, width = img.shape[:2]
    scale = min(128 / width, 64 / height)
    img = cv2.resize(img, None, fx=scale, fy=scale, interpolation=cv2.INTER_NEAREST)
    if borders:
        height, width = img.shape[:2]
        if width < 128:
            border_width = (128 - width) // 2
            return cv2.copyMakeBorder(img, 0, 0, border_width, 128 - width - border_width, cv2.BORDER_CONSTANT)
        elif height < 64:
            border_height = (64 - height) // 2
            return cv2.copyMakeBorder(img, border_height, 64 - height - border_height, 0, 0, cv2.BORDER_CONSTANT)
    return img


def convert_frame(img: npt.ArrayLike) -> npt.ArrayLike:
    img = resize_frame_borders(img, borders=False)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    _, mono = cv2.threshold(img, 127, 255, cv2.THRESH_BINARY)
    # Invert image
    return cv2.bitwise_not(mono)


def show_processed_frames(filename: str) -> None:
    for frame in get_frames(filename, 2):
        cv2.imshow("converted frame", cv2.resize(convert_frame(frame), None, fx=4, fy=4, interpolation=cv2.INTER_NEAREST))
        if cv2.waitKey(0) & 0xFF == ord('q'):
            cv2.destroyAllWindows()
            break


def encode_frame(frame: npt.ArrayLike) -> bytearray:
    contents = bytearray()
    for row in frame:
        b = 0
        l = 0
        for px in row:
            b <<= 1
            l += 1
            # Invert image
            if px:
                b |= 1
            if l == 8:
                contents.append(b)
                b = l = 0
    return contents


def encode_video_16(filename: str, fps: int, out):
    """
    Compress and write the video.
    """
    
    iter = get_frames(filename, fps)
    first_frame = convert_frame(next(iter))
    height, width = first_frame.shape
    out.write(width.to_bytes(1, byteorder="big"))
    out.write(height.to_bytes(1, byteorder="big"))
    # Framecount TODO
    # RLE the first frame in its entirety
    frame_bin = bytearray()
    val = None
    repeat = 0
    for row in first_frame:
        for px in row:
            if px != val or repeat >= 0x7F:
                if repeat:
                    frame_bin.append(0x80 | repeat if val else repeat)
                    repeat = 0
                val = px
            repeat += 1
    if repeat:
        frame_bin.append(0x80 | repeat if val else repeat)
    out.write(frame_bin)


if __name__ == "__main__":
    #encode_video_16("vidsrc.mp4", 10, open("video.bin", "wb"))
    # MAX_FRAMES = 200
    # frame_count = 0
    # with open("video.bin", "wb") as f:
    #     for frame in get_frames("vidsrc.mp4", 10):
    #         converted = convert_frame(frame)
    #         bframe = encode_frame(converted)
    #         f.write(bframe)
    #         frame_count += 1
    #         if frame_count >= MAX_FRAMES:
    #             break
    # print(frame_count, "frames written.")

    run_lengths = {}
    iter = get_frames("vidsrc.mp4", 10)
    frame_count = 0
    for frame in iter:
        frame = convert_frame(frame)
        val = None
        repeat = 0
        for row in frame:
            for px in row:
                if px != val:
                    if repeat:
                        if repeat in run_lengths:
                            run_lengths[repeat] += 1
                        else:
                            run_lengths[repeat] = 1
                        #frame_bin.append(0x80 | repeat if val else repeat)
                        repeat = 0
                    val = px
                repeat += 1
        if repeat:
            if repeat in run_lengths:
                run_lengths[repeat] += 1
            else:
                run_lengths[repeat] = 1
            #frame_bin.append(0x80 | repeat if val else repeat)
        frame_count += 1
        print("Processed frame", frame_count)
        if frame_count > 300:
            print(dict(sorted(run_lengths.items())))
            break
