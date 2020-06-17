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

static Mat halo_noise_and_center(Mat in_frame, int framecnt) {
	std::ofstream outell;
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
	
	// Open the outfile to append list of major ellipses
	outell.open("./data/ellipses.csv", std::ios_base::app);
	outell
	<< framecnt
	<< ","
	<< box.center.x
	<< ","
	<< box.center.y
	<< ","
	<< box.size.width
	<< ","
	<< box.size.height
	<< ","
	<< box.angle
	<< std::endl;
	outell.close();
	
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
	findContours(in_frame, contours, hierarchy, RETR_TREE, CHAIN_APPROX_NONE);
	return contours;
}

static RotatedRect ellipse_finder(Mat in_frame) {
	
	vector <vector<Point>> contours = contours_only(in_frame);
	
	if (DEBUG_COUT) {
		std::cout << "Number of detected contours in ellipse frame: " << contours.size() << std::endl;
	}
	
	int largest_contour_index = largest_contour(contours);
	
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
	std::cout << "contours: " << STARTUP_CONTOURS << "\n\n\n\n" << std::endl;
	
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
			if (STARTUP_CONTOURS < 1) {
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

static std::tuple <float, int> laplace_sum(vector<Point> contour, Mat lapframe) {
	float avg;
	int outval = 0;
	int cnt = 0;
	for (size_t i = 0; i<contour.size(); i++) {
		
// 		std::cout << "(x, y, L): (" << contour[i].x << ", " << contour[i].y << ", " << (int)lapframe.at<uchar>(contour[i].x, contour[i].y) << ")"<< std::endl;
		outval +=(int16_t)lapframe.at<int16_t>(contour[i].y, contour[i].x);
		cnt++;
	}
	avg = outval/cnt;
	return {avg, cnt};
}

void childcheck(int signum) {
	wait(NULL);
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
	
	// Touch the output file
	std::ofstream outfile;
	outfile.open("test.csv");
// 	outfile << "Data" << std::endl;
	outfile.close();
	// Touch output ellipse file
	std::ofstream outell;
	outell.open("./data/ellipses.csv");
	outell.close();
	
	// Instance and Assign ------------------------------------------------------------------------
	// Memory map variables we want to share across forks
	// mm_gotframe reports that the frame acquisition was completed
	// mm_gotconts reports that video processing was completed
	// mm_localfrm reports that the current frame was grabbed, proceed getting next frame
	// mm_ranprocs reports that the contours were recorded
	// mm_frmcount stores the frame counter
	auto *mm_gotframe = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_gotconts = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_localfrm = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_ranprocs = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_frmcount = static_cast <int *>(mmap(NULL, sizeof(int), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	*mm_frmcount = -1;
// 	auto *mm_vec1 = static_cast <vector <vector float>>(mmap(NULL, 524288000, \
 		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	
	// OpenCV specific variables
	Ptr <BackgroundSubtractor> pMOG = createBackgroundSubtractorMOG2(100, 16, true);
	Mat fg_mask;
	Mat frame;
	
	// Memory and fork management inits
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, NULL);
	int frame_fd = open("/tmp/file", O_CREAT|O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	int frame_fd2 = open("/tmp/file2", O_CREAT|O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	
	// Other local variables
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
	// Resize the frame_fd to fit the frame size we are using
	if (ftruncate(frame_fd2, shmem_size) != 0) {
		std::cout << "failed to truncate resize file descriptor 2" << std::endl;
		return -1;
	}
	
	
	unsigned char *buf = static_cast <unsigned char*>(mmap(NULL, shmem_size, PROT_READ|PROT_WRITE, MAP_SHARED, frame_fd, 0));
	if (DEBUG_COUT) {
		std::cout << "buf size: " << sizeof(*buf) << std::endl;
	}

	memcpy(buf, frame.ptr(), shmem_size);
	if (DEBUG_COUT) {
		std::cout << "memcpy'd" << std::endl;
	}
	
	unsigned char *buf2 = static_cast <unsigned char*>(mmap(NULL, shmem_size, PROT_READ|PROT_WRITE, MAP_SHARED, frame_fd2, 0));
	if (DEBUG_COUT) {
		std::cout << "buf2 size: " << sizeof(*buf2) << std::endl;
	}
	
	memcpy(buf2, frame.ptr(), shmem_size);
	if (DEBUG_COUT) {
		std::cout << "memcpy'd" << std::endl;
	}
	
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
		// Delared PID here so we can stash the fork in an if statement.
		int pid1;
		//int pid2;
		
		// Define all the mm_ codes as false
		*mm_gotframe = false;
		*mm_gotconts = false;
		*mm_localfrm = false;
		*mm_ranprocs = false;
		
// 		std::cout << "starting new fork\n\n\n" << std::endl;
		
		// If we have not told the program to exit, fork and continue.
		if (SIG_ALERT == 0) {
			pid1 = fork();
			//pid2 = fork();
		} else {
			break;
		}
		
		if (pid1 > 0) {
			// FORK 0 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			// This fork is the main coordinating fork and image processing fork
			// Due to OpenCV limitations, frame fetch and display must happen here to avoid headache
			if (DEBUG_COUT) {
				std::cout << "---------divider----------\nFRAME: " << (*mm_frmcount + 1) << std::endl;
			}
			// Fetch next frame
			cap >> frame;
			++*mm_frmcount;
			// Declare this here so it gets destroyed every loop
			Mat working_frame;
			// Image processing operations
			cvtColor(frame, working_frame, COLOR_BGR2GRAY);
			working_frame = halo_noise_and_center(working_frame.clone(), *mm_frmcount);
			// Store centered image into the memory buffer 2
			memcpy(buf2, working_frame.ptr(), shmem_size);
			Canny(working_frame.clone(), working_frame, thresh, thresh*2);
			working_frame = mask_halo(working_frame.clone());
			pMOG->apply(working_frame.clone(), working_frame, 0.05);
			// Wait for fork 1 to have the previous frame stored.  Prevent unlikely race condition.
			while (*mm_gotframe == false) {
				usleep(50);
			}
			*mm_gotframe = false;
			// Store this image into the memory buffer
			memcpy(buf, working_frame.ptr(), shmem_size);
			// Show frame
			if (DEBUG_FRAMES) {
				imshow("fg_mask", working_frame);
				waitKey(1);
			}
			// Ensure the other fork is done
			wait(0);
		} else {
			// FORK 1 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			// This fork performs data processing on contours in the current frame
			Point2f center;
			float radius;
			int local_count = *mm_frmcount;
			
			if (SIG_ALERT == 0) {
				// Grab the previous frame and store it locally
				Mat local_frame = cv::Mat(Size(frame.size().width, frame.size().height), CV_8UC1, buf, 1 * frame.size().width);
				Mat lap_frame = cv::Mat(Size(frame.size().width, frame.size().height), CV_8UC1, buf2, 1 * frame.size().width);
				
				if (DEBUG_COUT) {
					std::cout << "operating on data from frame: " << local_count << std::endl;
				}
				imwrite("./tst.png", lap_frame);
				imwrite("./tst2.png", local_frame);
				*mm_gotframe = true;
				// Get and store Laplacian version of frame
				Laplacian(lap_frame.clone(), lap_frame, CV_16S, 1, 1, 0, BORDER_REPLICATE);
				
// 				FileStorage file("some_name.xml", FileStorage::WRITE);
// 				file << "matName" << lap_frame;
				
				
				// Get contours
				if (local_count > -1) {
					vector <vector<Point>> contours = contours_only(local_frame);
					if (DEBUG_COUT) {
						std::cout << "Number of contours in data proc frame: " << contours.size() << std::endl;
					}
// 					if (contours.size() > 100) {
// 						if (DEBUG_COUT) {
// 							std::cout << "skipping this frame in log, greater than 100 contours" << std::endl;
// 						}
// 						return 0;
// 					}
					
					// Cycle through the contours
					for (auto vec : contours) {
						// Greater than one includes lunar ellipse
						if (vec.size() > 1) {
							if (DEBUG_COUT) {
								std::cout << vec << std::endl;
							}
							float contarea = contourArea(vec);
							minEnclosingCircle(vec, center, radius);
							float lapsum;
							int strictperi;
							std::tie(lapsum, strictperi) = laplace_sum(vec, lap_frame);
							// Open the outfile to append
							outfile.open("test.csv", std::ios_base::app);
							outfile
							<< local_count
							<< ","
							<< center.x
							<< ","
							<< center.y
							<< ","
							<< radius
							<< ","
							<< lapsum
							<< ","
							<< strictperi
							<< std::endl;
							outfile.close();
							if (DEBUG_COUT) {
								std::cout
								<< "area = "
								<< contarea
								<< std::endl
								<< "cent rad = "
								<< center
								<< " "
								<< radius
								<< std::endl;
							}
						} // END vector size if
					} // END contour for loop
				} // END skip -1
			} // END if SIG_ALERT==0
			// This fork dies with dignity
 			return 0;
		}
		
		
		
// 		if ((pid1 > 0) && (pid2 > 0)) {
// 			// FORK 0 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 			// This fork is the main coordinating fork.
// 			if (DEBUG_COUT) {
// 				std::cout << "---------divider----------" << std::endl;
// 			}
// 			// Wait for each child to die.
// 			wait(0);
// 			wait(0);
// 			wait(0);
// 			if (DEBUG_COUT) {
// 				std::cout
// 				<< "gotframe code: "
// 				<< *mm_gotframe
// 				<< ", gotconts code: "
// 				<< *mm_gotconts
// 				<< ", ranprocs code: "
// 				<< *mm_ranprocs
// 				<< std::endl;
// 			}
// 			if (DEBUG_FRAMES) {
// 				Mat local_frame = cv::Mat(Size(frame.size().width, frame.size().height), 16, buf, frame.elemSize() * frame.size().width);
// 				imshow("fg_mask", local_frame);
// 				waitKey(1);
// 			}
// 		} else if ((pid1 == 0) && (pid2 > 0)) {
// 			// FORK 1 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 			// This fork gets the next frame
// 			// TODO this waits for local_frame to be captured in FORK 2, may be not needed with mutex
// 			while (*mm_localfrm == false) {
// 				usleep(50);
// 			}
// 			if ((SIG_ALERT == 0) && (*mm_gotframe == false)) {
// 				cap >> frame;
// 				++*mm_frmcount;
// 				// NOTE do I need the mutex here?  Does it do anything within a fork?
// // 				pthread_mutex_lock(&mutex);
// 				memcpy(buf, frame.ptr(), shmem_size);
// // 				pthread_mutex_unlock(&mutex);
// 				if (DEBUG_COUT) {
// 					std::cout << "FRAME Number: " << *mm_frmcount << std::endl;
// 				}
// 				// Set the local "done" code for this fork in the mm
// 				*mm_gotframe = true;
// 			}
// 			// This fork dies with dignity
//  			exit(0);
// 			
// 		} else if ((pid1 > 0) && (pid2 == 0)) {
// 			// FORK 2 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 			// This fork performs image processing on the current frame
// 			if ((SIG_ALERT == 0) && (*mm_gotconts == false)) {
// 
// 				Mat working_frame;
// // 				pthread_mutex_lock(&mutex);
// 				Mat local_frame = cv::Mat(Size(frame.size().width, frame.size().height), 16, buf, frame.elemSize() * frame.size().width);
// // 				pthread_mutex_unlock(&mutex);
// 				// TODO May be redundant with mutex
// 				*mm_localfrm = true;
// 
// 				cvtColor(local_frame, working_frame, COLOR_BGR2GRAY);
// 				working_frame = halo_noise_and_center(working_frame);
// 				
// 				Canny(working_frame.clone(), working_frame, thresh, thresh*2);
// 				
// 				// custom threshold
// 				//cv::threshold(frame, frame, thresh, 255, CV_THRESH_BINARY);
// 				
// 				working_frame = mask_halo(working_frame);
// 				
// 				// Background subtraction
// 				pMOG->apply(working_frame.clone(), working_frame, 0.05);
// 				
// 				if (DEBUG_COUT) {
// 					std::cout << "gotconts_TRUE" << std::endl;
// 				}
// 				
// 				
// 				// Set the local "done" code for this fork in the mm
// 				*mm_gotconts = true;
// 			}
// 			// This fork dies with dignity
//  			exit(0);
// 			
// 		} else {
// 			// FORK 3 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 			// This fork processes data collected from previous images
// 			// TODO port data procs from Python
// 			if ((SIG_ALERT == 0) && (*mm_ranprocs == false)) {
// 				if (DEBUG_COUT) {
// 					std::cout << "running procs" << std::endl;
// 				}
// 				// Set the local "done" code for this fork in the mm
// 				*mm_ranprocs = true;
// 			}
// 			// This fork dies with dignity
//  			exit(0);
// 		}
		
	} // END of forks -----------------------------------------------------------------------------
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
