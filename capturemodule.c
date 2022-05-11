#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <getopt.h>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>


#define VIDEODEV "/dev/video0"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct buffer
{
  void* start;
  size_t length;
};

static int fd = -1;
struct buffer* buffers;
static unsigned int n_buffers = 0;

static void errno_exit(const char* s)
{
  fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
  exit(EXIT_FAILURE);
}

static int xioctl(int filehandle, int request, void* arg)
{
  int r;

  do
  {
    r = ioctl(filehandle, request, arg);
  } while(-1 == r && EINTR == errno);

  return r;
}

static int read_frame(char* output)
{
  struct v4l2_buffer buf;
  unsigned int i;

  CLEAR(buf);

  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  
  if(-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
  {
    switch(errno)
    {
    case EAGAIN:
      return 0;
    default:
      errno_exit("VIDIOC_DQBUF");
    }
  }
  
  assert(i < n_buffers);

  strncpy(output, buffers[0].start, buf.bytesused);

  if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
  {
    errno_exit("VIDIOC_QBUF");
  }
  
  return 1;
}

static void fill_buffer(char* output, int frame_count, size_t frame_size)
{
  const int frame_max = frame_count;
  while(frame_count > 0)
  {
    while(1)
    {
      fd_set fds;
      struct timeval tv;
      int r;

      FD_ZERO(&fds);
      FD_SET(fd, &fds);
    
      tv.tv_sec = 2;
      tv.tv_usec = 0;

      r = select(fd+1, &fds, NULL, NULL, &tv);

      if(-1 == r)
      {
	if(EINTR == errno)
        {
	  continue;
        }
	errno_exit("select");
      }

      if(0 == r)
      {
	fprintf(stderr, "select timeout\n");
	exit(EXIT_FAILURE);
      }

      if(read_frame(output))
      {
	output += frame_size;
	break;
      }
    }
    frame_count--;
  }
}

static void open_device(void)
{
  struct stat st;

  if(-1 == stat(VIDEODEV, &st))
  {
    fprintf(stderr, "Cannot identify '%s': %d, %s\n", VIDEODEV, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
  if(!S_ISCHR(st.st_mode))
  {
    fprintf(stderr, "'%s'is not a device\n %d, %s\n", VIDEODEV, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  fd = open(VIDEODEV, O_RDWR | O_NONBLOCK, 0);

  if(-1 == fd)
  {
    fprintf(stderr, "Cannot identify '%s': %d, %s\n", VIDEODEV, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

static void close_device(void)
{
  if(-1 == close(fd))
  {
    errno_exit("close");
  }

  fd = -1;
}

static void stop_capturing(void)
{
  enum v4l2_buf_type type;
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
  {
    errno_exit("VIDIOC_STREAMOFF");
  }  
}

static void start_capturing(void)
{
  unsigned int i;
  enum v4l2_buf_type type;

  for(i = 0; i < n_buffers; i++)
  {
    struct v4l2_buffer buf;

    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;

    if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
    {
      printf("first\n");
      errno_exit("VIDIOC_QBUF");
    }
  }
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(-1 == xioctl(fd, VIDIOC_STREAMON, &type))
  {
    errno_exit("VIDIOC_STREAMON");
  }
}

static void uninit_device(void)
{
  unsigned int i;

  for(i = 0; i < n_buffers; i++)
  {
    if(-1 == munmap(buffers[i].start, buffers[i].length))
    {
      errno_exit("munmap");
    }
  }

  free(buffers);
}

static void init_mmap(size_t buffer_size)
{
  struct v4l2_requestbuffers req;
  CLEAR(req);
  
  req.count = 4;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if(-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
  {
    if(EINVAL == errno)
    {
      fprintf(stderr, "%s does not support mmap \n", VIDEODEV);
      exit(EXIT_FAILURE);
    }
    else
    {
      errno_exit("VIDIOC_REQBUFS");
    }
  }

  if(req.count < 2)
  {
    fprintf(stderr, "insufficient buffer memory on %s\n", VIDEODEV);
    exit(EXIT_FAILURE);
  }

  buffers = calloc(4, sizeof(*buffers));

  if(!buffers)
  {
    fprintf(stderr, "out of memory \n");
    exit(EXIT_FAILURE);
  }

  for(n_buffers = 0; n_buffers < 4; n_buffers++)
  {
    struct v4l2_buffer buf;

    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = n_buffers;

    if(-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
    {
      errno_exit("VIDIOC_QUERYBUF");
    }

    buffers[n_buffers].length = buf.length;
    buffers[n_buffers].start = mmap(NULL, buf.length,
				    PROT_READ|PROT_WRITE,
				    MAP_SHARED,
				    fd, buf.m.offset);
    if(MAP_FAILED == buffers[n_buffers].start)
    {
      errno_exit("mmap");
    }	
  }
  
}

static size_t init_device(int width, int height)
{
  struct v4l2_capability cap;
  struct v4l2_cropcap cropcap;
  struct v4l2_crop crop;
  struct v4l2_format fmt;

  if(-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap))
  {
    if(EINVAL == errno)
    {
      fprintf(stderr, "%s is no V4L2 device \n", VIDEODEV);
      exit(EXIT_FAILURE);
    }
    else
    {
      errno_exit("VIDIOC_QUERYCAP");
    }
  }

  if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
  {
    fprintf(stderr, "%s is no video capture device \n", VIDEODEV);
    exit(EXIT_FAILURE);
  }

  if(!(cap.capabilities & V4L2_CAP_STREAMING))
  {
    fprintf(stderr, "%s does nto support streaming \n", VIDEODEV);
    exit(EXIT_FAILURE);
  }
  
  CLEAR(cropcap);

  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if(0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap))
  {
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect;

    xioctl(fd, VIDIOC_S_CROP, &crop);
  }

  CLEAR(fmt);

  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  fmt.fmt.pix.width = width;
  fmt.fmt.pix.height = height;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

  if(-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
  {
    errno_exit("VIDIOC_S_FMT");
  }

  init_mmap(fmt.fmt.pix.sizeimage);

  return fmt.fmt.pix.sizeimage;
}

size_t get_frame_size(int width, int height)
{
  struct v4l2_format fmt;

  CLEAR(fmt);

  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  fmt.fmt.pix.width = width;
  fmt.fmt.pix.height = height;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

  open_device();

  if(-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
  {
    errno_exit("VIDIOC_S_FMT");
  }
  
  close_device();

  return fmt.fmt.pix.sizeimage;
}

void capture(int frame_count, char* buffer, int width, int height)
{
  size_t frame_size;
  open_device();
  frame_size = init_device(width, height);
  start_capturing();
  fill_buffer(buffer, frame_count, frame_size);
  stop_capturing();
  uninit_device();
  close_device();
}


