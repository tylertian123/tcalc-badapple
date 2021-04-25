import cv2
import numpy as np
import numpy.typing as npt
from typing import Any, Iterable


def get_frames(path: str, framerate: int) -> Iterable[npt.ArrayLike]:
    count = 0
    cap = cv2.VideoCapture(path)

    while True:
        cap.set(cv2.CAP_PROP_POS_MSEC, count * 1000 / framerate)
        success, img = cap.read()
        if not success:
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


def convert_frame(img: npt.ArrayLike) -> npt.ArrayLike:
    img = resize_frame_borders(img)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    _, mono = cv2.threshold(img, 127, 255, cv2.THRESH_BINARY)
    return mono


def convert_frame2(img: npt.ArrayLike) -> npt.ArrayLike:
    img = resize_frame_crop(img)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    _, mono = cv2.threshold(img, 127, 255, cv2.THRESH_BINARY)
    return mono


for frame in get_frames("vidsrc.mp4", 2):
    cv2.imshow("borders", cv2.resize(convert_frame(frame), None, fx=4, fy=4, interpolation=cv2.INTER_NEAREST))
    cv2.imshow("crop", cv2.resize(convert_frame2(frame), None, fx=4, fy=4, interpolation=cv2.INTER_NEAREST))
    if cv2.waitKey(0) & 0xFF == ord('q'):
        cv2.destroyAllWindows()
        break
