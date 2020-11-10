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
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING << "SHIFT_X: " << shiftx << std::endl << "SHIFT_Y: " << shifty << std::endl;
		LOGGING.close();
	}
	
	if ((shiftx < 0) && (shifty < 0)) { // move right and down
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING << "shifting case 1 - move right and down" << std::endl;
			LOGGING.close();
		}
		in_frame(Rect(0, 0, BOXSIZE-abs(shiftx), BOXSIZE-abs(shifty)))
		.copyTo(zero_mask(Rect(abs(shiftx), abs(shifty), BOXSIZE-abs(shiftx), BOXSIZE-abs(shifty))));
	} else if (shiftx < 0) { // move right and maybe up
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING << "shifting case 2 - move right and maybe up" << std::endl;
			LOGGING.close();
		}
		in_frame(Rect(0, abs(shifty), BOXSIZE-abs(shiftx), BOXSIZE-abs(shifty)))
		.copyTo(zero_mask(Rect(abs(shiftx), 0, BOXSIZE-abs(shiftx), BOXSIZE-abs(shifty))));
	} else if (shifty < 0) { // move down and maybe left
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING << "shifting case 3 - move down and maybe left" << std::endl;
			LOGGING.close();
		}
		in_frame(Rect(abs(shiftx), 0, BOXSIZE-abs(shiftx), BOXSIZE-abs(shifty)))
		.copyTo(zero_mask(Rect(0, abs(shifty), BOXSIZE-abs(shiftx), BOXSIZE-abs(shifty))));
	} else {  // positive moves up and left
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING << "shifting case 4 - move up and left" << std::endl;
			LOGGING.close();
		}
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
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "PLUSX: " << plusx << " PLUSY: " << plusy << std::endl
		<< "LOCAL_TL = (" << local_tl.x << ", " << local_tl.y << ")" << std::endl
		<< "LOCAL_BR = (" << local_br.x << ", " << local_br.y << ")" << std::endl
		<< "ORIG_TL = (" << ORIG_TL.x << ", " << ORIG_TL.y << ")" << std::endl
		<< "ORIG_BR = (" << ORIG_BR.x << ", " << ORIG_BR.y << ")" << std::endl;
		LOGGING.close();
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
	
	in_frame = shift_frame(in_frame, shiftx/2, -shifty/2);
	
	
	return in_frame;
}

static vector <int> test_edges(Mat in_frame, vector<Point> contour) {
	Moments M = moments(contour);
	Point cen(int(M.m10/M.m00), int(M.m01/M.m00));
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING << "centroid: " << cen << std::endl;
		LOGGING.close();
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
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< to_top
		<< ","
		<< to_right
		<< ","
		<< to_bottom
		<< ","
		<< to_left
		<< std::endl;
		LOGGING.close();
	}
	
	int plusx = 0;
	int plusy = 0;
	if (to_top != to_bottom) {
		plusy = to_top - to_bottom;
	}
	if (to_left != to_right) {
		plusx = to_left - to_right;
	}
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING << plusx << ", " << plusy << std::endl;
		LOGGING.close();
	}
	
	vector <int> outplus;
	outplus.push_back(plusx);
	outplus.push_back(plusy);
	return outplus;
}

static int min_square_dim(Mat in_frame) {
	BOXSIZE = min(in_frame.rows, in_frame.cols);
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING << "Cutting frame to size: (" << BOXSIZE << ", " << BOXSIZE << ")" << std::endl;
		LOGGING.close();
	}
	return 0;
}

static vector <int> edge_width(vector<Point> contour) {
	vector <int> local_vec;
	
	Rect box = boundingRect(contour);
	int top = box.tl().y;
	int bot = box.br().y;
	
	int topout = 0;
	int botout = 0;
	int cnt = 0;
	for (size_t i = 0; i<contour.size(); i++) {
		if (contour[i].y == top) {
			topout += 1;
		}
		if (contour[i].y == bot) {
			botout += 1;
		}
	}
	
	local_vec.push_back(topout);
	local_vec.push_back(botout);
	
	return local_vec;
}

