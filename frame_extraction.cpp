/*
 * 
 * This is a more efficient frame data extraction program for the LunAero birdtracker project
 * 
 * 
 */

#include "frame_extraction.hpp"


static Mat shift_frame(Mat in_frame, int shiftx, int shifty) {
	Mat zero_mask = Mat::zeros(in_frame.size(), in_frame.type());
	
	
	if (DEBUG_COUT) {
		std::cout << "SHIFT_X: " << shiftx << std::endl << "SHIFT_Y: " << shifty << std::endl;
	}
	
	if ((shiftx < 0) && (shifty < 0)) { // move right and down
		if (DEBUG_COUT) {
			std::cout << "shifting case 1 - move right and down" << std::endl;
		}
		in_frame(Rect(0, 0, BOXSIZE-abs(shiftx), BOXSIZE-abs(shifty)))
		.copyTo(zero_mask(Rect(abs(shiftx), abs(shifty), BOXSIZE-abs(shiftx), BOXSIZE-abs(shifty))));
	} else if (shiftx < 0) { // move right and maybe up
		if (DEBUG_COUT) {
			std::cout << "shifting case 2 - move right and maybe up" << std::endl;
		}
		in_frame(Rect(0, abs(shifty), BOXSIZE-abs(shiftx), BOXSIZE-abs(shifty)))
		.copyTo(zero_mask(Rect(abs(shiftx), 0, BOXSIZE-abs(shiftx), BOXSIZE-abs(shifty))));
	} else if (shifty < 0) { // move down and maybe left
		if (DEBUG_COUT) {
			std::cout << "shifting case 3 - move down and maybe left" << std::endl;
		}
		in_frame(Rect(abs(shiftx), 0, BOXSIZE-abs(shiftx), BOXSIZE-abs(shifty)))
		.copyTo(zero_mask(Rect(0, abs(shifty), BOXSIZE-abs(shiftx), BOXSIZE-abs(shifty))));
	} else {  // positive moves up and left
		if (DEBUG_COUT) {
			std::cout << "shifting case 4 - move up and left" << std::endl;
		}
		in_frame(Rect(abs(shiftx), abs(shifty), BOXSIZE-abs(shiftx), BOXSIZE-abs(shifty)))
		.copyTo(zero_mask(Rect(0, 0, BOXSIZE-abs(shiftx), BOXSIZE-abs(shifty))));
	}
	
	in_frame = zero_mask;
	
	return in_frame;
}

static Mat corner_matching(Mat in_frame, vector<Point> contour, int plusx, int plusy) {
	int shiftx = 0;
	int shifty = 0;
	
	Point local_tl = boundingRect(contour).tl();
	Point local_br = boundingRect(contour).br();
	
	if (DEBUG_COUT) {
		std::cout << "PLUSX: " << plusx << " PLUSY: " << plusy << std::endl;
		std::cout << "LOCAL_TL = (" << local_tl.x << ", " << local_tl.y << ")" << std::endl;
		std::cout << "LOCAL_BR = (" << local_br.x << ", " << local_br.y << ")" << std::endl;
		std::cout << "ORIG_TL = (" << ORIG_TL.x << ", " << ORIG_TL.y << ")" << std::endl;
		std::cout << "ORIG_BR = (" << ORIG_BR.x << ", " << ORIG_BR.y << ")" << std::endl;
	}
	
	if ((abs(plusx) > EDGETHRESH) && (abs(plusx) > EDGETHRESH)) {
		if (plusx > 0) {
			if (plusy > 0) {
				// +x, +y (touching right and bottom edges)
				shiftx = (local_tl.x - ORIG_TL.x);
				shifty = (local_tl.y - ORIG_TL.y);
			} else {
				// +x, -y (touching right and top edges)
				shiftx = (local_tl.x - ORIG_TL.x);
				shifty = (local_br.y - ORIG_BR.y);
			}
		} else {
			if (plusy > 0) {
				// -x, +y (touching left and bottom edges)
				shiftx = (local_br.x - ORIG_BR.x);
				shifty = (local_tl.y - ORIG_TL.y);
			} else {
				// -x, -y (touching left and top edges)
				shiftx = (local_br.x - ORIG_BR.x);
				shifty = (local_br.y - ORIG_BR.y);
			}
		}
	} else if (abs(plusx) > EDGETHRESH) {
		if (plusx > 0) {
			// +x (touching right edge)
			shiftx = (local_tl.x - ORIG_TL.x);
			
		} else {
			// -x (touching left edge)
			shiftx = (local_br.x - ORIG_BR.x);
			
		}
	} else if (abs(plusy) > EDGETHRESH) {
		if (plusy > 0) {
			// +y (touching bottom edge)
			shifty = (local_tl.y - ORIG_TL.y);
			
		} else {
			// -y (touching top edge)
			shifty = (local_br.y - ORIG_BR.y);
			
		}
	}
	
	in_frame = shift_frame(in_frame, shiftx, shifty);
	
	
	return in_frame;
}

