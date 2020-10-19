




// Includes
#include <algorithm>
#include <ctime>                         // for NULL
//#include <exception>
#include <tgmath.h>                      // for sin, cos, etc.
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <numeric>                       // for accumulate
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>                      // for exit
#include <string>                        // for string
using std::string;
#include <sys/ipc.h>                     // for IPC_CREAT, key_t
#include <sys/shm.h>                     // for shmat
#include <sys/mman.h>                    // for MAP_ANONYMOUS, MAP_SHARED
#include <sys/wait.h>
#include <tuple>
#include <unistd.h>                      // for fork, sleep, usleep
#include <vector>                        // for vector
#include <typeinfo>
using std::vector;

// #include <config4cpp/Configuration.h>
// using namespace config4cpp;

#include "opencv2/opencv.hpp"
#include "opencv2/ximgproc.hpp"
using namespace cv;

std::map<std::string, std::string> options;

// Global Variables
int STARTUP_CONTOURS;
RotatedRect STOREBOX;
int SIG_ALERT = 0;
float ELL_RAD;
double ORIG_AREA;
double ORIG_PERI;
int ORIG_VERT;
int ORIG_HORZ;
int BOXSIZE;
Point ORIG_TL;
Point ORIG_BR;

// Global Variables declared in settings.cfg
bool DEBUG_FRAMES = false;
bool DEBUG_COUT = false;
std::string OSFPROJECT = "";
int EDGETHRESH = 10;
double T1_AT_MAX = 255;
int T1_AT_BLOCKSIZE = 65;
double T1_AT_CONSTANT = 35;
double T2_AT_MAX = 255;
int T2_AT_BLOCKSIZE = 65;
double T2_AT_CONSTANT = 20;
int T3_LAP_KERNEL = 11;
double T3_LAP_SCALE = 0.0001;
double T3_LAP_DELTA = 0;
int T3_GB_KERNEL_X = 11;
int T3_GB_KERNEL_Y = 11;
double T3_GB_SIGMA_X = 1;
double T3_GB_SIGMA_Y = 1;
int T3_CUTOFF_THRESH = 40;
int T3_DYNMASK_WIDTH = 45;
double T4_AT_MAX = 255;
int T4_AT_BLOCKSIZE = 65;
double T4_AT_CONSTANT = 20;
double T4_POWER = 2;
int T4_GB_KERNEL_X = 11;
int T4_GB_KERNEL_Y = 11;
double T4_GB_SIGMA_X = 1;
double T4_GB_SIGMA_Y = 1;
int T4_THINNING = 0;
int T4_DYNMASK_WIDTH = 45;


// Declared functions/prototypes
static Mat shift_frame(Mat in_frame, int shiftx, int shifty);
static Mat corner_matching(Mat in_frame, vector<Point> contour, int plusx, int plusy);
static Mat test_edges(Mat in_frame, vector<Point> contour);
static int min_square_dim(Mat in_frame);
static Mat first_frame(Mat in_frame, int framecnt);
static Mat halo_noise_and_center(Mat in_frame, int framecnt);
static void signal_callback_handler(int signum);
static Mat mask_halo(Mat in_frame, int maskwidth);
static vector<vector<Point>> fetch_dynamic_mask(Mat in_frame);
static Mat apply_dynamic_mask(Mat in_frame, vector<vector<Point>> contours, int maskwidth);
static int largest_contour(vector <vector<Point>> contours);
static Mat canny_convert(Mat in_frame, int in_thresh);
static vector <vector<Point>> contours_only(Mat in_frame);
static RotatedRect ellipse_finder(Mat in_frame);
// static int thresh_detect(Mat frame);
static void show_usage(string name);
static std::tuple <float, int> laplace_sum(vector<Point> contour, Mat lapframe);
static void childcheck(int signum);
static int tier_one(int cnt, Mat frame);
static int tier_two(int cnt, Mat frame);
static int tier_three(int cnt, Mat frame, Mat oldframe);
static int tier_four(int cnt, Mat frame, Mat oldframe);
static int parse_checklist(std::string name, std::string value);
int main(int argc, char* argv[]); 