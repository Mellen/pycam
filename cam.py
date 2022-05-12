from fcntl import ioctl
import mmap
import select
import v4l2
import pathlib
from ctypes import create_string_buffer, CDLL
lib = str(pathlib.Path(__file__).parent.resolve())+'/capturemodule.so'
capmod = CDLL(lib)

_capture = capmod.capture
_frame_size = capmod.get_frame_size

def capture_video(frame_count, width, height):
    buffer_size = _frame_size(width, height)
    buffer = create_string_buffer(buffer_size*frame_count)
    print('capturing video')
    _capture(frame_count, buffer, width, height)

    return buffer

def capture_n_frames(frame_count, width, height, callback):
    open_device()
      
    buffer_size = init_device(width, height)
    buffer = create_string_buffer(buffer_size*frame_count)

    print('capturing video')
    
    start_capturing()

    for i in range(frame_count):
        fill_buffer(buffer, 1, frame_size)
        callback(buffer)
    
    stop_capturing()
    uninit_device()
    close_device()