static vector <int> edge_height(vector<Point> contour) {
	vector <int> local_vec;
	
	Rect box = boundingRect(contour);
	int lef = box.tl().x;
	int rig = box.br().x;
	
	int lefout = 0;
	int rigout = 0;
	int cnt = 0;
	for (size_t i = 0; i<contour.size(); i++) {
		if (contour[i].y == lef) {
			lefout += 1;
		}
		if (contour[i].y == rig) {
			rigout += 1;
		}
	}
	
	local_vec.push_back(lefout);
	local_vec.push_back(rigout);
	
	return local_vec;
}

static Mat first_frame(Mat in_frame, int framecnt) {
	min_square_dim(in_frame);
	
	vector <vector<Point>> contours = contours_only(in_frame);
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "Number of detected contours in ellipse frame: "
		<< contours.size()
		<< std::endl;
		LOGGING.close();
	}
	
	int largest_contour_index = largest_contour(contours);
	
	// Store original area and perimeter of first frame
	Rect box = boundingRect(contours[largest_contour_index]);
	ORIG_AREA = box.area();
	ORIG_PERI = arcLength(contours[largest_contour_index], true);
	ORIG_VERT = box.height;
	ORIG_HORZ = box.width;
	
	ORIG_TL = box.tl();
	ORIG_BR = box.br();
	
	
// 	// Fit the largest contour to an ellipse
// 	RotatedRect box = fitEllipse(contours[largest_contour_index]);
	
// 	// Use minimum enclosing circle to get max diameter of moon ellipse
// 	Point2f center;
// 	minEnclosingCircle(contours[largest_contour_index], center, ELL_RAD);
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "box width: " << box.width << std::endl
		<< "box height: " << box.height << std::endl
		<< "box x: " << box.x << std::endl
		<< "box y: " << box.y << std::endl
		<< "box angle: " << box.area() << std::endl;
		LOGGING.close();
	}
	
	framecnt = framecnt + 1;
	
// 	// Select filled area with moon
// 	ellipse(mask_mat, box, 255, -1, LINE_AA);
	
	// Create rect representing the image
	Rect image_rect = Rect({}, in_frame.size());
// 	Rect roi = Rect(box.x-(BOXSIZE/2), box.y-(BOXSIZE/2), BOXSIZE, BOXSIZE);
	Point roi_tl = Point(box.tl().x - ((BOXSIZE - box.width)/2), (box.tl().y- (BOXSIZE - box.height)/2));
	Point roi_br = Point(((BOXSIZE - box.width)/2) + box.br().x, ((BOXSIZE - box.height)/2) + box.br().y);
