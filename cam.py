from fcntl import ioctl
import mmap
import select
import v4l2
from ctypes import create_string_buffer, CDLL
lib = './capturemodule.so'
capmod = CDLL(lib)

_capture = capmod.capture
_frame_size = capmod.get_frame_size

def capture_frame(frame_count, width, height):
    buffer_size = _frame_size(width, height)
    buffer = create_string_buffer(buffer_size*frame_count)
    print('capturing video')
    _capture(frame_count, buffer, width, height)

    return buffer
