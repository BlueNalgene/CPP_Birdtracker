
/*
 * CPP_Birdtracker/frame_extraction.hpp - Primary header LunAero birdtracker.
 * Copyright (C) <2020>  <Wesley T. Honeycutt>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FRAME_EXTRACTION_H
#define FRAME_EXTRACTION_H

// Includes
#include <algorithm>
#include <ctime>                         // for NULL
#include <errno.h>
#include <tgmath.h>                      // for sin, cos, etc.
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <numeric>                       // for accumulate
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>                      // for exit
#include <sys/ipc.h>                     // for IPC_CREAT, key_t
#include <sys/shm.h>                     // for shmat
#include <sys/mman.h>                    // for MAP_ANONYMOUS, MAP_SHARED
#include <sys/wait.h>
#include <tuple>
#include <unistd.h>                      // for fork, sleep, usleep
#include <typeinfo>
#include <regex>

#include <string>                        // for string
using std::string;

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include <vector>                        // for vector
using std::vector;

#include "opencv2/opencv.hpp"
#include "opencv2/ximgproc.hpp"
using namespace cv;

// std::map<std::string, std::string> options;

// Global Variables
/**
 * Major version number
 */
int MAJOR_VERSION = 0;
/**
 * Minor version number
 */
int MINOR_VERSION = 3;
/**
 * Add-on version number for alpha/beta/release desgination
 */
std::string ALPHA_BETA = "-alpha";
/**
 * Magic number of the ctrl+c interrupt signal in Linux
 */
int SIG_ALERT = 0;
/**
 * Area of the largest contour in the first "good" frame, determined by first_frame()
 */
double ORIG_AREA;
/**
 * Perimeter of the largest contour in the first "good" frame, determined by first_frame()
 */
double ORIG_PERI;
/**
 * Height of the largest contour in the first "good" frame, determined by first_frame()
 */
int ORIG_VERT;
/**
 * Width of the largest contour in the first "good" frame, determined by first_frame()
 */
int ORIG_HORZ;
/**
 * Minimum dimension of the raw input frame from the video.  Set by min_square_dim()
 * e.g. for a 1920x1080 video this will be 1080.
 */
int BOXSIZE;
/**
 * xy coorindates of the top left corner of the moon in the first "good" frame, determined by
 * first_frame()
 */
Point ORIG_TL;
/**
 * xy coorindates of the bottom right corner of the moon in the first "good" frame, determined by
 * first_frame()
 */
Point ORIG_BR;
/**
 * Holder for the location of the log file created by DEBUG_COUT functions.
 */
std::string LOGOUT;
/**
 * Holder for the output stream created by DEBUG_COUT functions.
 */
std::ofstream LOGGING;
/**
 * Holder for the location of the Tier 1 data output CSV
 */
std::string TIER1FILE;
/**
 * Holder for the location of the Tier 2 data output CSV
 */
std::string TIER2FILE;
/**
 * Holder for the location of the Tier 3 data output CSV
 */
std::string TIER3FILE;
/**
 * Holder for the location of the Tier 4 data output CSV
 */
std::string TIER4FILE;
/**
 * Holder for the location of the ellipse data output CSV
 */
std::string ELLIPSEDATA;
/**
 * Holder for the location of the box data output CSV
 */
std::string BOXDATA;
/**
 * Holder for the location of the metadata file
 */
std::string METADATA;
/**
 * Calcualted width to use for tight cropping
 */
int TC_W;
/**
 * Calculated height to use for tight cropping
 */
int TC_H;




// Global Variables declared in settings.cfg
/**
 * Run frame_extraction with debugging output (prints everything verbose)
 * User configurable from settings.cfg
 */
bool DEBUG_FRAMES = false;
/**
 * Run frame_extraction with OpenCV window for debugging purposes
 * User configurable from settings.cfg
 */
bool DEBUG_COUT = false;
/**
 * Should the frame extractor output every frame with detected contours?
 * User configurable from settings.cfg
 */
bool OUTPUT_FRAMES = false;
/**
 * Should the program cleanup the generated frame png images on exit?
 * This is ignored if OUTPUT_FRAMES = false
 * User configurable from settings.cfg
 */
bool EMPTY_FRAMES = true;
/**
 * Should the program concatenate the exported frames into a video at the end?
 * This is ignored if OUTPUT_FRAMES = false
 * User configurable from settings.cfg
 */