// 	Rect roi = Rect(box.tl(), box.br());
	Rect roi = Rect(roi_tl, roi_br);

	// Find intersection, i.e. valid crop region
	Rect intersection = image_rect & roi;

	// Move intersection to the result coordinate space
	Rect inter_roi = intersection - roi.tl();

	// Create black image and copy intersection
	Mat crop = Mat::zeros(roi.size(), in_frame.type());
	in_frame(intersection).copyTo(crop(inter_roi));
	
	in_frame = crop;
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "ORIG_AREA = "
		<< ORIG_AREA
		<< std::endl
		<< "ORIG_PERI = "
		<< ORIG_PERI
		<< std::endl;
		LOGGING.close();
	}
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "ORIG_TL = "
		<< ORIG_TL
		<< std::endl
		<< "ORIG_BR = "
		<< ORIG_BR
		<< std::endl;
		LOGGING.close();
	}
	
	
	vector <int> outplus = test_edges(in_frame, contours[largest_contour_index]);
	
	int edge_top = 0;
	int edge_bot = 0;
	int edge_lef = 0;
	int edge_rig = 0;
	
	if ((abs(outplus[0]) > EDGETHRESH) || (abs(outplus[1]) > EDGETHRESH)) {
		if ((outplus[0] > 0) || (outplus[0] < 0)) {
			vector <int> local_edge = edge_width(contours[largest_contour_index]);
			edge_top = local_edge[0];
			edge_bot = local_edge[1];
		}
		if ((outplus[1] > 0) || (outplus[1] < 0)) {
			vector <int> local_edge = edge_height(contours[largest_contour_index]);
			edge_lef = local_edge[0];
			edge_rig = local_edge[1];
		}
	}
	
	// Open the outfile to append list of major ellipses
	std::ofstream outell;
	outell.open(ELLIPSEDATA, std::ios_base::app);
	outell
	<< framecnt
	<< ","
	<< box.x
	<< ","
	<< box.y
	<< ","
	<< box.width
	<< ","
	<< box.height
	<< ","
	<< box.area()
	<< ","
	<< edge_top
	<< ","
	<< edge_bot
	<< ","
	<< edge_lef
	<< ","
	<< edge_rig
	<< std::endl;
	outell.close();
	
	return in_frame;
}

static Mat halo_noise_and_center(Mat in_frame, int framecnt) {
	// Find largest ellipse
	Rect box = box_finder(in_frame);
	// HACK increase the framecnt here
// 	framecnt = framecnt + 1;
	
	// Create rect representing the image
	Rect image_rect = Rect({}, in_frame.size());
// 	Rect roi  = Rect(box.x-(BOXSIZE/2), box.y-(BOXSIZE/2), BOXSIZE, BOXSIZE);
	Point roi_tl = Point(box.tl().x - ((BOXSIZE - box.width)/2), (box.tl().y- (BOXSIZE - box.height)/2));
	Point roi_br = Point(((BOXSIZE - box.width)/2) + box.br().x, ((BOXSIZE - box.height)/2) + box.br().y);
	Rect roi = Rect(roi_tl, roi_br);

	// Find intersection, i.e. valid crop region
	Rect intersection = image_rect & roi;

	// Move intersection to the result coordinate space
	Rect inter_roi = intersection - roi.tl();

	// Create black image and copy intersection
	Mat crop = Mat::zeros(Size(BOXSIZE, BOXSIZE), in_frame.type());
	in_frame(intersection).copyTo(crop(inter_roi));
	in_frame = crop;
	
	
	
	vector <vector<Point>> contours = contours_only(in_frame);
	int largest = largest_contour(contours);
	vector <int> outplus = test_edges(in_frame, contours[largest]);
	
	int edge_top = 0;
	int edge_bot = 0;
	int edge_lef = 0;
	int edge_rig = 0;
	
	if ((abs(outplus[0]) > EDGETHRESH) || (abs(outplus[1]) > EDGETHRESH)) {
		
		if (DEBUG_COUT) {
			LOGGING.open(LOGOUT, std::ios_base::app);
			LOGGING << "Activating Corner Matching" << std::endl;
			LOGGING.close();
		}
		in_frame = corner_matching(in_frame, contours[largest], outplus[0], outplus[1]);
		if ((outplus[0] > 0) || (outplus[0] < 0)) {
			vector <int> local_edge = edge_width(contours[largest]);
			edge_top = local_edge[0];
			edge_bot = local_edge[1];
		}
		if ((outplus[1] > 0) || (outplus[1] < 0)) {
			vector <int> local_edge = edge_height(contours[largest]);
			edge_lef = local_edge[0];
			edge_rig = local_edge[1];
		}
	}
	
	// Open the outfile to append list of major ellipses
	std::ofstream outell;
	outell.open(ELLIPSEDATA, std::ios_base::app);
	outell
	<< framecnt
	<< ","
	<< box.x
	<< ","
	<< box.y
	<< ","
	<< box.width
	<< ","
	<< box.height
	<< ","
	<< box.area()
	<< ","
	<< edge_top
	<< ","
	<< edge_bot
	<< ","
	<< edge_lef
	<< ","
	<< edge_rig
	<< std::endl;
	outell.close();
	
	
	return in_frame;
}

