from fcntl import ioctl
import mmap
import select
import v4l2
from ctypes import POINTER, create_string_buffer, pointer, c_void_p

class buffer_location():
    def __init__(self):
        self.start = v4l2.c_int(0)
        self.length = 0

def read_frame():
    return True
        
def capture_frame():
    with open('/dev/video0', 'rb') as cam, open('output.raw', 'wb+') as vidfile:
        index = v4l2.c_int(0)
        
        if -1 == ioctl(cam, v4l2.VIDIOC_S_INPUT, index):
            print('failed to switch to first video input')
        else:
            print('index of video input:', index.value)


        capability = v4l2.v4l2_capability()

        if -1 == ioctl(cam, v4l2.VIDIOC_QUERYCAP, capability):
            print("can't get capabilities")
        else:
            if capability.capabilities & v4l2.V4L2_CAP_STREAMING:
                print('can read the camera')
            else:
                print('cannot read the camera')


        fmt = v4l2.v4l2_format()

        fmt.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE
        fmt.fmt.pix.width = 640
        fmt.fmt.pix.height = 480
        fmt.fmt.pix.pixelformat = v4l2.V4L2_PIX_FMT_YUYV
        fmt.fmt.pix.field = v4l2.V4L2_FIELD_INTERLACED

        buffer_size = 0
        
        if -1 == ioctl(cam, v4l2.VIDIOC_S_FMT, fmt):
            print('format fail')
        else:
            print('bytesperline', fmt.fmt.pix.bytesperline)
            print('sizeimage', fmt.fmt.pix.sizeimage)
            buffer_size = fmt.fmt.pix.sizeimage


        req = v4l2.v4l2_requestbuffers()

        req.count = 4
        req.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE
        req.memory = v4l2.V4L2_MEMORY_USERPTR

        if -1 == ioctl(cam, v4l2.VIDIOC_REQBUFS, req):
            print('cannot get reqbufs')
        else:
            print('req count', req.count)

        buflocs = []
        buffers = []

        for i in range(req.count):
            bl = buffer_location()
            buflocs.append(bl)
            bl.length = buffer_size
            buff = create_string_buffer(buffer_size)
            buffers.append(buff)
            bl.start = pointer(buff)
            print(dir(buff))

        for i in range(req.count):
            v4l2buf = v4l2.v4l2_buffer()
            v4l2buf.type = v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE
            v4l2buf.memory = v4l2.V4L2_MEMORY_USERPTR
            v4l2buf.index = i
            v4l2buf.m.usrptr = buflocs[i].start
            v4l2buf.length = buffer_size


            if -1 == ioctl(cam, v4l2.VIDIOC_QBUF, v4l2buf):
                print('cannot qbuf')
            
            
        type = v4l2.c_int(v4l2.V4L2_BUF_TYPE_VIDEO_CAPTURE)
        
        if -1 == ioctl(cam, v4l2.VIDIOC_STREAMON, type):
            print('cannot STREAM ON')

        print('stream on')

        # for frame_n in range(70):
        #     while(True):
        #         fds = [cam]
        #         r,_,_ = select.select(fds,[],[],2.0)
        #         if 0 < len(r):
        #             if read_frame():
        #                 break
        #         else:
        #             print('timeout')    
                    
                
