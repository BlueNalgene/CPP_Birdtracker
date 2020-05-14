/*
 * 
 * This is a more efficient frame data extraction program for the LunAero birdtracker project
 * 
 * 
 */

// Includes
#include <algorithm>
#include <ctime>                         // for NULL
//#include <exception>
#include <fcntl.h>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>                      // for exit
#include <string>                        // for string
using std::string;
#include <sys/ipc.h>                     // for IPC_CREAT, key_t
#include <sys/shm.h>                     // for shmat
#include <sys/mman.h>                    // for MAP_ANONYMOUS, MAP_SHARED
#include <unistd.h>                      // for fork, sleep, usleep
#include <vector>                        // for vector
using std::vector;

#include "opencv2/opencv.hpp"
using namespace cv;

// Defined Constants

// Global Variables
bool DEBUG_FRAMES = false;
bool DEBUG_COUT = false;
int STARTUP_CONTOURS;
RotatedRect STOREBOX;
int SIG_ALERT = 0;

// Declared functions
static void show_usage(string name);
int main(int argc, char* argv[]);
static void halo_startup(Mat in_frame, int in_thresh);
static int largest_contour(vector <vector<Point>> contours);
static Mat halo_noise_and_center(Mat in_frame);
static RotatedRect ellipse_finder(Mat in_frame);
static Mat canny_convert(Mat in_frame, int in_thresh);
static vector <vector<Point>> contours_only(Mat in_frame);
static int thresh_detect(Mat frame);

static Mat halo_noise_and_center(Mat in_frame) {
	RotatedRect box = ellipse_finder(in_frame);
	
	int cols = in_frame.size().width;
	int rows = in_frame.size().height;
	
	// create mats from size
	Mat mask_mat = Mat::zeros(Size(cols, rows), CV_8UC1);
	Mat zero_mat = Mat::zeros(Size(cols, rows), CV_8UC3);
	
	// Select filled area with moon
	ellipse(mask_mat, box, 255, -1, LINE_AA);
	in_frame.copyTo(zero_mat, mask_mat);
	
	// Reset our in_frame
	in_frame = zero_mat.clone();

	// Clear memory
	zero_mat = Mat::zeros(Size(cols, rows), CV_8UC1);
	
	// Shift left by X and shift up by Y (distance from box center to frame center
	int xdi = (cols / 2) - box.center.x;
	int ydi = (rows / 2) - box.center.y;
	
	// Use warpaffine for generic transformation
	// FIXME Warp affine is inefficient for what we need here, we should really use:
	// - input(cv::Rect(sx, sy, w - sx, h - sy)).copyTo(output(cv::Rect(0, 0, w - sx, h - sy)));
	// but that requires figuring out how to make the bounding rect behave for the rotatedrect.
	mask_mat = (Mat_<float>(2, 3) << 1, 0, (float) xdi, 0, 1, (float) ydi);
	warpAffine(in_frame, zero_mat, mask_mat, zero_mat.size());
	
	std::cout << (cols/2) << ", " << (rows/2) << std::endl;
	STOREBOX = RotatedRect(Point2f((cols/2), (rows/2)), Size2f(box.size.width*.98, box.size.height*.98), box.angle);
	
	// Put it back to our inframe
	in_frame = zero_mat.clone();
	
	return in_frame;
}

void signal_callback_handler(int signum) {
	if (signum == 2) {
		std::cout << "Caught ctrl+c interrupt signal: " << std::endl;
	}
	// Terminate program
	SIG_ALERT = signum;
	return;
}

