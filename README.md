![The first fully automated lunar bird tracker](https://i.imgur.com/jRGf8h6h.jpg "The first fully automated lunar bird tracker")

# The LunAero Project
LunAero is a hardware and software project using OpenCV to automatically
track birds as they migrate in front of the moon.  This repository contains
software used to track bird silhouettes from video produced by the LunAero
moontracking robot.  For information on the hardware and software for the
robot, please see https://github.com/BlueNalgene/LunAero_C .

Created by @BlueNalgene, working in the lab of @Eli-S-Bridge.

# Birdtracker_CPP vs. LunAero

The Birdtracker_CPP repository is an updated version of the LunAero silhouette
tracking software.  The previous implementation
(archived at https://github.com/BlueNalgene/LunAero) was a Python 3
script.  This updated version is C++ code designed for Linux, and includes
many improvements, additional features, and is generally more mature code.

## Software Install Instructions

### Option 1: Compile this Repository

To follow my instructions, you need to use the Linux terminal, where you
can copy and paste the following scripts.  To open the Linux terminal,
press <kbd>ctrl</kbd>+<kbd>alt</kbd>+<kbd>t</kbd>.

When you install this for the first time on to a Linux system, you will
need certain repositories installed to make it.  If you are using a
Debian-like operating system (e.g. Ubuntu or Mate), use the following
command in your terminal. (Note: this looks way more daunting than it
really is, your OS will have most of these installed with a default
configuration):

```sh
sudo apt update
sudo apt -y install git libgcc-7-dev libc6 libstdc++-7-dev libc6-dev \
libgl1 libqt5test5 libqt5opengl5 libqt5widgets5 libqt5gui5 libqt5core5a \
libdc1394-22 libavcodec-extra57 libavformat57 libavutil55 libswscale4 \
libpng16-16 libtiff5 libopenexr22 libtbb2 zlib1g libglx0 libglvnd0 \
libharfbuzz0b libicu60 libdouble-conversion1 libglib2.0-0 libraw1394-11 \
libusb-1.0-0 libswresample2 libwebp6 libcrystalhd3 libva2 libzvbi0 \
libxvidcore4 libx265-146 libx264-152 libwebpmux3 libwavpack1 libvpx5 \
libvorbisenc2 libvorbis0a libvo-amrwbenc0 libtwolame0 libtheora0 \
libspeex1 libsnappy1v5 libshine3 librsvg2-2 libcairo2 libopus0 \
libopenjp2-7 libopencore-amrwb0 libopencore-amrnb0 libmp3lame0 libgsm1 \
liblzma5 libssh-gcrypt-4 libopenmpt0 libbluray2 libgnutls30 libxml2 \
libgme0 libchromaprint1 libbz2-1.0 libx11-6 libdrm2 libvdpau1 \
libva-x11-2 libva-drm2 libjbig0 libjpeg-turbo8 libilmbase12 libfreetype6 \
libgraphite2-3 libpcre3 libudev1 libsoxr0 libnuma1 libogg0 \
libgdk-pixbuf2.0-0 libpangocairo-1.0-0 libpangoft2-1.0-0 libpango-1.0-0 \
libfontconfig1 libcroco3 libffi6 libpixman-1-0 libxcb-shm0 libxcb1 \
libxcb-render0 libxrender1 libxext6 libgcrypt20 libgssapi-krb5-2 \
libmpg123-0 libvorbisfile3 libp11-kit0 libidn2-0 libunistring2 \
libtasn1-6 libnettle6 libhogweed4 libgmp10 libxfixes3 libgomp1 \
libselinux1 libmount1 libthai0 libexpat1 libxau6 libxdmcp6 libgpg-error0 \
libkrb5-3 libk5crypto3 libcom-err2 libkrb5support0 libblkid1 libdatrie1 \
libbsd0 libkeyutils1 libuuid1
```

Then comes the step that is actually daunting.  This program requires
OpenCV compiled with `contrib`.  This means you MUST compile OpenCV from
source.  This has been tested for OpenCV version `4.4.0-pre`.  Older
versions may not work.  There are many options for compiling, I used
the following on my Ubuntu box:

+ Install prerequisites
```sh
sudo apt install build-essential cmake git pkg-config libgtk-3-dev \
libavcodec-dev libavformat-dev libswscale-dev libv4l-dev \
libxvidcore-dev libx264-dev libjpeg-dev libpng-dev libtiff-dev \
gfortran openexr libatlas-base-dev python3-dev python3-numpy \
libtbb2 libtbb-dev libdc1394-22-dev
```
+ Download files
```sh
mkdir ~/opencv_build && cd ~/opencv_build
git clone https://github.com/opencv/opencv.git
git clone https://github.com/opencv/opencv_contrib.git
```
+ Prep make options
```sh
cd ~/opencv_build/opencv
mkdir build && cd build
cmake -D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D INSTALL_C_EXAMPLES=ON \
    -D INSTALL_PYTHON_EXAMPLES=ON \
    -D OPENCV_GENERATE_PKGCONFIG=ON \
    -D OPENCV_EXTRA_MODULES_PATH=~/opencv_build/opencv_contrib/modules \
    -D BUILD_EXAMPLES=ON ..
```
+ Make the files using multiple threads and install.  Get coffee during
this step.
```sh
make -j8
sudo make install
```

Next, use `git` to pull this repository.  Once the LunAero `git`
repository is pulled, you can compile it.  To do this, execute
the lines in the terminal:

```sh
cd /home/$USER/Documents
git https://github.com/BlueNalgene/CPP_Birdtracker.git
cd Birdtracker_CPP
```

This script downloads the everything you need to compile the program from
this `git` repository.  Once you have the repository on your device
and have installed all of the relevant packages from the `apt` command
above, enter the LunAero_C folder, and issue the following command:

```sh
make
```

The `make` command follows the instructions from the `Makefile` to
compile your program.  If you ever make changes to the source material,
remember to edit the `Makefile` to reflect changes to the packages used
or the required C++ files.  If `make` runs correctly, your terminal will
print some text that looks like `g++ blah blah -o frame_extractor.o`
spanning a few lines.  If the output is significantly longer than that
and includes words like ERROR or WARNING, something may have gone wrong,
and you should read the error messages to see if something needs to be
fixed.

### Option 2 (experimental): Download a Pre-Compiled Binary Release







## Running LunAero

To run the `frame_extractor.o`, use the Linux terminal.  There, you enter
the command for the program and information about the video you want to
run the program on.  It is convenient if you navigate to the directory
where you made the file:

```sh
/path/to/Birdtracker_CPP
```

### Edit the Settings

A file called `settings.cfg` has been provided to give the user the
ability to modify settings without needing to recompile.  Before
running, LunAero for the first time, it is prudent to check that you are
happy with the default settings.  This is especially true for the
General Settings at to top of the file.

A python3 script called `fog_removal.py` has been added to simplify
selecting the appropriate value for BLACKOUT_THRESH.  Run this script
prior to running a video to visually determine the appropriate value
for this variable.

### Command Options

The following options are available for command line switches:

| Full Command  | Short Command | Expected Input |           Description          |
|---------------|---------------|----------------|--------------------------------|
|   `--help`    |     `-h`      |     none       | Show this info                 |
|  `--version`  |     `-v`      |     none       | Print version info to terminal |
|   `--input`   |     `-i`      |path to vid file| Specify path to input video    |
|`--config-file`|     `-c`      | path to config | Specify config file            |
|` --osf-path`  |    `-osf`     | url to osf vid | Specify path to osf video      |

The "Short Command" is just a helpful shorter version to replace the full command.
Using either the `--help` or `--version` commands will print the relevant info to
the terminal and exit, without running the full program.  Each command or input
must be separated by a space.

Each of the commands `--input`, `--config-file`, and `--osf-path` require input
arguments to follow the switch.  These inputs are paths to the local storage
location (in the case of `--input` and `--config-file`) or a url path (in the
case of `--osf-path`).  If any of these switches are issued without an appropriate
path argument following it, the program will crash.  You may not issue both the
`--input` and `--osf-path` in the same terminal command, as these switches conflict.
(You can only have one source video!).  You do not need to issue a `--config-file`
switch if you are using the `settings.cfg` file stored in the default location.

Some example commands:

```sh
./frame_extractor.o -i /path/to/video.mp4

```
```sh
./frame_extractor.o -i "/path/to/video with spaces.mp4"
```

```sh
./frame_extractor.o -c /path/to/special_config.cfg -osf osfstorage/path/to/video.mp4
```

Remember: any time you want to check the usage information to view what each
command switch does, use `--help` in terminal, e.g.:

```sh
./frame_extractor.o --help
```

### Using videos from OSF

Run Birdtracker_CPP on videos directly from storage repositories on
Open Science Framework (OSF), you need to do some extra steps to pull
in the API and enable access.

+ The Python package `OSFClient` must be installed on your system.  To install it,
use the `pip` package manager e.g.:
```sh
pip3 install OSFClient --user
```
+ You must configure OSFClient on your system if you are using a password
protected repository.  To do this, add an environmental variable to your terminal
for the entry "OSF_PASSWORD".  On my system, this is easiest by editing the `.bashrc`
file to contain the export command.  
```sh
nano /home/$USER/.bashrc
```
scroll to the bottom and add a line
```sh
export OSF_PASSWORD=[your password goes here]
```
This is the safest way to access according to the OSFClient maintainers, and I
did not write this part of the code.  Your milage may vary.  If you are using a
shared computer, be sure to remove your password when you are done.
+ You need to know the path to your file including the OSF repo ID and
the storage name.  The repository ID is a 5-digit alphanumeric code you can see
when you got to the browser version of the site (osf.io/12345).  This value must
be written to the string `OSFPROJECT` in settings.cfg.
+ If you are using the defult OSF storage servers, you need to include the text
"osfstorage" at the beginning of your path in the terminal.  If you are using
another storage server, please let me know what you have to use!

### Valid Input File Formats

The best choice for file format is MP4, as that is what everything
is configured in the code to use.  Other formats, such as the raw
H264 video format which is output by LunAero, are converted to MP4
prior to starting the run.  This video conversion may add time to
your run, so consider this if you run this on shared systems with
restricted time access.

Video conversion is done using `ffmpeg`, and this program is thus
a prerequisite for use.  If you are on a Linux system, you probably
have `ffmpeg` installed already though.  So don't sweat it.

## Behind the Curtain: What CPP_Birdtracker does

This program executes birdtracking efficiently by splitting tasks
between CPU threads using the `fork()` command.  Data which are shared
between the forks such as frame numbers and matrices, is shared using
memory mapping such that they can be accessed easily using pointers.
The `wait` command is used to sync across forks, and the script executes
code concurrently.  When all frames of the video are completed, the data
are passed through post-processing and the program exits.  A simplified
flowchart explains the process visually:

```
                     /-------\
                     | start |
                     \-------/
                         |
                    +---------+
                    | startup |
                    +---------+
                         |
                  +-------------+
                  | pre-process |
                  +-------------+
                         |
                    +--------+
                    | fork() |
                    +--------+
                         |
            +------------+
            |            |
        +-------+    +------+
        | fetch |  ->| sync | <------------------------\
    +-->| next  |  | +------+                           \
    |   | frame |  |     |                               \
    |   +-------+  | +--------+                           \
    |       |      | | fork() |                            \
    |   +-------+  | +--------+                             \
    |   | cycle |  |         |                               \
    |   | frame |  |         +------------------------+       \
    |   | buffer|  |         |                        |        \
    |   +-------+  |         |                   +--------+    |
    |       |      |    +--------+               | cycle  |    |
    |   +-------+ /     | Tier 1 |               | frame  |    |
    |   |  wait |/      +--------+               | buf 2  |    |
    |   +-------+            |                   +--------+    |
    |       |           +--------+                    |        |
    |       ^           | Tier 2 |               +--------+    |
 no |      / \          +--------+               | Tier 3 |    |
    |     /   \              |                   +--------+    |
    |    /     \         +-------+                    |        |while
    |___/ video \        | wait/ |               +--------+    |loop
        \ done? /        |  sync |               | Tier 4 |    |
         \     /         +-------+               +--------+    |
          \   /              |                        |        |
           \ /               \                    +-------+    |
            v                 \                   | wait/ |    |
        yes |                  \                  |  sync |    |
            |                   \                 +-------+    |
    +--------------+             \                 |           /
    | post-process |              \                /          /
    +--------------+               \              /          /
            |                       \            /          /
            |                        \          /          /
            |                         \        /          /
            |                          \      /          /
            |                          +------+         /
            |                          | kill |--------/
            |                          | fork |
            |                          +------+
         /-----\
         | end |
         \-----/
```

Individual steps with more detail, but still simplified:

- startup:
  + Parses the input commands
  + Redefines globals based on values from settings.cfg
  + Creates directories/empty data files
  + Instance and assign initial values to simple memory mapped regions
- pre-process
  + Load the video
  + Find the first frame where the moon is not touching the edge of the screen
  + Record initial values
  + Instance and assign initial values to complext memory mapped regions (frame buffers)
  + Prepare for fork()
- FORK TWICE (three concurrent threads)
  - fork0
    + Check for exit command and sync status
    + Coordinate sync values across forks
    + Fetch the next frame
    + Prep frame (color conversion and cropping)
    + Store frame to the first buffer
    + Wait for sync.
  - fork1
    + Check for exit command and sync status
    + Run Tier 1 and Tier 2 calculations
    + Wait for sync
  - fork2
    + Check for exit command and sync status
    + Cycle first frame buffer with second frame buffer such that we have frame n and n-1
    stored in the memory.
    + Run Tier 3 and Tier 4 calculations (requires n and n-1 frames)
    + Wait for sync
- Kill unused fork and continue loop
- If at end of the frames, kill all forks.
- post-process
  + Create slideshow
  + Concat data
  + Crop slideshow
- Cleanup and Exit

### Image Processing

While there are a few image processing functions peppered through the code,
most of these are simple color conversions and cropping.  The meat of the
image processing occurs within the "Tiers".  This is where contours of
silhouettes crossing the moon are detected.  They all operate on the same
frames, but some "Tiers" offer different capabilities.  Each have strengths
and weaknesses.

- Tier 1: Strict adaptive threshold. In perfect conditions, this finds
the majority of the large silhouettes.  As the contours become less
clear or obvious, the image processing done on this Tier will begin to
miss some silhouettes.
- Tier 2: Loose adaptive threshold.  In perfect conditions, this finds
most of the smaller silhouettes which may be missed by Tier 1.  However,
it produces much more noise.  Similar to Tier 1, as the silhouttes become
harder to see, they are less likely to be picked up here.
- Tier 3: Isotropic Laplacian filter.  This is a very noisy filter which
compares differences in the Laplacian across two frames.  The result is
blurred prior to detection, but much of the noise remains.
- Tier 4: UnCanny filter.  This experimental filter attempts to perform
the steps used in the classic Canny filter backwards...across two frames.
The Sobel and power steps of the Canny filter occur on each frame (n and
n-1), then are added, rooted, and blurred.  The result is passed through
an edge thinner and the silhouettes are detected.

See the descriptions and code in the documentation
[here](https://bluenalgene.github.io/CPP_Birdtracker/html/frame__extraction_8cpp.html).

Each Tier process has values which may be manipulated in `settings.cfg`
to customize the behavior during runtime.


### Estimating Run Time

On the test computer (Intel(R) Core(TM) i7-7500U CPU @ 2.70GHz), the
run time of this program is approximately 7 times the length of the
input video, when the video is 1920x1080 at 30fps.  So a 5 minute
video will run in about 35 minutes.

Why does it take so long?  Most of the computation time is spent
opening and closing image files.  This cannot be avoided.

## The Output

This program outputs several files during the run.  The files are stored
by default in the folder `Birdtracker_Output`.  Data files are stored
in the `data` directory, if the settings are configured to output the
frames, a `frames` directory is created containing `png` images of each
frame in the video, this will also create an `output.mp4` slideshow of
the frames.

### The Data

The data files in the `data` directory are as follows:

|      Filename      |                         Description                         |
|--------------------|-------------------------------------------------------------|
| log.log            | Debugging log of run                                        |
| metadata.csv       | Input video metadata                                        |
| boxes.csv          | Boundary values of largest contour in frame                 |
| ellipses.csv       | Properties of the largest contour in frame                  |
| Tier1.csv          | Tier 1 detected silhouettes                                 |
| Tier2.csv          | Tier 2 detected silhouettes                                 |
| Tier3.csv          | Tier 3 detected silhouettes                                 |
| Tier4.csv          | Tier 4 detected silhouettes                                 |
| mixed_tiers.csv    | All tier data mixed into a single file                      |
| offscreen_moon.csv | Number of pixels where the moon is touching the screen edge |

- log.log - If the boolean toggle `DEBUG_COUT` is set to `true` in
settings.cfg, this log file will be created.  Debugging messages are
written to this file over the course of the run.  Note, that this does
not include any STDOUT or STDERR messages.  If there is an error
message written to the terminal, for example, you must fetch that another
way.
- metadata.csv - This file lists properties of the input video.
Currently, that just includes the path to the input video.
- boxes.csv - This comma separated values (csv) file is mostly used for
internal calculations.  It is retained for diagnostic conveneince. The
contents are values calculated which describe the location and size of
the boundary points of the largest contour on each frame (presumably,
this is the moon).
- ellipses.csv - This csv file lists properties of the largest contour
on the screen more relevant to lunar calculations.  Includes information
such as the major and minor axis of the contour, area, and edges
which touch the side of the screen.
- Tier1.csv - This csv lists all of the silhouettes detected using the
stricter version of the `adaptiveThreshold` method.  See 
[tier_one()](https://bluenalgene.github.io/CPP_Birdtracker/html/frame__extraction_8cpp.html#a4f9b7b156d48ad3f147ff2b4edc01509)
- Tier2.csv - This csv lists all of the silhouettes detected using the
more lenient version of the `adaptiveThreshold` method.
- Tier3.csv - This csv lists all of the silhouettes detected using
isotropic Laplacian matching between frames.
- Tier4.csv - This csv lists all of the silhouettes detected using
a new filter we are calling "UnCanny", which is a reverse operation
of the Canny filter between two frames.
- mixed_tiers.csv - This csv is a convenience file which inclues
everything from Tier*.csv in frame order.  An additional column
indicates which Tier method the line was generated from.  Only created
if `CONCAT_TIERS` is set to `true`.
- offscreen_moon.csv - This csv is a convenience file which edits down
ellipses.csv to only report frames with non-zero values in one or more
of the screen edge columns.  Only created if `SIMP_ELL` is set to
`true`.

### The Video

If the `OUTPUT_FRAMES` value in settings is set to `true`, the Birdtracker
will output every cropped frame to the `frames` directory as a `png` file
with the naming format based on the frame number.  These frames will remain
in the directory unless the setting `EMPTY_FRAMES` is set to `true`.

If the program is outputting frames, and `GEN_SLIDESHOW` is set to `true`,
the frames will be contatenated into a slideshow saved as `output.mp4`.

If the program generates a slideshow, and `TIGHT_CROP` is set to `true`,
the output video will be modified at the end of the run to crop each frame
to a smaller size.  The size of these frames is based on the extreme
edges of the moon as reported in `boxes.csv` + 10 pixels.  The cropped
video is saved as `output.mp4`.

## When Something Goes Wrong

Something always goes wrong.  It is the way of things.  When something
inevitably does go wrong with LunAero program, you should first check
the logs.  If the setting `DEBUG_COUT` in `settings.cfg` is set to `true`
the program will attempt to save a log file in the same directory where
the videos are saved for each run.  This is very detailed, so searching
for keywords like `WARNING` and `ERROR` are suggested.

### Something Went Wrong...and it isn't listed here

If you need help, raise an Issue on this `git` repository with
descriptive information so the package maintainer can help you.

## What if I Want to Play with the Source Code?

The source code is documented with the `Doxygen` standard.  Every
function and most variables are heavily commented to make it easy for
you.  You can view the documentation online by going to:
https://bluenalgene.github.io/CPP_Birdtracker/html/index.html .