void signal_callback_handler(int signum) {
	if (signum == 2) {
		std::cerr << "Caught ctrl+c interrupt signal: " << std::endl;
	}
	// Terminate program
	SIG_ALERT = signum;
	return;
}

static vector <vector<Point>> fetch_dynamic_mask(Mat in_frame) {
	vector <vector<Point>> output;
	vector <vector<Point>> contours = contours_only(in_frame);
	int index = largest_contour(contours);
	if (index < 0) {
		vector<Point> aaa;
		aaa.push_back(Point(-1, -1));
		output.push_back(aaa);
		return output;
	}
	vector<Point> maxcont = contours[index];
	output.push_back(maxcont);
	return output;
}

static Mat apply_dynamic_mask(Mat in_frame, vector <vector<Point>> contours, int maskwidth) {
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

static Rect box_finder(Mat in_frame) {
	
	vector <vector<Point>> contours = contours_only(in_frame);
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "Number of detected contours in ellipse frame: "
		<< contours.size()
		<< std::endl;
		LOGGING.close();
	}
	
	int largest_contour_index = largest_contour(contours);
	
	// Find the bounding box for the large contour
	Rect box = boundingRect(contours[largest_contour_index]);

	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "box tr: " << box.tl() << std::endl
		<< "box bl: " << box.br() << std::endl
		<< "box x: " << box.x << std::endl
		<< "box y: " << box.y << std::endl
		<< "box width: " << box.width << std::endl
		<< "box height: " << box.height << std::endl
		<< "box area: " << box.area() << std::endl;
		LOGGING.close();
	}
	
	
	
	return box;
}

static void show_usage(string name) {
	std::cerr << "Usage: "  << " <option(s)> \t\tSOURCES\tDescription\n"
			<< "Options:\n"
			<< "\t-h,--help\t\t\tShow this help message\n"
			<< "\t-v,--version\t\t\tPrint version info to STDOUT\n"
			<< "\t-i,--input\t\tINPUT\tSpecify path to input video\n"
			<< "\t-c,--config-file \tINPUT\tSpecify config file (default settings.cfg)\n"
			<< "\t-osf,--osf-path \tINPUT\tSpeify path to osf video\n"
			<< std::endl;
}

static std::tuple <float, int> laplace_sum(vector<Point> contour, Mat lapframe) {
	float avg;
	int outval = 0;
	int cnt = 0;
	for (size_t i = 0; i<contour.size(); i++) {
		outval +=(int16_t)lapframe.at<int16_t>(contour[i].y, contour[i].x);
		cnt++;
	}
	avg = outval/cnt;
	return {avg, cnt};
}

static vector <vector<Point>> quiet_halo_elim(vector <vector<Point>> contours, int tier) {
	int largest_contour_index = largest_contour(contours);
	if (largest_contour_index < 0) {
		return contours;
	}
	vector <Point> bigone = contours[largest_contour_index];
	
	float distance;
	vector <vector<Point>> out_contours;
	
	bool caught_mask = false;
	
	for (size_t i = 0; i < contours.size(); i++) {
		caught_mask = false;
		// Skip the big one
		if (i == largest_contour_index) {
			continue;
		}
		Moments M = moments(contours[i]);
		int x_cen = (M.m10/M.m00);
		int y_cen = (M.m01/M.m00);
		if ((x_cen < 0) || (y_cen < 0)) {
			x_cen = contours[i][0].x;
			y_cen = contours[i][0].y;
		}
		for (size_t j = 0; j < bigone.size(); j++) {
			distance = sqrt(pow((x_cen - bigone[j].x), 2) + pow((y_cen - bigone[j].y), 2));
			if (distance < QHE_WIDTH) {
				caught_mask = true;
				break;
			}
		}
		if (!caught_mask) {
			out_contours.push_back(contours[i]);
		}
	}
	return out_contours;
}

