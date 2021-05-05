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


def resize_frame_borders(img: npt.ArrayLike) -> npt.ArrayLike:
    """
    Resize a frame to 128x64 by resizing and then adding black borders.
    """

    height, width = img.shape[:2]
    scale = min(128 / width, 64 / height)
    img = cv2.resize(img, None, fx=scale, fy=scale, interpolation=cv2.INTER_NEAREST)
    height, width = img.shape[:2]
    if width < 128:
        border_width = (128 - width) // 2
        return cv2.copyMakeBorder(img, 0, 0, border_width, 128 - width - border_width, cv2.BORDER_CONSTANT)
    elif height < 64:
        border_height = (64 - height) // 2
        return cv2.copyMakeBorder(img, border_height, 64 - height - border_height, 0, 0, cv2.BORDER_CONSTANT)


def convert_frame(img: npt.ArrayLike) -> npt.ArrayLike:
    img = resize_frame_borders(img)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    _, mono = cv2.threshold(img, 127, 255, cv2.THRESH_BINARY)
    return mono


def show_processed_frames(filename: str) -> None:
    for frame in get_frames(filename, 2):
        cv2.imshow("converted frame", cv2.resize(convert_frame(frame), None, fx=4, fy=4, interpolation=cv2.INTER_NEAREST))
        if cv2.waitKey(0) & 0xFF == ord('q'):
            cv2.destroyAllWindows()
            break


if __name__ == "__main__":
    cap = cv2.VideoCapture("vidsrc.mp4")
    cap.set(cv2.CAP_PROP_POS_MSEC, 7800)
    success, img = cap.read()
    cap.release()
    frame = convert_frame(img)
    with open("img.bin", "wb") as f:
        contents = bytearray()
        for row in frame:
            b = 0
            l = 0
            for px in row:
                b <<= 1
                l += 1
                # Invert image
                if not px:
                    b |= 1
                if l == 8:
                    contents.append(b)
                    b = l = 0
        f.write(contents)