bool GEN_SLIDESHOW = true;
/**
 * Should the program attempt to created a simplified file of frames where the edge is off screen?
 * User configurable from settings.cfg
 */
bool SIMP_ELL = true;
/**
 * Should the program concatenate the Tiered data into a single file?
 * User configurable from settings.cfg
 */
bool CONCAT_TIERS = true;
/**
 * Tight crop the output frames when generating the slideshow?
 * This is ignored if OUTPUT_FRAMES = false
 * User configurable from settings.cfg
 */
bool TIGHT_CROP = true;
/**
 * The project code for the project on OSF (optional)
 * User configurable from settings.cfg
 */
std::string OSFPROJECT = "";
/**
 * The directory where the output data should be put.  Default creates a new directory.
 * No quotation marks.  Requires a leading slash and trailing slash (e.g. /test/)
 * User configurable from settings.cfg
 */
std::string OUTPUTDIR = "Birdtracker_Output/";
/**
 * Number of Pixels touching the edge of the image to count as "off screen"
 * User configurable from settings.cfg
 */
int EDGETHRESH = 10;
/**
 * Quiet Halo Elimination mask width.  How many pixels around the moon halo can be ignored?
 * User configurable from settings.cfg
 */
int QHE_WIDTH = 10;
/**
 * Threshold value for a TOZERO OpenCV thresholding operation.  This is used to mask any haze around
 * the moon, making sure the 'night sky' remains black, and prevents a hazy moon from doing strange
 * things to the perceived size of the moon by the dumb algorithm.  When finding the the size of the moon
 * halo, all pixels below this value (of max 255) become 0.  If you set it too high for your video, you
 * may lose some moon size in the process, but this will not break your bird tracking.
 * User configurable from settings.cfg
 */
int BLACKOUT_THRESH = 51;


/**
 * Gaussian Blur X kernel size for QHE Bigone.
 * User configurable from settings.cfg
 */
int QHE_GB_KERNEL_X = 15;
/**
 * Gaussian Blur Y kernel size for QHE Bigone.
 * User configurable from settings.cfg
 */
int QHE_GB_KERNEL_Y = 15;
/**
 * Gaussian Blur sigma x value for QHE Bigone.
 * User configurable from settings.cfg
 */
double QHE_GB_SIGMA_X = 1;
/**
 * Gaussian Blur sigma y value for QHE Bigone.
 * User configurable from settings.cfg
 */
double QHE_GB_SIGMA_Y = 1;



/**
 * Max value for adaptive threshold for Tier 1.
 * User configurable from settings.cfg
 */
double T1_AT_MAX = 255;
/**
 * Blocksize for adaptive threshold for Tier 1.
 * User configurable from settings.cfg
 */
int T1_AT_BLOCKSIZE = 65;
/**
 * Subtracted constant for adaptive threshold for Tier 1.
 * User configurable from settings.cfg
 */
double T1_AT_CONSTANT = 35;
/**
 * Width of the dynamic mask around the edge of the moon for Tier 1.
 * User configurable from settings.cfg
 */
int T1_DYMASK = 25;


/**
 * Max value for adaptive threshold for Tier 2.
 * User configurable from settings.cfg
 */
double T2_AT_MAX = 255;
/**
 * Blocksize for adaptive threshold for Tier 2.
 * User configurable from settings.cfg
 */
int T2_AT_BLOCKSIZE = 65;
/**
 * Subtracted constant for adaptive threshold for Tier 2.
 * User configurable from settings.cfg
 */
double T2_AT_CONSTANT = 20;
/**
 * Width of the dynamic mask around the edge of the moon for Tier 2.
 * User configurable from settings.cfg
 */
int T2_DYMASK = 25;


/**
 * Kernel size of laplacian transform for Tier 3.
 * User configurable from settings.cfg
 */
int T3_LAP_KERNEL = 11;
/**
 * Scaling value of laplacian transform for Tier 3.
 * User configurable from settings.cfg
 */
double T3_LAP_SCALE = 0.0001;
/**
 * Delta value of laplacian transform for Tier 3.
 * User configurable from settings.cfg
 */
double T3_LAP_DELTA = 0;
/**
 * Gaussian Blur X kernel size for Tier 3.
 * User configurable from settings.cfg
 */