int tier_one(int cnt, Mat frame) {
	Point2f center;
	float radius;
	float bigradius = 0;
	std::ofstream outfile;
	vector <vector<Point>> dymask = fetch_dynamic_mask(frame);
	adaptiveThreshold(frame.clone(), frame, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 65, 35);
	if ((dymask[0][0].x != -1) && (dymask[0][0].y != -1)) {
		frame = apply_dynamic_mask(frame.clone(), dymask, T1_DYMASK);
	}
	vector <vector<Point>> contours = contours_only(frame);
	contours = quiet_halo_elim(contours, 1);
// 	cnt = cnt + 1;
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "Number of contours in tier 1 pass for frame "
		<< cnt
		<< ": "
		<< contours.size()
		<< std::endl;
		LOGGING.close();
	}
	int largest_contour_index = largest_contour(contours);
	if (largest_contour_index > -1) {
		minEnclosingCircle(contours[largest_contour_index], center, bigradius);
	}
	outfile.open(TIER1FILE, std::ios_base::app);
	// Cycle through the contours
	for (auto vec : contours) {
		// Greater than one includes lunar ellipse
		if (vec.size() > 1) {
			minEnclosingCircle(vec, center, radius);
			if (radius != bigradius) {
				// Open the outfile to append
				outfile
				<< cnt
				<< ","
				<< static_cast<int>(center.x)
				<< ","
				<< static_cast<int>(center.y)
				<< ","
				<< radius
				<< std::endl;
			}
		}
	}
	outfile.close();
	return 0;
}

int tier_two(int cnt, Mat frame) {
	Point2f center;
	float radius;
	float bigradius = 0;
	std::ofstream outfile;
	vector <vector<Point>> dymask = fetch_dynamic_mask(frame);
	adaptiveThreshold(frame.clone(), frame, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 65, 20);
	if ((dymask[0][0].x != -1) && (dymask[0][0].y != -1)) {
		frame = apply_dynamic_mask(frame.clone(), dymask, T2_DYMASK);
	}
	vector <vector<Point>> contours = contours_only(frame);
	contours = quiet_halo_elim(contours, 2);
// 	cnt = cnt + 1;
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "Number of contours in tier 2 pass for frame "
		<< cnt
		<< ": "
		<< contours.size()
		<< std::endl;
		LOGGING.close();
	}
	int largest_contour_index = largest_contour(contours);
	if (largest_contour_index > -1) {
		minEnclosingCircle(contours[largest_contour_index], center, bigradius);
	}
	outfile.open(TIER2FILE, std::ios_base::app);
	// Cycle through the contours
	for (auto vec : contours) {
		// Greater than one includes lunar ellipse
		if (vec.size() > 1) {
			minEnclosingCircle(vec, center, radius);
			if (radius != bigradius) {
				// Open the outfile to append
				outfile
				<< cnt
				<< ","
				<< static_cast<int>(center.x)
				<< ","
				<< static_cast<int>(center.y)
				<< ","
				<< radius
				<< std::endl;
			}
		}
	}
	outfile.close();
	return 0;
}

