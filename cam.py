from fcntl import ioctl
import mmap
import select
import v4l2
from ctypes import POINTER, create_string_buffer, CDLL
lib = './capturemodule.so'
capmod = CDLL(lib)

_capture = capmod.capture
_frame_size = capmod.get_frame_size

def capture_frame():
    width, height = 1280, 720
    frame_count = 60
    with open('output.raw', 'wb') as vidfile:
        buffer_size = _frame_size(width, height)
        buffer = create_string_buffer(buffer_size*frame_count)
        _capture(frame_count, buffer, width, height)
        vidfile.write(buffer)
