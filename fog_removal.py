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
##- imutils (for ASCII variant)

# Default imports
import os
import subprocess

# User imports
import numpy as np
import cv2

"""!
This is a user configurable option to match the color schema of your terminal for using the ascii
imaging options.  If your terminal is dark text on a light background, set to False.  If your
terminal is light text on a dark background, set to True.  You can safely ignore this if you
are using the GUI version.
"""
BLACK_BACK = True

##\cond DOXYGEN_SHOULD_SKIP_THIS
TOGGLE_PRESSED = False
TOGGLE_ASC = False

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

def ascii_version(in_frame):
	"""!
	This is a terminal-only version of the fog removal code.  In this loop, the user is prompted
	for a threshold value to test or to exit by entering "q".  The in_frame param passes through to
	the nested function.
	
	@param in_frame The input 3-channel OpenCV image
	"""
	print("=======================================================================")
	print("Use this program to determine the optimal threshold value for the input vid.  ASCII mode is enabled for use in terminals without GUI access.  Try to make any fog around the moon disappear to make the moon appear with a crisp edge. Don't overdo it and lose some of the edge though!  The original image uses threshold 0, the max value of 255 will make all but the brightest spots disappear.  When you are happy with the results, use the value you entered as the value for BLACKOUT_THRESH in settings.cfg.\n")
	print("=======================================================================")
	local_exit_toggle = True
	in_frame = cv2.cvtColor(in_frame, cv2.COLOR_BGR2GRAY)
	while (local_exit_toggle):
		print("Enter a value between (0-255) or \"q\" to exit")
		local_string = input("Entry:")
		if (local_string == 'q') or (local_string == 'Q'):
			local_exit_toggle = False
		elif local_string.isdigit():
			if int(local_string) < 256:
				ascii_proc(in_frame, int(local_string))
			else:
				print("You entered an invalid integer value, please choose between 0-255")
		else:
			print("Unrecognized characters in entry.  Try again")
	return

def ascii_proc(in_frame, thresh):
	"""!
	This function does the heavy lifting for the ASCII version of this code.  The input image
	is cropped to ignore as much black background as possible, then resized to fit the user's
	terminal (using output from `tput`).  The image is thresholded, and the output from this
	thresholding operaiton is converted to a string of ASCII characters.  There are 97 characters
	in the grayscale list.  If the user has a black background, as determined by the global
	variable BLACK_BACK, the order of ASCII characters are reversed.  After printing our "image",
	the user is informed if the image was resized and reminded of what value they entered.
	
	@param in_frame The input 1-channel OpenCV image
	@param thresh An integer between 0-255 to represent the threshold of to test here
	"""
	import imutils
	
	# Stores if the image was resized
	rezzed = False
	
	# Empty output "image"
	picstring = ""
	
	# Fetch the width of the user's terminal as an int
	local_width = int(subprocess.check_output(["tput", "cols"]).decode("ascii").strip('\n'))
	
	# A list of characters in order of descending darkness for use in ASCII images.  97 items.
	ascii_list = ["@", "M", "B", "H", "E", "N", "R", "#", "K", "W", "X", "D", "F", "P", "Q", "A", "S", "U", "Z", "b", "d", "e", "h", "x", "*", "8", "G", "m", "&", "0", "4", "L", "O", "V", "Y", "k", "p", "q", "5", "T", "a", "g", "n", "s", "6", "9", "o", "w", "z", "$", "C", "I", "u", "2", "3", "J", "c", "f", "r", "y", "%", "1", "v", "7", "l", "+", "i", "t", "[", "]", "", "{", "}", "?", "j", "|", "(", ")", "=", "~", "!", "-", "/", "<", ">", "\\", "\"", "^", "_", "'", ";", ",", ":", "`", ".", " ", " "]
	
	# For dark background terminals, use the reverse of the list
	if BLACK_BACK:
		ascii_list.reverse()
	
	# Divisor ratio to give weight to each ASCII value
	ascii_ratio = 256/len(ascii_list)
	
	# Fetch largest contour and crop to that size
	contours, hierarchy = cv2.findContours(in_frame, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
	# Holds largest bounding box
	box = (0,0,0,0)      # biggest bounding box so far
	# Holds largest bounding box area
	box_area = 0
	for cont in contours:
		# Retrieves the boudning box properties x, y, width, and height
		xxx, yyy, www, hhh = cv2.boundingRect(cont)
		area = www * hhh
		# Check if this is a bigger one
		if area > box_area:
			box = xxx, yyy, www, hhh
			box_area = area
	# Once we found the biggest contour, we re-grab the boundaries
	xxx, yyy, www, hhh = box
	# And do an OpenCV crop
	roi=in_frame[yyy:yyy+hhh, xxx:xxx+www]
	
	# Resize the cropped image if the terminal is too small.
	# We only worry about width since modern terminals have scrolling height.
	if (roi.shape[0] > local_width):
		roi = imutils.resize(roi, width=local_width)
		rezzed = True
	
	# perform thresh
	_, roi = cv2.threshold(roi, thresh, 255, cv2.THRESH_TOZERO)
	
	rows, cols = roi.shape
	print("generating image...")
	for i in range(0, rows):
		for j in range(0, cols):
			# Convert pixel value to scaled ASCII code.
			picstring += ascii_list[int(roi[i, j]/ascii_ratio)]
		picstring += '\n'
	print(picstring)
	if rezzed:
		print("Your terminal is", local_width, " pixels wide, but the cropped raw image was", cols, "pixels wide.  Your output has been resized.")
	print("The above image threshold value was", thresh)
	return

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
	parser.add_argument("-a", "--ascii", dest="ascii_toggle", required=False, action="store_true",\
		help="Toggle for an ascii (terminal) only version of this script", default=False)
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
	TOGGLE_ASC = args.ascii_toggle
	while cnt < cnt_max:
		ret, frame = cap.read()
		cnt = cnt + 1
	if TOGGLE_ASC:
		ascii_version(frame)
	else:
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