static Mat mask_halo(Mat in_frame) {
	// 	// Rezero mat
// 	zero_mat = Mat::zeros(Size(cols, rows), CV_8UC1);
// 	mask_mat = Mat::zeros(Size(cols, rows), CV_8UC1);
// 	
// 	
// 	
// 	// Shrink Ellipse
// 	RotatedRect new_ellipse(Point2f(cols/2, rows/2), Size2f(box.size.width*0.95, box.size.height*0.95), (float)box.angle);
// 
// 	ellipse(in_frame, new_ellipse, 0, 8, LINE_AA);
// 	
// // 	// Make it a filled ellipse this time
// // 	ellipse(mask_mat, new_ellipse, 255, -1, LINE_AA);
// // 	
// // 	// Slap what we have into the mask
// // 	in_frame.copyTo(zero_mat, mask_mat);
// // 	
// // 	// Put it back to our inframe
// // 	in_frame = zero_mat.clone();
	
	ellipse(in_frame, STOREBOX, 0, 20, LINE_AA);

	
	return in_frame;
}

static int largest_contour(vector <vector<Point>> contours) {
	int largest_contour_index = -1;
	int largest_area = 0;
	
	// Finds the largest contour in the "canny_output" of the input frame
	for( size_t i = 0; i< contours.size(); i++ ) {
		double area = contourArea(contours[i]);
	
		if (area > largest_area) {
			largest_area = area;
			largest_contour_index = i;
		}
	}
	return largest_contour_index;
}

static Mat canny_convert(Mat in_frame, int in_thresh) {
	Mat canny_output;
	cvtColor(in_frame, canny_output, COLOR_BGR2GRAY);
	Canny(canny_output.clone(), canny_output, in_thresh, in_thresh*2);
	
	return canny_output;
}

