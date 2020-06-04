


// Includes
#include <algorithm>
#include <ctime>                         // for NULL
#include <errno.h>                       // for error codes
//#include <exception>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>                      // for exit
#include <string>                        // for string
using std::string;
#include <sys/ipc.h>                     // for IPC_CREAT, key_t
#include <sys/shm.h>                     // for shmat
#include <sys/stat.h>                    // for mkdir
#include <sys/mman.h>                    // for MAP_ANONYMOUS, MAP_SHARED
#include <sys/wait.h>
#include <unistd.h>                      // for fork, sleep, usleep
#include <vector>                        // for vector
using std::vector;

#include "opencv2/opencv.hpp"
using namespace cv;

bool DEBUG_FRAMES = false;
bool DEBUG_COUT = false;

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

static Mat halo(Mat in_frame) {
	Mat working_frame;
	vector <vector<Point>> contours;
	vector<Vec4i> hierarchy;
	
	cvtColor(in_frame, working_frame, COLOR_BGR2GRAY);
	findContours(working_frame, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
	int largest_contour_index = largest_contour(contours);
	
	// Fit the largest contour to an ellipse
	RotatedRect box = fitEllipse(contours[largest_contour_index]);
	
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
	// Put it back to our inframe
	in_frame = zero_mat.clone();
	
	return in_frame;
}

class CSVRow {
	public:
		std::string const &operator[](std::size_t index) const {
			return m_data[index];
		}
		std::size_t size() const {
			return m_data.size();
		}
		void readNextRow(std::istream &str) {
			std::string line;
			std::getline(str, line);
			std::stringstream lineStream(line);
			std::string cell;
			
			m_data.clear();
			while(std::getline(lineStream, cell, ',')) {
				m_data.push_back(cell);
			}
			// This checks for a trailing comma with no data after it.
			if (!lineStream && cell.empty()) {
				// If there was a trailing comma then add an empty element.
				m_data.push_back("");
			}
		}
	private:
		std::vector<std::string> m_data;
};

std::istream &operator >> (std::istream &str, CSVRow &data) {
	data.readNextRow(str);
	return str;
}



class CSVIterator{
	public:
		typedef std::input_iterator_tag iterator_category;
		typedef CSVRow value_type;
		typedef std::size_t difference_type;
		typedef CSVRow* pointer;
		typedef CSVRow& reference;

		CSVIterator(std::istream& str):m_str(str.good()?&str:NULL) { ++(*this); }
		CSVIterator():m_str(NULL) {}

		// Pre Increment
		CSVIterator& operator++() {
			if (m_str) {
				if (!((*m_str) >> m_row)) {
					m_str = NULL;
				}
			}
			return *this;
		}
		// Post increment
		CSVIterator operator++(int) {
			CSVIterator tmp(*this);
			++(*this);
			return tmp;
		}
		CSVRow const& operator*() const {
			return m_row;
		}
		CSVRow const* operator->() const {
			return &m_row;
		}

		bool operator==(CSVIterator const& rhs) {
			return ((this == &rhs) || ((this->m_str == NULL) && (rhs.m_str == NULL)));
		}
		bool operator!=(CSVIterator const& rhs) {
			return !((*this) == rhs);
		}
	private:
		std::istream* m_str;
		CSVRow m_row;
};

int important_frames(string vidpath, string path) {
	int frame_fd = open("/tmp/file", O_CREAT|O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	const size_t shmem_size = 1920 * 1080 * 3;
	if (ftruncate(frame_fd, shmem_size) != 0) {
		std::cout << "failed to truncate resize file descriptor" << std::endl;
		return -1;
	}
	
	unsigned char *buf = static_cast <unsigned char*>(mmap(NULL, shmem_size, PROT_READ|PROT_WRITE, MAP_SHARED, frame_fd, 0));
	if (DEBUG_COUT) {
		std::cout << "buf size: " << sizeof(*buf) << std::endl;
	}

	
	
	const string input_csv = "./data/output.csv";
	std::ifstream file(input_csv);
	
	const Scalar color1 = Scalar(0, 0, 255);
	
	VideoCapture cap(vidpath); // open the default camera
	if(!cap.isOpened()) {
		return -1;
	}
	
	Mat img;
	int cnt = -1;
	
	for (CSVIterator loop(file); loop != CSVIterator(); ++loop) {
		int frame1 = int(std::stof((*loop)[0]));
		int xxx1 = int(std::stof((*loop)[1]));
		int yyy1 = int(std::stof((*loop)[2]));
		double rrr1 = double(std::stof((*loop)[3]));
		int frame2 = int(std::stof((*loop)[4]));
		int xxx2 = int(std::stof((*loop)[5]));
		int yyy2 = int(std::stof((*loop)[6]));
		double rrr2 = double(std::stof((*loop)[7]));
		int frame3 = int(std::stof((*loop)[8]));
		int xxx3 = int(std::stof((*loop)[9]));
		int yyy3 = int(std::stof((*loop)[10]));
		double rrr3 = double(std::stof((*loop)[11]));
		
		if (cnt == frame1) {
// 			std::stringstream filename;
// 			filename.clear();
// 			filename << "./data/" << std::setfill('0') << std::setw(8) << frame1 << ".png";
// 			std::cout << filename.str() << std::endl;
// 			img = imread(filename.str(), CV_LOAD_IMAGE_COLOR);
			memcpy(img.ptr(), buf, shmem_size);
			if (DEBUG_COUT) {
				std::cout << "memcpy'd in" << std::endl;
			}
			circle(img, Point(xxx1, yyy1), (int(rrr1) + 3), color1, 2, 8, 0);
			memcpy(buf, img.ptr(), shmem_size);
			if (DEBUG_COUT) {
				std::cout << "memcpy'd" << std::endl;
			}
			
		} else {
			std::stringstream filename;
			filename.str(std::string());
			
			if (cnt > 0) {
				if (DEBUG_COUT) {
				std::cout << "memcpy'd in" << std::endl;
				}
				filename << "./data/frames/" << std::setfill('0') << std::setw(8) << cnt << ".png";
				imwrite(filename.str(), img);
				filename.str(std::string());
			}
// 			filename << "./data/frames" << std::setfill('0') << std::setw(8) << frame1 << ".png";
// 			std::cout << filename.str() << std::endl;
			cap.set(CAP_PROP_POS_FRAMES, double(frame1));
			double currentPos = cap.get(CV_CAP_PROP_POS_FRAMES);
			if (currentPos != frame1) {
				std::cout << "Requesting frame " << frame1 << " but got == " << currentPos << std::endl;
			}
			cap.read(img);
			img = halo(img.clone());
			circle(img, Point(xxx1, yyy1), (int(rrr1) + 3), color1, 2, 8, 0);
			memcpy(buf, img.ptr(), shmem_size);
			if (DEBUG_COUT) {
				std::cout << "memcpy'd" << std::endl;
			}
			cnt = frame1;
		}
		if (DEBUG_FRAMES) {
			imshow("output", img);
			waitKey(1);
		}
// 		std::cout << "Elements(" << frame1 << xxx1 << yyy1 << rrr1 << std::endl;
	}
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
	const string frame_out_path = "./data/frames/";
	
	
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
	
	const int dir_err = mkdir(frame_out_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (-1 == dir_err){
		int errnocode = errno;
		if (errnocode != EEXIST) {
			printf("Error creating directory!\n");
			exit(1);
		} else {
			std::cout << "The path " << frame_out_path << " exists. Continuing" << std::endl;
		}
	}
	
	important_frames(input_file, frame_out_path);
	
// 	// Touch the output file
// 	std::ofstream outfile;
// 	outfile.open("./data/test.txt");
// // 	outfile << "Data" << std::endl;
// 	outfile.close();
	
	return 0;
}
