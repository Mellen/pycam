import pathlib
import time
from ctypes import create_string_buffer, CDLL

lib = './capturemodule.so'
try:
    lib = str(pathlib.Path(__file__).parent.resolve())+'/capturemodule.so'
except NameError:
    pass
capmod = CDLL(lib)

_capture = capmod.capture
_frame_size = capmod.get_frame_size
_open_device = capmod.open_device
_init_device = capmod.init_device
_start_capturing = capmod.start_capturing
_fill_buffer = capmod.fill_buffer
_stop_capturing = capmod.stop_capturing
_uninit_device = capmod.uninit_device
_close_device = capmod.close_device

def capture_video(frame_count, width, height):
    buffer_size = _frame_size(width, height)
    buffer = create_string_buffer(buffer_size*frame_count)
    print('capturing video')
    _capture(frame_count, buffer, width, height)

    return buffer

def capture_n_frames(frame_count, width, height, callback):
    _open_device()
      
    buffer_size = _init_device(width, height)
    buffer = create_string_buffer(buffer_size)

    print('capturing video')
    
    _start_capturing()

    for i in range(frame_count):
        _fill_buffer(buffer, 1, buffer_size)
        callback(buffer, i)
    
    _stop_capturing()
    _uninit_device()
    _close_device()

def capture_frames_for_time(seconds, width, height, callback):
    _open_device()
      
    buffer_size = _init_device(width, height)
    buffer = create_string_buffer(buffer_size)

    print('capturing video')
    
    _start_capturing()

    start_time = time.time()
    next_time = time.time()

    count = 0
    
    while (next_time - start_time) <= seconds:
        count += 1
        _fill_buffer(buffer, 1, buffer_size)
        callback(buffer, count)
        next_time = time.time()
    
    _stop_capturing()
    _uninit_device()
    _close_device()