static vector <vector<Point>> contours_only(Mat in_frame) {
	vector <vector<Point>> contours;
	vector<Vec4i> hierarchy;
	findContours(in_frame, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
	return contours;
}

static RotatedRect ellipse_finder(Mat in_frame) {
	
	vector <vector<Point>> contours = contours_only(in_frame);
	
	if (DEBUG_COUT) {
		std::cout << "Number of detected contours in frame: " << contours.size() << std::endl;
	}

	int largest_contour_index = largest_contour(contours);
	
// 	if (DEBUG_COUT) {
// 		std::cout << "largest contour: " << contours[largest_contour_index] << std::endl;
// 	}

	// Fit the largest contour to an ellipse
	RotatedRect box = fitEllipse(contours[largest_contour_index]);
	
	if (DEBUG_COUT) {
		std::cout << "box width: " << box.size.width << std::endl;
		std::cout << "box height: " << box.size.height << std::endl;
		std::cout << "box x: " << box.center.x << std::endl;
		std::cout << "box y: " << box.center.y << std::endl;
		std::cout << "box angle: " << box.angle << std::endl;
	}
	
	return box;
}


static void halo_startup(Mat in_frame, int in_thresh) {
	
	int cols = in_frame.size().width;
	int rows = in_frame.size().height;
	
	Mat ones_mat = Mat::ones(Size(cols, rows), CV_8UC1);
	Mat zero_mat = Mat::zeros(Size(cols, rows), CV_8UC3);
	ones_mat = ones_mat * 255;
	Mat canny_output = canny_convert(in_frame, in_thresh);
	
	RotatedRect box = ellipse_finder(canny_output);
	ellipse(ones_mat, box, 0, 15, LINE_AA);
	
	if (DEBUG_COUT) {
		std::cout << "frame size & ones_mat size & zero_mat size: "
			<< canny_output.size()
			<< " & "
			<< ones_mat.size()
			<< " & "
			<< zero_mat.size()
			<< std::endl;
	}
	
	// Create masked copy
	canny_output.copyTo(zero_mat, ones_mat);
	
	if (DEBUG_FRAMES) {
		imshow("fg_mask", zero_mat);
	}
	
	// Check masked image for contours
	vector <vector<Point>> contours = contours_only(zero_mat);
	STARTUP_CONTOURS = contours.size();
	std::cout << "\n\n\n\ncontours: " << STARTUP_CONTOURS << std::endl;
	
	int keyboard = waitKey(1);
	if (keyboard == 'q' || keyboard == 27) {
		exit(0);
	}
}

static int thresh_detect(Mat in_frame) {
	int out_thresh = 0;
	GaussianBlur(in_frame.clone(), in_frame, Size(5, 5), 0, 0);
	while (true) {
		int thresh_i;
		
		for (thresh_i = 0; thresh_i <256; ++thresh_i) {
			halo_startup(in_frame, thresh_i);
			// if we get to no contours besides the mask, that is the optimal threshold
			if (STARTUP_CONTOURS == 0) {
				out_thresh = thresh_i;
				break;
			}
		}
		if (out_thresh == 0) {
			if (DEBUG_COUT) {
				std::cout << "WARNING: Failed to completely optimize threshold, reverting to 50\% max thresh"
					<< std::endl;
			}
			out_thresh = 127;
		}
		break;
	}
	return out_thresh;
}

static void show_usage(string name) {
	std::cerr << "Usage: "  << " <option(s)> \t\tSOURCES\tDescription\n"
			<< "Options:\n"
			<< "\t-h,--help\t\t\tShow this help message\n"
			<< "\t-i,--input\t\tINPUT\tSpecify path to input video\n"
			<< "\t-do,--debug-output \t\tPrint debugging info to the terminal\n"
			<< "\t-df,--debug-frames \t\tShow gui window of current frame"
			<< std::endl;
}


int main(int argc, char* argv[]) {
	// Capture interrupt signals so we don't create zombie processes
	// FIXME This doesn't work right with all the forks.
	signal(SIGINT, signal_callback_handler);
	
	// Arg Handler --------------------------------------------------------------------------------
	
	// Handle arguments
	if (argc < 3) {
		show_usage(argv[0]);
		return 1;
	}
	
	vector <string> sources;
	string input_file;
	
	for (int i = 1; i < argc; ++i) {
		string arg = argv[i];
		if ((arg == "-h") || (arg == "--help")) {
			show_usage(argv[0]);
			
			return 0;
		} else if ((arg == "-do") || (arg == "--debug-output")) {
			DEBUG_COUT = true;
		} else if ((arg == "-df") || (arg == "--debug-frames")) {
			DEBUG_FRAMES = true;
		} else if ((arg == "-i") || (arg == "--input")) {
			// Make sure we aren't at the end of argv!
			if (i + 1 < argc) {
				// Increment 'i' so we don't get the argument as the next argv[i].
				input_file = argv[++i];
			} else {
				std::cerr << "--input option requires one argument." << std::endl;
				return 1;
			}
		} else {
			sources.push_back(argv[i]);
		}
	}
	
	if (DEBUG_COUT) {
		std::cout << "Using input file: " << input_file << std::endl;
	}
	
	// Instance and Assign ------------------------------------------------------------------------
	// Memory map variables we want to share across forks
	// mm_killcode is a generic "kill this fork" code
	// mm_gotframe reports that the frame acquisition was completed
	// mm_gotconts reports that video processing was completed
	// mm_ranprocs reports that the contours were recorded
	auto *mm_killcode = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_gotframe = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_gotconts = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_localfrm = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_ranprocs = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	
	// OpenCV specific variables
	Ptr <BackgroundSubtractor> pMOG = createBackgroundSubtractorMOG2(100, 16, true);
	Mat fg_mask;
	Mat frame;
	
	// Memory and fork management inits
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, NULL);
	int frame_fd = open("/tmp/file", O_CREAT|O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	
	// Other local variables
	// TODO the count needs to be in mm
	int count = -1;
	int thresh = 0;
	
	
	// Tell OpenCV to open our video and fetch the first frame ------------------------------------
	VideoCapture cap(input_file); // open the default camera
	if (!cap.isOpened())  // check if we succeeded
		return -1;
	// Get the first frame
	cap >> frame;
	
	
	// Memory management init continued -----------------------------------------------------------
	// Memory map slot for video frame
	// This must be called after the first test frame so we have the correct size info
	// e.g. we expect 6220800 bytes for a 3 channel 1920x1080 image
	const size_t shmem_size = frame.total() * frame.elemSize();
	
	if (DEBUG_COUT) {
		std::cout
		<< "frame dimensions: "
		<< frame.size().width
		<< " x "
		<< frame.size().height
		<< " x "
		<< frame.elemSize()
		<< " = "
		<< shmem_size
		<< std::endl
		<< "frame_fd: "
		<< frame_fd
		<< std::endl;
	}
	
	// Resize the frame_fd to fit the frame size we are using
	if (ftruncate(frame_fd, shmem_size) != 0) {
		std::cout << "failed to truncate resize file descriptor" << std::endl;
		return -1;
	}
	
	unsigned char *buf = static_cast <unsigned char*>(mmap(NULL, shmem_size, PROT_READ|PROT_WRITE, MAP_SHARED, frame_fd, 0));
	if (DEBUG_COUT) {
		std::cout << "buf size: " << sizeof(*buf) << std::endl;
	}
	
// 	pthread_mutex_lock(&mutex);
// 	if (DEBUG_COUT) {
// 		std::cout << "mutex locked" << std::endl;
// 	}

	memcpy(buf, frame.ptr(), shmem_size);
	if (DEBUG_COUT) {
		std::cout << "memcpy'd" << std::endl;
	}

// 	pthread_mutex_unlock(&mutex);
// 	if (DEBUG_COUT) {
// 		std::cout << "mutex unlocked, buf size: " << sizeof(*buf) << std::endl;
// 	}
	
	
	// Thresh detect ------------------------------------------------------------------------------
	// Determine the optimal threshold from the first frame
	thresh = thresh_detect(frame);
	std::time_t result = std::time(nullptr);
	if (DEBUG_COUT) {
		std::cout
		<< "filesize: "
		<< shmem_size
		<< std::endl
		<< "thresh: "
		<< thresh
		<< std::endl
		<< "exiting thresh detect mode"
		<< std::endl;
	}
	usleep(500);
	
	// Main Loop ----------------------------------------------------------------------------------
	
	while (1) {
		
		// Define all the mm_ codes as false
		*mm_killcode = false;
		*mm_gotframe = false;
		*mm_gotconts = false;
		*mm_localfrm = false;
		*mm_ranprocs = false;
		
		std::cout << "starting new forks\n\n\n" << std::endl;
		
		int pid1 = fork();
		int pid2 = fork();
		
		if ((pid1 > 0) && (pid2 > 0)) {
			// FORK 0 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			// This fork is the main coordinating fork.
			if (SIG_ALERT != 0) {
				usleep(1000);
				break;
			}
			
			std::cout << "---------divider----------" << std::endl;
			while((*mm_gotframe != true) && (*mm_gotconts != true) && (*mm_ranprocs != true)) {
				usleep(50);
			}
			std::cout
			<< "gotframe code: "
			<< *mm_gotframe
			<< ", gotconts code: "
			<< *mm_gotconts
			<< ", ranprocs code: "
			<< *mm_ranprocs
			<< std::endl;
			//sleep(3);
			if (DEBUG_COUT) {
				std::cout << "sending kill code" << std::endl;
			}
			// Set the local "done" code for this fork in the mm
			*mm_killcode = true;
		} else if ((pid1 == 0) && (pid2 > 0)) {
			// FORK 1 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			// This fork gets the next frame
			// TODO this waits for local_frame to be captured in FORK 2, may be not needed with mutex
			while (*mm_localfrm == false) {
				usleep(50);
			}
			if ((SIG_ALERT == 0) && (*mm_gotframe == false)) {
				cap >> frame;
				++count;
				// NOTE do I need the mutex here?  Does it do anything within a fork?
				pthread_mutex_lock(&mutex);
				memcpy(buf, frame.ptr(), shmem_size);
				pthread_mutex_unlock(&mutex);
				if (DEBUG_COUT) {
					std::cout << "FRAME Number: " << count << std::endl;
				}
				// Set the local "done" code for this fork in the mm
				*mm_gotframe = true;
				// Wait for sync
				while(*mm_killcode != true) {
					usleep(50);
				}
			}
			// This fork dies with dignity
 			exit(0);
			
		} else if ((pid1 > 0) && (pid2 == 0)) {
			// FORK 2 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			// This fork performs image processing on the current frame
			if ((SIG_ALERT == 0) && (*mm_gotconts == false)) {

				Mat working_frame;
				pthread_mutex_lock(&mutex);
				Mat local_frame = cv::Mat(Size(frame.size().width, frame.size().height), 16, buf, frame.elemSize() * frame.size().width);
				pthread_mutex_unlock(&mutex);
				// TODO May be redundant with mutex
				*mm_localfrm = true;

				cvtColor(local_frame, working_frame, COLOR_BGR2GRAY);
				working_frame = halo_noise_and_center(working_frame);
				
				Canny(working_frame.clone(), working_frame, thresh, thresh*2);
				
				// custom threshold
				//cv::threshold(frame, frame, thresh, 255, CV_THRESH_BINARY);
				
				working_frame = mask_halo(working_frame);
				
				// Background subtraction
				pMOG->apply(working_frame.clone(), working_frame, 0.05);
				
				if (DEBUG_COUT) {
					std::cout << "gotconts_TRUE" << std::endl;
				}
				// Set the local "done" code for this fork in the mm
				*mm_gotconts = true;
				// Wait for sync
				while(*mm_killcode != true) {
					usleep(50);
				}
			}
			// This fork dies with dignity
 			exit(0);
			
		} else {
			// FORK 3 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			// This fork processes data collected from previous images
			// TODO port data procs from Python
			if ((SIG_ALERT == 0) && (*mm_ranprocs == false)) {
				if (DEBUG_COUT) {
					std::cout << "running procs" << std::endl;
				}
				// Set the local "done" code for this fork in the mm
				*mm_ranprocs = true;
				// Wait for sync
				while(*mm_killcode != true) {
					usleep(50);
				}
			}
			// This fork dies with dignity
 			exit(0);
		}
		// END of forks ---------------------------------------------------------------------------
	}
	// END of main loop ---------------------------------------------------------------------------
				

// 	//while (std::time(nullptr) < (result + 60)) {
// 	while (SIG_ALERT == 0) {
// 		// Get frame
// 		cap >> frame; // get a new frame from camera
// 
// 		// Update counter
// 		++count;
// 		//if (DEBUG_COUT) {
// 			std::cout << "FRAME Number: " << count << std::endl;
// 		//}
// 
// 		//Mat working_frame = canny_convert(frame, thresh);
// 		Mat working_frame;
// 		
// 		cvtColor(frame, frame, COLOR_BGR2GRAY);
// 		working_frame = halo_noise_and_center(frame);
// 		
// 		Canny(working_frame.clone(), working_frame, thresh, thresh*2);
// 		
// 		// custom threshold
// 		//cv::threshold(frame, frame, thresh, 255, CV_THRESH_BINARY);
// 		
// 		working_frame = mask_halo(working_frame);
// 		
// 		// Background subtraction
// 		pMOG->apply(working_frame.clone(), working_frame, 0.05);
// 
// 		if (DEBUG_FRAMES) {
// 			//imshow("frame", frame);
// 			imshow("Birdtracker: Working Frame", working_frame);
// 		}
// 
// 		// Save foreground mask
// 		string name = "mask_" + std::to_string(count) + ".png";
// 		//imwrite("D:\\SO\\temp\\" + name, fg_mask);
// 
// 		int keyboard = waitKey(1);
// 			if (keyboard == 'q' || keyboard == 27)
// 				break;
// 	}
	// the camera will be deinitialized automatically in VideoCapture destructor
// 	kill(pid1, SIGKILL);
// 	kill(pid2, SIGKILL);
// 	usleep(1000000);
	exit(SIG_ALERT);
 	return 0;
}