int tier_three(int cnt, Mat frame, Mat oldframe) {
	Point2f center;
	float radius;
	float bigradius = 0;
	std::ofstream outfile;
	Mat scaleframe;
	vector <vector<Point>> dymask = fetch_dynamic_mask(frame);
// 	cnt = cnt + 1;
	
	/* Eli Method for Tier 3 */
	Laplacian(frame.clone(), frame, CV_32F, 11, 0.0001, 0, BORDER_DEFAULT);
	Laplacian(oldframe.clone(), oldframe, CV_32F, 11, 0.0001, 0, BORDER_DEFAULT);
	GaussianBlur(frame.clone(), frame, Size(11, 11), 1, 1, BORDER_DEFAULT);
	GaussianBlur(oldframe.clone(), oldframe, Size(11, 11), 1, 1, BORDER_DEFAULT);
	scaleframe = frame.clone() - oldframe.clone();
	scaleframe = scaleframe.clone() > 40;
	/* end Eli Method */
	if ((dymask[0][0].x != -1) && (dymask[0][0].y != -1)) {
		scaleframe = apply_dynamic_mask(scaleframe.clone(), dymask, T3_DYMASK);
	}
	vector <vector<Point>> contours = contours_only(scaleframe);
	contours = quiet_halo_elim(contours, 3);
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "Number of contours in tier 3 pass for frame "
		<< cnt
		<< ": "
		<< contours.size()
		<< std::endl;
		LOGGING.close();
	}
	int largest_contour_index = largest_contour(contours);
	if (largest_contour_index > -1) {
		minEnclosingCircle(contours[largest_contour_index], center, bigradius);
	}
	outfile.open(TIER3FILE, std::ios_base::app);
	// Cycle through the contours
	for (auto vec : contours) {
		// Greater than one includes lunar ellipse
		if (vec.size() > 1) {
			minEnclosingCircle(vec, center, radius);
			if (radius != bigradius) {
				// Open the outfile to append
				outfile
				<< cnt
				<< ","
				<< static_cast<int>(center.x)
				<< ","
				<< static_cast<int>(center.y)
				<< ","
				<< radius
				<< std::endl;
			}
		}
	}
	outfile.close();
}