int T3_GB_KERNEL_X = 11;
/**
 * Gaussian Blur Y kernel size for Tier 3.
 * User configurable from settings.cfg
 */
int T3_GB_KERNEL_Y = 11;
/**
 * Gaussian Blur sigma x value for Tier 3.
 * User configurable from settings.cfg
 */
double T3_GB_SIGMA_X = 1;
/**
 * Gaussian Blur sigma y value for Tier 3.
 * User configurable from settings.cfg
 */
double T3_GB_SIGMA_Y = 1;
/**
 * Thresholding value unique to Tier 3.  Removes items less than the threshold
 * User configurable from settings.cfg
 */
int T3_CUTOFF_THRESH = 40;
/**
 * Width of the dynamic mask application for Tier 3.
 * User configurable from settings.cfg
 */
int T3_DYMASK = 45;


/**
 * Max value for adaptive threshold for Tier 4.
 * User configurable from settings.cfg
 */
double T4_AT_MAX = 255;
/**
 * Blocksize for adaptive threshold for Tier 4.
 * User configurable from settings.cfg
 */
int T4_AT_BLOCKSIZE = 65;
/**
 * Subtracted constant for adaptive threshold for Tier 4.
 * User configurable from settings.cfg
 */
double T4_AT_CONSTANT = 20;
/**
 * "UnCanny" script raises values to this power for Tier 4.
 * User configurable from settings.cfg
 */
double T4_POWER = 2;
/**
 * Gaussian Blur X kernel size for Tier 4.
 * User configurable from settings.cfg
 */
int T4_GB_KERNEL_X = 11;
/**
 * Gaussian Blur Y kernel size for Tier 4.
 * User configurable from settings.cfg
 */
int T4_GB_KERNEL_Y = 11;
/**
 * Gaussian Blur sigma x value for Tier 4.
 * User configurable from settings.cfg
 */
double T4_GB_SIGMA_X = 1;
/**
 * Gaussian Blur sigma y value for Tier 4.
 * User configurable from settings.cfg
 */
double T4_GB_SIGMA_Y = 1;
/**
 * Contour Thinning type (see ximgproc::thinning) for Tier 4.
 * User configurable from settings.cfg
 */
int T4_THINNING = 0;
/**
 * Width of the dynamic mask application for Tier 4.
 * User configurable from settings.cfg
 */
int T4_DYMASK = 45;


// Declared functions/prototypes
static Mat shift_frame(Mat in_frame, int shiftx, int shifty);
static Mat corner_matching(Mat in_frame, vector<Point> contour, int plusx, int plusy);
static vector <int> test_edges(Mat in_frame, vector<Point> contour, int te_ret);
static int min_square_dim(Mat in_frame);
static vector <int> edge_width(vector<Point> contour);
static vector <int> edge_height(vector<Point> contour);
static Mat initial_crop(Mat in_frame, int framecnt);
static int touching_edges(Mat in_frame, vector<Point> contour);
static Mat traditional_centering(Mat in_frame, vector <vector<Point>> contours, int largest, Rect box);
static int first_frame(Mat in_frame, int framecnt);
static Mat halo_noise_and_center(Mat in_frame, int framecnt);
static void signal_callback_handler(int signum);
static Mat apply_dynamic_mask(Mat in_frame, vector<vector<Point>> contours, int maskwidth);
static int largest_contour(vector <vector<Point>> contours);
static vector <vector<Point>> contours_only(Mat in_frame);
static Rect box_finder(Mat in_frame, bool do_thresh);
static int box_data(Rect box, int framecnt);
static int show_usage(string name);
static vector <Point> qhe_bigone(Mat in_frame);
static vector <vector<Point>> quiet_halo_elim(vector <vector<Point>> contours, vector <Point> bigone);
static int tier_one(int cnt, Mat in_frame, vector <Point> bigone);
static int tier_two(int cnt, Mat in_frame, vector <Point> bigone);
static int tier_three(int cnt, Mat in_frame, Mat old_frame, vector <Point> bigone);
static int tier_four(int cnt, Mat in_frame, Mat old_frame, vector <Point> bigone);
static int parse_checklist(std::string name, std::string value);
static std::string out_frame_gen(int framecnt);
std::string space_space(std::string instring);
static int generate_slideshow();
static int concat_tiers();
static int off_screen_ellipse();
static int post_processing();
int main(int argc, char* argv[]); 

#endif