static Mat test_edges(Mat in_frame, vector<Point> contour) {
	Moments M = moments(contour);
	Point cen(int(M.m10/M.m00), int(M.m01/M.m00));
	if (DEBUG_COUT) {
		std::cout << "centroid: " << cen << std::endl;
	}
	
	// Targeting reticle for debugging
// 	circle(in_frame, cen, 3, Scalar(255, 0, 0), 1, 8, 0);
	
	Rect rect  = boundingRect(contour);
	int to_top = int(M.m01/M.m00) - rect.tl().y;
	int to_right = rect.br().x - int(M.m10/M.m00);
	int to_bottom = rect.br().y - int(M.m01/M.m00);
	int to_left = int(M.m10/M.m00) - rect.tl().x;
	
	int xsize = rect.br().x - rect.tl().x;
	int ysize = rect.br().y - rect.tl().y;
	
	std::cout << to_top << "," << to_right << "," << to_bottom << "," << to_left << std::endl;
	
	int plusx = 0;
	int plusy = 0;
	if (to_top != to_bottom) {
		plusy = to_top - to_bottom;
	}
	if (to_left != to_right) {
		plusx = to_left - to_right;
	}
	
	std::cout << plusx << ", " << plusy << std::endl;
	
	if ((abs(plusx) > EDGETHRESH) || (abs(plusy) > EDGETHRESH)) {
		
		if (DEBUG_COUT) {
			std::cout << "Activating Corner Matching" << std::endl;
		}
		in_frame = corner_matching(in_frame, contour, plusx, plusy);
	}
	
	return in_frame;
}

static int min_square_dim(Mat in_frame) {
	BOXSIZE = min(in_frame.rows, in_frame.cols);
	if (DEBUG_COUT) {
		std::cout << "Cutting frame to size: (" << BOXSIZE << ", " << BOXSIZE << ")" << std::endl;
	}
	return 0;
}



static Mat first_frame(Mat in_frame, int framecnt) {
	min_square_dim(in_frame);
	
	std::ofstream outell;
	vector <vector<Point>> contours = contours_only(in_frame);
	
	if (DEBUG_COUT) {
		std::cout << "Number of detected contours in ellipse frame: " << contours.size() << std::endl;
	}
	
	int largest_contour_index = largest_contour(contours);
	
	// Store original area and perimeter of first frame
	ORIG_AREA = contourArea(contours[largest_contour_index]);
	ORIG_PERI = arcLength(contours[largest_contour_index], true);
	ORIG_VERT = boundingRect(contours[largest_contour_index]).height;
	ORIG_HORZ = boundingRect(contours[largest_contour_index]).width;
	
	
	// Fit the largest contour to an ellipse
	RotatedRect box = fitEllipse(contours[largest_contour_index]);
	
	// Use minimum enclosing circle to get max diameter of moon ellipse
	Point2f center;
	minEnclosingCircle(contours[largest_contour_index], center, ELL_RAD);
	
	if (DEBUG_COUT) {
		std::cout << "box width: " << box.size.width << std::endl;
		std::cout << "box height: " << box.size.height << std::endl;
		std::cout << "box x: " << box.center.x << std::endl;
		std::cout << "box y: " << box.center.y << std::endl;
		std::cout << "box angle: " << box.angle << std::endl;
	}
	
	framecnt = framecnt + 1;
	
// 	// Select filled area with moon
// 	ellipse(mask_mat, box, 255, -1, LINE_AA);
	
	// Create rect representing the image
	Rect image_rect = Rect({}, in_frame.size());
	Rect roi = Rect(box.center.x-(BOXSIZE/2), box.center.y-(BOXSIZE/2), BOXSIZE, BOXSIZE);

	// Find intersection, i.e. valid crop region
	Rect intersection = image_rect & roi;

	// Move intersection to the result coordinate space
	Rect inter_roi = intersection - roi.tl();

	// Create black image and copy intersection
	Mat crop = Mat::zeros(roi.size(), in_frame.type());
	in_frame(intersection).copyTo(crop(inter_roi));
	
	in_frame = crop;	
	
	// Open the outfile to append list of major ellipses
	outell.open(ELLIPSEDATA, std::ios_base::app);
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
	<< ","
	<< ELL_RAD
	<< std::endl;
	outell.close();
	
	if (DEBUG_COUT) {
		std::cout
		<< "ORIG_AREA = "
		<< ORIG_AREA
		<< std::endl
		<< "ORIG_PERI = "
		<< ORIG_PERI
		<< std::endl;
	}
	
	// Now that the contour has moved, fetch the corners
	contours = contours_only(in_frame);
	vector<Point> bigone = contours[largest_contour(contours)];
	ORIG_TL = boundingRect(bigone).tl();
	ORIG_BR = boundingRect(bigone).br();
	
	return in_frame;
}