int tier_four(int cnt, Mat frame, Mat oldframe) {
	Point2f center;
	float radius;
	float bigradius = 0;
	std::ofstream outfile;
	Mat scaleframe;
	vector <vector<Point>> dymask = fetch_dynamic_mask(frame);
	
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
	
	if ((dymask[0][0].x != -1) && (dymask[0][0].y != -1)) {
		scaleframe = apply_dynamic_mask(scaleframe.clone(), dymask, T4_DYMASK);
	}
// 	imwrite("./tstx.png", frame);
// 	imwrite("./tsty.png", scaleframe);
	vector <vector<Point>> contours = contours_only(scaleframe);
	contours = quiet_halo_elim(contours, 4);
	
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "Number of contours in tier 4 pass for frame "
		<< cnt
		<< ": "
		<< contours.size()
		<< std::endl;
		LOGGING.close();
	}
	int largest_contour_index = largest_contour(contours);
	if (largest_contour_index > -1) {
		minEnclosingCircle(contours[largest_contour_index], center, bigradius);
	}
	outfile.open(TIER4FILE, std::ios_base::app);
	// Cycle through the contours
	for (auto vec : contours) {
		// Greater than one includes lunar ellipse
		if (vec.size() > 1) {
			minEnclosingCircle(vec, center, radius);
			if (radius != bigradius) {
				// Open the outfile to append
				outfile
				<< cnt
				<< ","
				<< static_cast<int>(center.x)
				<< ","
				<< static_cast<int>(center.y)
				<< ","
				<< radius
				<< std::endl;
			}
			
		}
	}
	outfile.close();
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
			std::cerr << "Invalid boolean value in settings.cfg item: " << name << std::endl;
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
		|| name == "QHE_WIDTH"
		|| name == "T1_AT_BLOCKSIZE"
		|| name == "T1_DYMASK"
		|| name == "T2_AT_BLOCKSIZE"
		|| name == "T2_DYMASK"
		|| name == "T3_LAP_KERNEL"
		|| name == "T3_GB_KERNEL_X"
		|| name == "T3_GB_KERNEL_Y"
		|| name == "T3_CUTOFF_THRESH"
		|| name == "T3_DYMASK"
		|| name == "T4_AT_BLOCKSIZE"
		|| name == "T4_GB_KERNEL_X"
		|| name == "T4_GB_KERNEL_Y"
		|| name == "T4_THINNING"
		|| name == "T4_DYMASK"
		) {
		int result = std::stoi(value);
		if (name == "EDGETHRESH") {
			EDGETHRESH = result;
		} else if (name == "QHE_WIDTH") {
			QHE_WIDTH = result;
		} else if (name == "T1_AT_BLOCKSIZE") {
			T1_AT_BLOCKSIZE = result;
		} else if (name == "T1_DYMASK") {
			T1_DYMASK = result;
		} else if (name == "T2_AT_BLOCKSIZE") {
			T2_AT_BLOCKSIZE = result;
		} else if (name == "T2_DYMASK") {
			T2_DYMASK = result;
		} else if (name == "T3_LAP_KERNEL") {
			T3_LAP_KERNEL = result;
		} else if (name == "T3_GB_KERNEL_X") {
			T3_GB_KERNEL_X = result;
		} else if (name == "T3_GB_KERNEL_Y") {
			T3_GB_KERNEL_Y = result;
		} else if (name == "T3_CUTOFF_THRESH") {
			T3_CUTOFF_THRESH = result;
		} else if (name == "T3_DYMASK") {
			T3_DYMASK = result;
		} else if (name == "T4_AT_BLOCKSIZE") {
			T4_AT_BLOCKSIZE = result;
		} else if (name == "T4_GB_KERNEL_X") {
			T4_GB_KERNEL_X = result;
		} else if (name == "T4_GB_KERNEL_Y") {
			T4_GB_KERNEL_Y = result;
		} else if (name == "T4_THINNING") {
			T4_THINNING = result;
		} else if (name == "T4_DYMASK") {
			T4_DYMASK = result;
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
	if (argc < 2) {
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
		} else if ((arg == "-v") || (arg == "--version")) {
			std::cout
			<< "LunAero Frame Extractor"
			<< std::endl
			<< "v" << MAJOR_VERSION << "." << MINOR_VERSION << ALPHA_BETA
			<< std::endl
			<< "Compiled: " << __TIMESTAMP__
			<< std::endl
			<< "Copyright (C) 2020, GPL-3.0"
			<< std::endl
			<< "Wesley T. Honeycutt, Oklahoma Biological Survey"
			<< std::endl;
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
		std::cerr
		<< "Input file ("
		<< input_file
		<< ") does not end with \"mp4\".  Assuming the input file is incompatible"
		<< std::endl;
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
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
		<< "Config File Loaded from" << config_file << std::endl
		<< "Using input file: " << input_file << std::endl;
		LOGGING.close();
	}
	
	
	// Touch the output file ----------------------------------------------------------------------
	
	// Create directories
	std::string localpath;
	localpath = fs::current_path();
	OUTPUTDIR = localpath + OUTPUTDIR;
	localpath = OUTPUTDIR + "data";
	fs::create_directories(localpath);
	std::ofstream outfile;
// 	fs::permissions(localpath, fs::perms::others_all, fs::perm_options::remove);
	if (DEBUG_COUT) {
		LOGOUT = OUTPUTDIR + "data/log.log";
		LOGGING.open(LOGOUT);
		LOGGING.close();
	}
	if (OUTPUT_FRAMES) {
		localpath = OUTPUTDIR + "frames";
		fs::create_directories(localpath);
// 		fs::permissions(localpath, fs::perms::others_all, fs::perm_options::remove);
	}
	
	// Synthesize Filenames
	TIER1FILE = OUTPUTDIR + "data/Tier1.csv";
	TIER2FILE = OUTPUTDIR + "data/Tier2.csv";
	TIER3FILE = OUTPUTDIR + "data/Tier3.csv";
	TIER4FILE = OUTPUTDIR + "data/Tier4.csv";
	ELLIPSEDATA  = OUTPUTDIR + "data/ellipses.csv";
	METADATA = OUTPUTDIR + "data/metadata.csv";
	
	outfile.open(TIER1FILE);
	outfile
	<< "frame number"
	<< ","
	<< "x pos"
	<< ","
	<< "y pos"
	<< ","
	<< "radius"
	<< std::endl;
	outfile.close();
	outfile.open(TIER2FILE);
	outfile
	<< "frame number"
	<< ","
	<< "x pos"
	<< ","
	<< "y pos"
	<< ","
	<< "radius"
	<< std::endl;
	outfile.close();
	outfile.open(TIER3FILE);
	outfile
	<< "frame number"
	<< ","
	<< "x pos"
	<< ","
	<< "y pos"
	<< ","
	<< "radius"
	<< std::endl;
	outfile.close();
	outfile.open(TIER4FILE);
	outfile
	<< "frame number"
	<< ","
	<< "x pos"
	<< ","
	<< "y pos"
	<< ","
	<< "radius"
	<< std::endl;
	outfile.close();
	
	// Touch output ellipse file
	std::ofstream outell;
	outell.open(ELLIPSEDATA);
	outell
	<< "frame number"
	<< ","
	<< "moon center x"
	<< ","
	<< "moon center y"
	<< ","
	<< "moon x diameter"
	<< ","
	<< "moon y diameter"
	<< ","
	<< "moon enclosing box area"
	<< ","
	<< "points on top edge"
	<< ","
	<< "points on bot edge"
	<< ","
	<< "points on left edge"
	<< ","
	<< "points on right edge"
	<< std::endl;
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
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING
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
		LOGGING.close();
	}
	
	// Resize the frame_fd to fit the frame size we are using
	if (ftruncate(frame_fd, shmem_size) != 0) {
		std::cerr << "failed to truncate resize file descriptor" << std::endl;
		return 1;
	}
	// Resize the frame_fd to fit the frame size we are using
	if (ftruncate(frame_fd2, shmem_size) != 0) {
		std::cerr << "failed to truncate resize file descriptor 2" << std::endl;
		return 1;
	}
	
	
	unsigned char *buf = static_cast <unsigned char*>(mmap(NULL, shmem_size, PROT_READ|PROT_WRITE, MAP_SHARED, frame_fd, 0));
	memcpy(buf, frame.ptr(), shmem_size);
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING << "memcpy'd buf1 with size:" << sizeof(*buf) << std::endl;
		LOGGING.close();
	}
	
	unsigned char *buf2 = static_cast <unsigned char*>(mmap(NULL, shmem_size, PROT_READ|PROT_WRITE, MAP_SHARED, frame_fd2, 0));
	memcpy(buf2, frame.ptr(), shmem_size);
	if (DEBUG_COUT) {
		LOGGING.open(LOGOUT, std::ios_base::app);
		LOGGING << "memcpy'd buf2 with size:"  << sizeof(*buf2) << std::endl;
		LOGGING.close();
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
	*mm_tier1 = true;
	*mm_tier2 = true;
	*mm_tier3 = true;
	
	// Main Loop ----------------------------------------------------------------------------------
	pid0 = fork();
	
	if (pid0 > 0) {
		// FORK 0 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
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
			if (frame.empty()) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING << "Reached end of frames.  Exiting" << std::endl;
				LOGGING.close();
				break;
			}
			++*mm_frmcount;
			// Image processing operations
			cvtColor(frame, frame, COLOR_BGR2GRAY);
			frame = halo_noise_and_center(frame.clone(), *mm_frmcount);
			
			if (DEBUG_COUT) {
				LOGGING.open(LOGOUT, std::ios_base::app);
				LOGGING 
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
				LOGGING.close();
			}
			// Store this image into the memory buffer
			memcpy(buf, frame.ptr(), shmem_size);
			
			// Report that the frame was stored
			*mm_frameavail = true;
			
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
						LOGGING.open(LOGOUT, std::ios_base::app);
						LOGGING
						<< "skipping early frame tier3 in frame: "
						<< local_count
						<< std::endl;
						LOGGING.close();
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
