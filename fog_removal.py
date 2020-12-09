# CPP_Birdtracker/fog_removal.py - Script to determine appropriate BLACKOUT_THRESH
# Copyright (C) <2020>  <Wesley T. Honeycutt>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


##@file fog_removal.py
##@brief A script to remove "fog" from around the moon prior to run.
##
##This script uses the OpenCV highgui to allow the user to test the first frame of a video to
##determine the most approparite thresholding value to remove fog from the around the moon.  The
##threshold value selected here should be used as the input value of BLACKOUT_THRESH in the
##settings.cfg config file.  Use the terminal to run this script, as it requires the user to choose
##the file to be operated on using an "-f" switch.
##
##@section example_fog Usage Example
##@verbatim
##python3 ./fog_removal.py -f /path/to/file.mp4
##@endverbatim
##
##@section libraries_fog Libraries/Modules
##- OpenCV 3.0 or later
##- Numpy

# Default imports
import os

# User imports
import numpy as np
import cv2

##\cond DOXYGEN_SHOULD_SKIP_THIS
TOGGLE_PRESSED = False

def nothing(args):
	"""
	nothing
	"""
	pass
##\endcond /* DOXYGEN_SHOULD_SKIP_THIS */

def create_directions():
	"""!
	Create intro/directions slide for the thresholding Python script.
	
	@return image A 1080x1080 black 3 color image with red text (CV Mat)
	"""
	image = np.zeros((1000, 1000, 3))
	font = cv2.FONT_HERSHEY_SIMPLEX
	text_color = (0, 0, 255)
	text_lines = ["Use this program to determine the optimal threshold", \
				"value for the input vid.  Adjust the \"Threshold\"", \
				"slider until the fog around the moon disappears and the", \
				"edge of the moon is crisp, without losing parts of the", \
				"edge. When you are happy with the results, use the value", \
				"on the slider in this windowas the value for ", \
				"BLACKOUT_THRESH in settings.cfg.", \
				"", "", "", "", "", "", \
				"To exit this script, press ESC", \
				"", "", "", \
				"To begin testing the image, or return to these directions,", \
				"use the \"Directions\" slider"
	]
	for i, j in enumerate(text_lines):
		image = cv2.putText(image, j, (50, 50*(i+1)), font, 1, text_color, 2, cv2.LINE_AA)
	return image

def is_valid_file(parser, arg):
	"""!
	Check if arg is a valid file that already exists on the file system.
	
	@param parser the argparse object
	@param arg the string of the filename we want to test
	
	@return arg
	"""
	arg = os.path.abspath(arg)
	if not os.path.exists(arg):
		parser.error("The file %s does not exist!" % arg)
		raise FileNotFoundError("The file you told the script to run on does not exist")
	else:
		return arg

def check_positive(value):
	"""!
	Check that the input integer for the starting frame number is valid
	"""
	ivalue = int(value)
	if ivalue <= 0:
		raise RuntimeError("%s is an invalid positive int value" % value)
	return ivalue


def get_parser():
	"""!
	Get parser object for this script
	
	@return Parser object
	"""
	from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
	parser = ArgumentParser(description=__doc__, formatter_class=ArgumentDefaultsHelpFormatter)
	parser.add_argument("-f", "--file", dest="filename", required=True,\
		type=lambda x: is_valid_file(parser, x), help="video input FILE", metavar="FILE")
	parser.add_argument("-n", "--nth_frame", dest="max_frame", required=False,\
		type=check_positive, help="Use the nth frame of the video to test fog", \
		nargs=None, default=100, metavar='N')
	return parser

def main():
	"""!
	Perform primary functions of this script.
	Command line arguments are first parsed to get the file name on which we want to operate.  Then,
	we ensure that this is a valid file.  The video file is opened and the first frame is read.
	We then create our output window of size 1080x1080 and trackbars for the Directions toggle and
	Image Threshold.  If everything was successful, the window opens with the Diretions image.  When
	the user tells the Directions to toggle closed, the first frame from the video is shown after
	conversion to grayscale and thresholded to zero.  The image threshold is determined by the value
	of the threshold trackbar.  The user exits with the ESC key.  On exit, the final value of the
	threshold is printed to the terminal.
	"""
	# Prepare input file based on the terminal command args
	args = get_parser().parse_args()
	print("Operating on file:", args.filename)
	# Get the first frame of the video from the terminal command
	cap = cv2.VideoCapture(args.filename)
	ret, frame = cap.read()
	cnt = 1
	cnt_max = args.max_frame
	while cnt < cnt_max:
		ret, frame = cap.read()
		cnt = cnt + 1
	# Create our window and relevant trackbars	
	cv2.namedWindow("image", cv2.WINDOW_NORMAL)
	cv2.resizeWindow("image", 1080, 1080)
	cv2.createTrackbar("Directions", "image", 0, 1, nothing)
	cv2.createTrackbar("Threshold", "image", 0, 255, nothing)
	# Misc initial values
	outimg = create_directions()
	# If we got a video frame, execute the main loop.
	if ret:
		# Misc initial values
		pretest = 0
		thresh = 0
		# Convert input image to gray
		frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
		while(1):
			# Get the value of your directions toggle
			local_button = cv2.getTrackbarPos("Directions", "image")
			if (local_button == 0) and (pretest == 1):
				outimg = create_directions()
			# Set the value for the directions toggle
			if local_button > 0:
				TOGGLE_PRESSED = True
			else:
				TOGGLE_PRESSED = False
			# Process the image with the threshold value
			if (TOGGLE_PRESSED):
				thresh = cv2.getTrackbarPos("Threshold", "image")
				_, outimg = cv2.threshold(frame, thresh, 255, cv2.THRESH_TOZERO)
			# Show the image, either directions or the thresholded image
			cv2.imshow("image", outimg)
			# ESC to kill this script
			kill = cv2.waitKey(1) & 0xFF
			if kill == 27:
				print("Your final threshold value was:", thresh)
				break
			# Set the pretest value for the next loop
			pretest = local_button
	
	else:
		# Error loading video
		raise RuntimeError("ERROR: could not get frame from video")
	cv2.destroyAllWindows()
	return

if __name__ == "__main__":
	main()