static Mat halo_noise_and_center(Mat in_frame, int framecnt) {
	// Find largest ellipse
	RotatedRect box = ellipse_finder(in_frame);
	// HACK increase the framecnt here
	framecnt = framecnt + 1;
	
	// Create rect representing the image
	Rect image_rect = Rect({}, in_frame.size());
	Rect roi  = Rect(box.center.x-(BOXSIZE/2), box.center.y-(BOXSIZE/2), BOXSIZE, BOXSIZE);

	// Find intersection, i.e. valid crop region
	Rect intersection = image_rect & roi;

	// Move intersection to the result coordinate space
	Rect inter_roi = intersection - roi.tl();

	// Create black image and copy intersection
	Mat crop = Mat::zeros(roi.size(), in_frame.type());
	in_frame(intersection).copyTo(crop(inter_roi));
	in_frame = crop;
	
	// Open the outfile to append list of major ellipses
	std::ofstream outell;
	outell.open(ELLIPSEDATA, std::ios_base::app);
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
	<< ","
	<< ELL_RAD
	<< std::endl;
	outell.close();
	
	vector <vector<Point>> contours = contours_only(in_frame);
	int largest = largest_contour(contours);
	in_frame = test_edges(in_frame, contours[largest]);
	
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

static Mat mask_halo(Mat in_frame, int maskwidth) {
	ellipse(in_frame, STOREBOX, 0, maskwidth, LINE_AA);
	return in_frame;
}

static vector <vector<Point>> fetch_dynamic_mask(Mat in_frame) {
	vector <vector<Point>> contours = contours_only(in_frame);
	int index = largest_contour(contours);
	vector<Point> maxcont = contours[index];
	vector <vector<Point>> output;
	output.push_back(maxcont);
	return output;
}

static Mat apply_dynamic_mask(Mat in_frame, vector <vector<Point>> contours, int maskwidth) {
// 	std::cout << "contour " << contours << std::endl;
	drawContours(in_frame, contours, -1, 0, maskwidth, LINE_8);
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
	
	// Check spokes
// 	center_spokes(in_frame, contours[largest_contour_index]);
	
	// Use minimum enclosing circle to get max diameter of moon ellipse
	Point2f center;
	minEnclosingCircle(contours[largest_contour_index], center, ELL_RAD);
	
	if (DEBUG_COUT) {
		std::cout << "box width: " << box.size.width << std::endl;
		std::cout << "box height: " << box.size.height << std::endl;
		std::cout << "box x: " << box.center.x << std::endl;
		std::cout << "box y: " << box.center.y << std::endl;
		std::cout << "box angle: " << box.angle << std::endl;
	}
	
	
	
	return box;
}

// static int thresh_detect(Mat in_frame) {
// 	// THIS IS REQUIRED AND I DON"T KNOW WHY!!!!!
// 	GaussianBlur(in_frame.clone(), in_frame, Size(5, 5), 0, 0);
// 
// 	return 0;
// }

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

int tier_one(int cnt, Mat frame) {
	Point2f center;
	float radius;
	std::ofstream outfile;
// 	vector <vector<Point>> dymask = fetch_dynamic_mask(frame);
	adaptiveThreshold(frame.clone(), frame, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 65, 35);
	frame = mask_halo(frame.clone(), 40);
// 	frame = apply_dynamic_mask(frame.clone(), dymask, 25);
	vector <vector<Point>> contours = contours_only(frame);
	cnt = cnt + 1;
	
	if (DEBUG_COUT) {
		std::cout << "Number of contours in tier 1 pass for frame " << cnt << ": " << contours.size() << std::endl;
	}
	outfile.open(TIER1FILE, std::ios_base::app);
	// Cycle through the contours
	for (auto vec : contours) {
		// Greater than one includes lunar ellipse
		if (vec.size() > 1) {
			minEnclosingCircle(vec, center, radius);
			// Open the outfile to append
			outfile
			<< cnt
			<< ","
			<< center.x
			<< ","
			<< center.y
			<< ","
			<< radius
			<< std::endl;
			
		}
	}
	outfile.close();
	return 0;
}

int tier_two(int cnt, Mat frame) {
	Point2f center;
	float radius;
	std::ofstream outfile;
// 	vector <vector<Point>> dymask = fetch_dynamic_mask(frame);
	adaptiveThreshold(frame.clone(), frame, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 65, 20);
	frame = mask_halo(frame.clone(), 40);
// 	frame = apply_dynamic_mask(frame.clone(), dymask, 25);
	vector <vector<Point>> contours = contours_only(frame);
	cnt = cnt + 1;
	
	if (DEBUG_COUT) {
		std::cout << "Number of contours in tier 2 pass for frame " << cnt << ": " << contours.size() << std::endl;
	}
	outfile.open(TIER2FILE, std::ios_base::app);
	// Cycle through the contours
	for (auto vec : contours) {
		// Greater than one includes lunar ellipse
		if (vec.size() > 1) {
			minEnclosingCircle(vec, center, radius);
			// Open the outfile to append
			
			outfile
			<< cnt
			<< ","
			<< center.x
			<< ","
			<< center.y
			<< ","
			<< radius
			<< std::endl;
		}
	}
	outfile.close();
	return 0;
}

int tier_three(int cnt, Mat frame, Mat oldframe) {
	Point2f center;
	float radius;
	std::ofstream outfile;
	Mat scaleframe;
	vector <vector<Point>> dymask = fetch_dynamic_mask(frame);
	cnt = cnt + 1;
	
	/* Eli Method for Tier 3 */
	Laplacian(frame.clone(), frame, CV_32F, 11, 0.0001, 0, BORDER_DEFAULT);
	Laplacian(oldframe.clone(), oldframe, CV_32F, 11, 0.0001, 0, BORDER_DEFAULT);
	GaussianBlur(frame.clone(), frame, Size(11, 11), 1, 1, BORDER_DEFAULT);
	GaussianBlur(oldframe.clone(), oldframe, Size(11, 11), 1, 1, BORDER_DEFAULT);
	scaleframe = frame.clone() - oldframe.clone();
	scaleframe = scaleframe.clone() > 40;
	/* end Eli Method */
	
	scaleframe = apply_dynamic_mask(scaleframe.clone(), dymask, 45);
	vector <vector<Point>> contours = contours_only(scaleframe);
	
	if (DEBUG_COUT) {
		std::cout << "Number of contours in tier 3 pass for frame " << cnt << ": " << contours.size() << std::endl;
	}
	outfile.open(TIER3FILE, std::ios_base::app);
	// Cycle through the contours
	for (auto vec : contours) {
		// Greater than one includes lunar ellipse
		if (vec.size() > 1) {
			minEnclosingCircle(vec, center, radius);
			// Open the outfile to append
			outfile
			<< cnt
			<< ","
			<< center.x
			<< ","
			<< center.y
			<< ","
			<< radius
			<< std::endl;
			
		}
	}
	outfile.close();
}

int tier_four(int cnt, Mat frame, Mat oldframe) {
	Point2f center;
	float radius;
	std::ofstream outfile;
	Mat scaleframe;
	vector <vector<Point>> dymask = fetch_dynamic_mask(frame);
// 	cnt = cnt + 1;
	
// 	/* Eli Method for Tier 3 */
// 	Laplacian(frame.clone(), frame, CV_32F, 11, 0.0001, 0, BORDER_DEFAULT);
// 	Laplacian(oldframe.clone(), oldframe, CV_32F, 11, 0.0001, 0, BORDER_DEFAULT);
// 	GaussianBlur(frame.clone(), frame, Size(11, 11), 1, 1, BORDER_DEFAULT);
// 	GaussianBlur(oldframe.clone(), oldframe, Size(11, 11), 1, 1, BORDER_DEFAULT);
// 	Mat scaleframe = frame.clone() - oldframe.clone();
// 	scaleframe = scaleframe.clone() > 40;
// 	/* end Eli Method */
	
	/* Canny Method */
	
// 	Canny(frame.clone(), scaleframe, 25, 75, 3);
	
	/* end Canny Method*/
	
// 	/* UnCanny Method for Tier 3 */
// 	GaussianBlur(frame.clone(), frame, Size(11, 11), 1, 1, BORDER_DEFAULT);
// 	
// 	adaptiveThreshold(frame.clone(), frame, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 35, 5);
// 	adaptiveThreshold(oldframe.clone(), oldframe, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 35, 5);
// 	
// 	Mat ix1, iy1, ix2, iy2, ii1, ii2;
// 	Sobel(frame.clone(), ix1, CV_32F, 1, 0);
// 	Sobel(frame.clone(), iy1, CV_32F, 0, 1);
// 	Sobel(oldframe.clone(), ix2, CV_32F, 1, 0);
// 	Sobel(oldframe.clone(), iy2, CV_32F, 0, 1);
// 	pow(ix1.clone(), 2, ix1);
// 	pow(iy1.clone(), 2, iy1);
// 	pow(ix2.clone(), 2, ix2);
// 	pow(iy2.clone(), 2, ix2);
// 	add(ix1, iy1, ii1);
// 	add(ix2, iy2, ii2);
// 	sqrt(ii1.clone(), ii1);
// 	sqrt(ii2.clone(), ii2);
// 	
// 	pow(ii1.clone(), 2, ii1);
// 	pow(ii2.clone(), 2, ii2);
// 	add(ii1, ii2, frame);
// 	sqrt(frame.clone(), frame);
// 	
// // 	subtract(frame.clone(), oldframe.clone(), frame);
// // 	GaussianBlur(oldframe.clone(), oldframe, Size(11, 11), 1, 1, BORDER_DEFAULT);
// // 	double minval, maxval;
// // 	minMaxLoc(frame, &minval, &maxval);
// // 	frame = frame.clone() / maxval * 255;
// 	
// 	convertScaleAbs(frame.clone(), scaleframe);
// 	ximgproc::thinning(scaleframe.clone(), scaleframe, 0);
// 	
// 	/* end UnCanny Method */
	
	/* UnCanny v2 */
	
	subtract(frame.clone(), oldframe.clone(), frame);
	adaptiveThreshold(frame.clone(), frame, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 35, 5);
	Mat ix1, iy1, ii1;
	Sobel(frame.clone(), ix1, CV_32F, 1, 0);
	Sobel(frame.clone(), iy1, CV_32F, 0, 1);
	pow(ix1.clone(), 2, ix1);
	pow(iy1.clone(), 2, iy1);
	add(ix1, iy1, ii1);
	sqrt(ii1.clone(), ii1);
	convertScaleAbs(ii1.clone(), ii1);
	GaussianBlur(ii1.clone(), ii1, Size(11, 11), 1, 1, BORDER_DEFAULT);
	ximgproc::thinning(ii1.clone(), scaleframe, 0);
	
	/* end UnCanny v2 */
	
	
	scaleframe = apply_dynamic_mask(scaleframe.clone(), dymask, 45);
// 	imwrite("./tstx.png", frame);
// 	imwrite("./tsty.png", scaleframe);
	vector <vector<Point>> contours = contours_only(scaleframe);
	
	if (DEBUG_COUT) {
		std::cout << "Number of contours in tier 4 pass for frame " << cnt << ": " << contours.size() << std::endl;
	}
	outfile.open(TIER4FILE, std::ios_base::app);
	// Cycle through the contours
	for (auto vec : contours) {
		// Greater than one includes lunar ellipse
		if (vec.size() > 1) {
			minEnclosingCircle(vec, center, radius);
			// Open the outfile to append
			outfile
			<< cnt
			<< ","
			<< center.x
			<< ","
			<< center.y
			<< ","
			<< radius
			<< std::endl;
			
		}
	}
	outfile.close();
// 	if (DEBUG_COUT) {
// 		imwrite("./tier3.png", frame);
// 	}
	return 0;
}


int parse_checklist(std::string name, std::string value) {
	// Boolean cases
	if (name == "DEBUG_COUT"
		|| name == "DEBUG_FRAMES"
		|| name == "OUTPUT_FRAMES"
		) {
		// Define booleans
		bool result;
		if (value == "true" || value == "True" || value == "TRUE") {
			result = true;
		} else if (value == "false" || value == "False" || value == "FALSE") {
			result = false;
		} else {
			std::cout << "Invalid boolean value in settings.cfg item: " << name << std::endl;
			return 1;
		}
		
		if (name == "DEBUG_COUT") {
			DEBUG_COUT = result;
		} else if (name == "DEBUG_FRAMES") {
			DEBUG_FRAMES = result;
		} else if (name == "OUTPUT_FRAMES") {
			OUTPUT_FRAMES = result;
		}
	}
	// Int cases
	else if (
		name == "EDGETHRESH"
		|| name == "T1_AT_BLOCKSIZE"
		|| name == "T2_AT_BLOCKSIZE"
		|| name == "T3_LAP_KERNEL"
		|| name == "T3_GB_KERNEL_X"
		|| name == "T3_GB_KERNEL_Y"
		|| name == "T3_CUTOFF_THRESH"
		|| name == "T3_DYNMASK_WIDTH"
		|| name == "T4_AT_BLOCKSIZE"
		|| name == "T4_GB_KERNEL_X"
		|| name == "T4_GB_KERNEL_Y"
		|| name == "T4_THINNING"
		|| name == "T4_DYNMASK_WIDTH"
		) {
		int result = std::stoi(value);
		if (name == "EDGETHRESH") {
			EDGETHRESH = result;
		}else if (name == "T1_AT_BLOCKSIZE") {
			T1_AT_BLOCKSIZE = result;
		} else if (name == "T2_AT_BLOCKSIZE") {
			T2_AT_BLOCKSIZE = result;
		} else if (name == "T3_LAP_KERNEL") {
			T3_LAP_KERNEL = result;
		} else if (name == "T3_GB_KERNEL_X") {
			T3_GB_KERNEL_X = result;
		} else if (name == "T3_GB_KERNEL_Y") {
			T3_GB_KERNEL_Y = result;
		} else if (name == "T3_CUTOFF_THRESH") {
			T3_CUTOFF_THRESH = result;
		} else if (name == "T3_DYNMASK_WIDTH") {
			T3_DYNMASK_WIDTH = result;
		} else if (name == "T4_AT_BLOCKSIZE") {
			T4_AT_BLOCKSIZE = result;
		} else if (name == "T4_GB_KERNEL_X") {
			T4_GB_KERNEL_X = result;
		} else if (name == "T4_GB_KERNEL_Y") {
			T4_GB_KERNEL_Y = result;
		} else if (name == "T4_THINNING") {
			T4_THINNING = result;
		} else if (name == "T4_DYNMASK_WIDTH") {
			T4_DYNMASK_WIDTH = result;
		}
		
	}
	// Double cases
	else if (
		name == "T1_AT_MAX"
		|| name == "T1_AT_CONSTANT"
		|| name == "T2_AT_MAX"
		|| name == "T2_AT_CONSTANT"
		|| name == "T3_LAP_SCALE"
		|| name == "T3_LAP_DELTA"
		|| name == "T3_GB_SIGMA_X"
		|| name == "T3_GB_SIGMA_Y"
		|| name == "T4_AT_MAX"
		|| name == "T4_AT_CONSTANT"
		|| name == "T4_POWER"
		|| name == "T4_GB_SIGMA_X"
		|| name == "T4_GB_SIGMA_Y"
		) {
		double result = std::stod(value);
		if (name == "T1_AT_MAX") {
			T1_AT_MAX = result;
		} else if (name == "T1_AT_CONSTANT") {
			T1_AT_CONSTANT = result;
		} else if (name == "T2_AT_MAX") {
			T2_AT_MAX = result;
		} else if (name == "T2_AT_CONSTANT") {
			T2_AT_CONSTANT = result;
		} else if (name == "T3_LAP_SCALE") {
			T3_LAP_SCALE = result;
		} else if (name == "T3_LAP_DELTA") {
			T3_LAP_DELTA = result;
		} else if (name == "T3_GB_SIGMA_X") {
			T3_GB_SIGMA_X = result;
		} else if (name == "T3_GB_SIGMA_Y") {
			T3_GB_SIGMA_Y = result;
		} else if (name == "T4_AT_MAX") {
			T4_AT_MAX = result;
		} else if (name == "T4_AT_CONSTANT") {
			T4_AT_CONSTANT = result;
		} else if (name == "T4_POWER") {
			T4_POWER = result;
		} else if (name == "T4_GB_SIGMA_X") {
			T4_GB_SIGMA_X = result;
		} else if (name == "T4_GB_SIGMA_Y") {
			T4_GB_SIGMA_Y = result;
		}
	} else
		// String cases
		if (name == "OSFPROJECT"
		|| name == "OUTPUTDIR"
		) {
			if (name == "OSFPROJECT") {
				OSFPROJECT = value;
			} else if (name == "OUTPUTDIR") {
				OUTPUTDIR = value;
			}
	} else {
		std::cerr << "Did not recognize entry " << name << " in config file, skipping" << std::endl;
	}
	return 0;
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
	std::string osf_file = "";
	std::string input_file = "";
	std::string config_file = "settings.cfg";
	
	for (int i = 1; i < argc; ++i) {
		string arg = argv[i];
		if ((arg == "-h") || (arg == "--help")) {
			show_usage(argv[0]);
			
			return 0;
		} else if ((arg == "-c") || (arg == "--config-file")) {
			if (i + 1 < argc) {
				config_file = argv[++i];
			} else {
				std::cerr << "--config-file option requires one argument" << std::endl;
				return 1;
			}
		} else if ((arg == "-osf") || (arg == "--osf-path")) {
			// Make sure we aren't at the end of argv!
			if (i + 1 < argc) {
				// Increment 'i' so we don't get the argument as the next argv[i].
				osf_file = argv[++i];
			} else {
				std::cerr << "--osf-path option requires one argument." << std::endl;
				return 1;
			}
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
	
	// Check that we did not include both an OSF and manual input file.
	if (osf_file.length() > 0 && input_file.length() > 0) {
		std::cerr << "Either use an input file or an OSF file, not both" << std::endl;
		return 1;
	} else if (osf_file.length() == 0 && input_file.length() == 0) {
		std::cerr << "You must give frame_extraction an input file or an OSF path" << std::endl;
		return 1;
	} else if (osf_file.length() > 0) {
		if (!(osf_file.substr(0, 9) == "osfstorage" && osf_file.substr(osf_file.size() - 3) == "mp4")) {
			std::cerr << "The path to the OSF video must look like \"osfstorage/path/to/file.mp4\"" << std::endl;
			return 1;
		}
		int ret = system("osf --version");
		if (WEXITSTATUS(ret) != 0x10) {
			std::cerr << "OSFClient is not available on this system. Install using \"pip3 install osfclient --user\"" << std::endl;
		}
		std::string osfcommand = "osf -p ";
		osfcommand.append(OSFPROJECT);
		osfcommand.append(" fetch ");
		osfcommand.append(osf_file);
		osfcommand.append(" ./local.mp4");
		system(osfcommand.c_str());
		input_file = "local.mp4";
	}
	
	// Make sure file exists
	if (!std::experimental::filesystem::exists(input_file)) {
		std::cerr << "Input file (" << input_file << ") not found.  Aborting." << std::endl;
		return 1;
	}
	
	// H264 -> MP4 --------------------------------------------------------------------------------
	// TODO add detection and conversion using MP4Box
	// Note, this method only works with linux afaik
	
	
	// Hack method to kick out h264 files
	auto delimiter_pos = input_file.find('.');
	if (input_file.substr(delimiter_pos + 1) != "mp4") {
		std::cerr << "Input file (" << input_file
		<< ") does not end with \"mp4\".  Assuming the input file is incompatible" << std::endl;
		return 1;
	}
	
	
	// Config Handler -----------------------------------------------------------------------------
	
    std::ifstream cFile (config_file);
	if (cFile.is_open()) {
		std::string line;
		while(getline(cFile, line)){
			line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
			if(line[0] == '#' || line.empty()) {
				continue;
			}
			delimiter_pos = line.find("=");
			std::string name = line.substr(0, delimiter_pos);
			std::string value = line.substr(delimiter_pos + 1);
			if (parse_checklist(name, value)) {
				return 1;
			}
		}
		
	}
	else {
		std::cerr << "Couldn't open config file for reading." << std::endl;
		return 1;
	}

	if (DEBUG_COUT) {
		std::cout << "Config File Loaded from" << config_file << std::endl;
		std::cout << "Using input file: " << input_file << std::endl;
	}
	
	
	// Touch the output file ----------------------------------------------------------------------
	
	// Create directories
	std::string localpath;
	fs::current_path(fs::temp_directory_path());
	localpath = OUTPUTDIR.append("data");
	fs::create_directories(localpath);
// 	fs::permissions(localpath, fs::perms::others_all, fs::perm_options::remove);
	if (OUTPUT_FRAMES) {
		localpath = OUTPUTDIR.append("frames");
		fs::create_directories(localpath);
// 		fs::permissions(localpath, fs::perms::others_all, fs::perm_options::remove);
	}
	
	// Synthesize Filenames
	TIER1FILE = OUTPUTDIR.append("data/Tier1.csv");
	TIER2FILE = OUTPUTDIR.append("data/Tier2.csv");
	TIER3FILE = OUTPUTDIR.append("data/Tier3.csv");
	TIER4FILE = OUTPUTDIR.append("data/Tier4.csv");
	ELLIPSEDATA  = OUTPUTDIR.append("data/ellipses.csv");
	METADATA = OUTPUTDIR.append("metadata.csv");
	
	std::ofstream outfile;
	outfile.open(TIER1FILE);
	outfile.close();
	outfile.open(TIER2FILE);
	outfile.close();
	outfile.open(TIER3FILE);
	outfile.close();
	outfile.open(TIER4FILE);
	outfile.close();
	
	// Touch output ellipse file
	std::ofstream outell;
	outell.open(ELLIPSEDATA);
	outell.close();
	
	// Touch and create metafile
	std::ofstream metafile;
	metafile.open(METADATA);
	if (!osf_file.empty()) {
		metafile << "video:," << osf_file << std::endl;
	} else {
		metafile << "video:," << input_file << std::endl;
	}
	metafile.close();
	
	// Instance and Assign ------------------------------------------------------------------------
	// Memory map variables we want to share across forks
	// mm_gotframe reports that the frame acquisition was completed
	// mm_gotconts reports that video processing was completed
	// mm_localfrm reports that the current frame was grabbed, proceed getting next frame
	// mm_ranprocs reports that the contours were recorded
	// mm_frmcount stores the frame counter
	auto *mm_killed = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_frameavail = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_gotframe1 = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_gotframe2 = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_gotframe3 = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_tier1 = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_tier2 = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_tier3 = static_cast <bool *>(mmap(NULL, sizeof(bool), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	auto *mm_frmcount = static_cast <int *>(mmap(NULL, sizeof(int), \
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	*mm_frmcount = -1;
// 	auto *mm_vec1 = static_cast <vector <vector float>>(mmap(NULL, 524288000, \
 		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0));
	
	// OpenCV specific variables
	Mat frame;
	
	// Memory and fork management inits
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
	cvtColor(frame.clone(), frame, COLOR_BGR2GRAY);
	frame = first_frame(frame.clone(), *mm_frmcount);
// 	frame = halo_noise_and_center(frame.clone(), *mm_frmcount);
	++*mm_frmcount;
	
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
	memcpy(buf, frame.ptr(), shmem_size);
	if (DEBUG_COUT) {
		std::cout << "memcpy'd buf1 with size:" << sizeof(*buf) << std::endl;
	}
	
	unsigned char *buf2 = static_cast <unsigned char*>(mmap(NULL, shmem_size, PROT_READ|PROT_WRITE, MAP_SHARED, frame_fd2, 0));
	memcpy(buf2, frame.ptr(), shmem_size);
	if (DEBUG_COUT) {
		std::cout << "memcpy'd buf2 with size:"  << sizeof(*buf2) << std::endl;
	}
	
	// Store first frame in memory
	memcpy(buf, frame.ptr(), shmem_size);
	*mm_frameavail = true;
	
	// Delared PID here so we can stash the fork in an if statement.
	int pid0;
	int pid1;
	int pid2;
	
	// Set all the mm_ codes
	*mm_killed = false;
	*mm_frameavail = true;
// 	*mm_gotframe1 = false;
// 	*mm_gotframe2 = false;
// 	*mm_gotframe3 = false;
	*mm_tier1 = true;
	*mm_tier2 = true;
	*mm_tier3 = true;
	
// 	// Thresh detect ------------------------------------------------------------------------------
// 	// Determine the optimal threshold from the first frame
// 	// AAAAH WHY CAN'T I KILL THIS THING!!!
// 	thresh = thresh_detect(frame);
	
	// Main Loop ----------------------------------------------------------------------------------
	// If we have not told the program to exit, fork and continue.
// 	if (SIG_ALERT == 0) {
// 		
// 	} else {
// 		break;
// 	}

		
	pid0 = fork();
	
	if (pid0 > 0) {
		// FORK 0 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 			// BEGIN get first frame on start
// 			*mm_frameavail = false;
// 			++*mm_frmcount;
// 			if (DEBUG_COUT) {
// 				std::cout << "frame number: " << *mm_frmcount << std::endl;
// 			}
// 			
// 			// Image processing operations
// // 			cvtColor(frame, frame, COLOR_BGR2GRAY);
// 			frame = halo_noise_and_center(frame.clone(), *mm_frmcount);
// 			// Store this image into the memory buffer
// 			printf("1\n");
// 			memcpy(buf, frame.ptr(), shmem_size);
// 			printf("2\n");
// 			*mm_frameavail = true;
// 			// END get first frame on start
		while (cap.isOpened()) {
			// Check for ctrl C
			if (SIG_ALERT != 0) {
				*mm_killed = true;
				cap.release();
				break;
			}
			while ((*mm_tier1 == false) || (*mm_tier2 == false) || (*mm_tier3 == false)) {
				usleep(50);
			}
			*mm_frameavail = false;
			*mm_tier1 = false;
			*mm_tier2 = false;
			*mm_tier3 = false;
			
			
			// Cycle ring buffer
			memcpy(buf2, buf, shmem_size);
			// Get new frame; operate and store
			cap >> frame;
			++*mm_frmcount;
			// Image processing operations
			cvtColor(frame, frame, COLOR_BGR2GRAY);
			frame = halo_noise_and_center(frame.clone(), *mm_frmcount);
			
			if (DEBUG_COUT) {
				std::cout
				<< "frame number: "
				<< *mm_frmcount
				<< std::endl
				<< "frame dimensions: "
				<< frame.size().width
				<< " x "
				<< frame.size().height
				<< " x "
				<< frame.elemSize()
				<< " = "
				<< shmem_size
				<< std::endl;
			}
			// Store this image into the memory buffer
			memcpy(buf, frame.ptr(), shmem_size);
			
			// Report that the frame was stored
			*mm_frameavail = true;
// 			while ((*mm_tier1 == false) || (*mm_tier2 == false) || (*mm_tier3 == false)) {
// 				usleep(50);
// 			}
// 			wait(0);
// 				wait(0);
			
			if (DEBUG_FRAMES) {
				imshow("fg_mask", frame);
				waitKey(1);
			}
		}
		*mm_killed = true;
	} else {
		pid1 = fork();
		if (pid1 > 0) {
			// FORK 1 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			while (*mm_killed == false) {
				while ((*mm_frameavail == false) || (*mm_tier1 == true) || (*mm_tier2 == true) || (*mm_tier3 == true)) {
					usleep(50);
				}
				// Grab the frame and store it locally
				Mat local_frame = cv::Mat(Size(frame.size().width, frame.size().height), CV_8UC1, buf, 1 * frame.size().width);
				Mat local_frame_1 = local_frame.clone();
				Mat local_frame_2 = local_frame.clone();
				int local_count = *mm_frmcount;
				tier_one(local_count, local_frame_1.clone());
				tier_two(local_count, local_frame_2.clone());
				*mm_tier1 = true;
				*mm_tier2 = true;
			}
		} else {
			// FORK 2 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			while (*mm_killed == false) {
				while ((*mm_frameavail == false) || (*mm_tier1 == true) || (*mm_tier2 == true) || (*mm_tier3 == true))  {
					usleep(50);
				}
				int local_count = *mm_frmcount;
				if (local_count == 0) {
					// Skip the zeroth frame since there will be no old_frame
					if (DEBUG_COUT) {
						std::cout << "skipping early frame tier3 in frame: " << local_count << std::endl;
					}
				} else {
					
					// Grab the previous frame + current frame and store them locally
					Mat local_frame = cv::Mat(Size(frame.size().width, frame.size().height), CV_8UC1, buf, 1 * frame.size().width);
					Mat oldframe = cv::Mat(Size(frame.size().width, frame.size().height), CV_8UC1, buf2, 1 * frame.size().width);
					tier_three(local_count, local_frame.clone(), oldframe.clone());
					tier_four(local_count, local_frame.clone(), oldframe.clone());
					*mm_tier3 = true;
				}
			}
			
		}
	}
		
 	usleep(1000000);
	exit(SIG_ALERT);
 	return 0;
}
