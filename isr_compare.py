import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

data_cpp = "./test.csv"
data_isr = "/home/wes/Documents/Aeroecology_Biologging_Initiative/LunAero Project/Israel_vid1/osf/isrframes.csv"
data_out = "./data/output.csv"
data_ell = "./data/ellipses.csv"

USE_CV = False
DEBUG = True
STATS = True

# "isclose" tolerance values
SPER = 1
SPEA = 10
ANGR = 1e-1 #5e-1
ANGA = 2e-3
RADR = 1
RADA = 10

def frame_cutoff(inout):
	# I only have data from this test video for the first 60k frames (40 minutes)
	inout = inout[np.where(inout[:, 0] < 60000)]
	if DEBUG:
		print("the first 60k frames:", np.shape(inout))
	return inout

def dtype_restrict(inout):
	if np.max(inout[:, 0] > 65535):
		print("the input frame value exceeds integer range!")
		raise RuntimeError
	oldbyte = inout.nbytes
	inout = inout.astype('i4')
	print("reduced floating point precision to uint32, array now takes up", inout.nbytes, "rather than", oldbyte, "bytes")
	return inout

def no_tiny_rad(inout, thresh=1):
	"""
	This function removes all contours from the input list which are smaller than a threshold.
	
	:param inout: The input data set. Must be a Numpy array with shape (:, 4).
	:param thresh: Radius threshold max (default=1)
	:type inout: np.ndarray
	:type thresh: int, float,...
	:return: The output numpy array with shape (:, 4)
	:rtype: np.ndarray
	"""
	inout = inout[np.where(inout[:, 3] > thresh)]
	if DEBUG:
		print("without ultrasmall contours:", np.shape(inout))
	return inout

def max_conts(inout):
	"""
	This function limits the number of contours which can appear in a single frame.  If the number of contours
	in a frame is an outlier, the frame is rejected. Outliers are defined as :math: `\\overline{n} + (3\\,\\sigma_n)`.
	
	:param inout: The input data set. Must be a Numpy array with shape (:, 4).
	:type inout: np.ndarray
	:return: The output numpy array with shape (:, 4)
	:rtype: np.ndarray
	"""
	# Find unique frames/counts of contours
	unique, counts = np.unique(inout[:, 0], return_counts=True)
	# Remove outliers
	#maxarray = unique[np.where(counts <= (int(np.mean(counts) + (3*np.std(counts)))))]
	maxarray = unique[np.where(counts <= 30)]
	inout = inout[np.where(np.isin(inout[:, 0], maxarray))]
	if DEBUG:
		print("noise ceiling:", (int(np.mean(counts) + (1*np.std(counts)))))
		print("noise ceiling shape:", np.shape(inout))
	return inout

def unique_pull(inout):
	"""
	This function performs a pairwise test for unique contour centers across all frames in the input data. If
	an (x, y) center of a contour appears a large number of times such that it is an outlier (again using
	:math: `\\overline{n} + (3\\,\\sigma_n)`), we remove all contours that occur at that center.
	
	:param inout: The input data set. Must be a Numpy array with shape (:, 4).
	:type inout: np.ndarray
	:return: The output numpy array with shape (:, 4)
	:rtype: np.ndarray
	"""
	if DEBUG:
		max_cpp_frame = np.max(inout[:, 0])
		print("cpp max frame reached:", max_cpp_frame)
	# Create test array of x, y points
	test_arr = np.column_stack((inout[:, 1], inout[:, 2]))
	# find unique pairs
	unique, count = np.unique(test_arr, return_counts=True, axis=0)
	# Remove outliers
	unique = unique[np.where(count[:] < (np.mean(count) + (3 * np.std(count))))]
	# Void and ravel to compare the x,y points across the axis
	void_dt = np.dtype((np.void, test_arr.dtype.itemsize * test_arr.shape[1]))
	test_arr = test_arr.view(void_dt).ravel()
	unique = unique.view(void_dt).ravel()
	test_arr = np.isin(test_arr, unique)
	# Only keep the good stuff
	inout = inout[test_arr]
	if DEBUG:
		print("newcpp shape", np.shape(inout))
		print("newcpp median:", np.median(inout[:, 3]))
	return inout

def remove_dupes(inout):
	"""
	This function removes duplicate contours from a single frame, keeping the one with the maximum radius.
	OpenCV returns contours for both "inside" and "outside" a continuous shape.  Normally, the OpenCV code
	would sanitize the input with contour hierarchy, but this is not practical for our case.  Using hierarchy
	may do strange things to the contour of the moon, especially in frames where the camera is moving.  So,
	we must use this work around.  This is a computationally intensive step for large datasets.
	
	:param inout: The input data set. Must be a Numpy array with shape (:, 4).
	:type inout: np.ndarray
	:return: The output numpy array with shape (:, 4)
	:rtype: np.ndarray
	"""
	if DEBUG:
		print("removing duplicates")
	indf = pd.DataFrame(inout)
	outdf = indf.groupby([0, 1, 2])[3].max().reset_index()
	outdf = outdf.values
	if DEBUG:
		print("reduced from", inout.shape[0], "to", outdf.shape[0], "entries")
	return outdf

def concat_x_deg(data):
	"""
	This function concatenates contours across 3 frames (the frame of interest and the two previous ones).
	Since this function tiles and repeats all contours in nearby frames, it may potentially create a super
	large dataset.  It must only be called if the input has been sanitized to remove repeated and junk data.
	
	:param inout: The input data set. Must be a Numpy array with shape (:, 4).
	:type inout: np.ndarray
	:return: The output numpy array with shape (:, 12)
	:rtype: np.ndarray
	"""
	import os
	import re
	unique = np.unique(data[:, 0])
	if DEBUG:
		print("Concat/tile data")
		print("Unique frames:", unique.shape)
	output = np.empty((0, 12))
	cnt = 0
	maxcnt = np.max(unique)
	for i in unique[:]:
		if i > (cnt+1)*1000:
			if DEBUG:
				print("frame:", i)
			if cnt > 0:
				np.savetxt("./data/temp-{:06d}.csv".format(cnt), output, delimiter=',', fmt='%i')
				output = np.empty((0, 12))
			cnt += 1
		if not np.isin(i-1, unique[:]) and np.isin(i-2, unique[:]):
			continue
		temp1 = data[np.where(data[:, 0] == i)]
		temp2 = data[np.where(data[:, 0] == i-1)]
		temp1 = np.column_stack((np.repeat(temp1, np.size(temp2, 0), axis=0), np.tile(temp2, (np.size(temp1, 0), 1))))
		temp2 = data[np.where(data[:, 0] == i-2)]
		temp1 = np.column_stack((np.repeat(temp1, np.size(temp2, 0), axis=0), np.tile(temp2, (np.size(temp1, 0), 1))))
		if np.shape(temp1)[1] == 12:
			output = np.vstack((output, temp1))
	np.savetxt("./data/temp-{:06d}.csv".format(cnt), output, delimiter=',', fmt='%i')
	#for f in sorted(os.listdir("./data")):
		#if re.match(r"temp-[0-9]{6}\.csv", f):
			#output = np.vstack((output, np.loadtxt("./data/"+f, delimiter=',')))
			#os.remove("./data/"+f)
	#if DEBUG:
		#print("shape after concat", np.shape(output))
	if DEBUG:
		print("generated temp files")
	return

def filewise_dirdist_test():
	import os
	import re
	output = np.empty((0, 16))
	if DEBUG:
		print("testing temp files and concat")
	for f in sorted(os.listdir("./data")):
		if re.match(r"temp-[0-9]{6}\.csv", f):
			inout = np.loadtxt("./data/"+f, delimiter=',').astype('i4')
			if DEBUG:
				print("type:", inout.dtype)
				print("input shape of", f, "=", inout.shape)
			# Get distance
			inout = distance_calc(inout)
			# new format: (n) dist1 dist2
			# Get direction
			inout = direction_calc(inout)
			# new format: (n) (dist) dir1 dir2
			# Perform "isclose" tests
			inout = test_dist(inout)
			inout = test_dir(inout)
			inout = test_rad(inout)
			if DEBUG:
				print("output shape of", f, "=", inout.shape)
			output = np.vstack((output, inout))
			os.remove("./data/"+f)
	return output




def distance_calc(inout):
	# in size is [:, 12]
	# Distance formula for each item
	# First for n, n-1
	inout = np.column_stack((inout, np.sqrt(np.square(np.subtract(inout[:, 1], inout[:, 5])) + np.square(np.subtract(inout[:, 2], inout[:, 6])))))
	# Do it again for n-1, n-2
	inout = np.column_stack((inout, np.sqrt(np.square(np.subtract(inout[:, 5], inout[:, 9])) + np.square(np.subtract(inout[:, 6], inout[:, 10])))))
	# The value at dist passes a test, we treat the row as true, else false row.
	# This is broadcast back to our out
	#out = out[np.where((out[:, 8]/10 > smin) & (out[:, 8]/10 < smax), True, False)]
	if DEBUG:
		print("post distcalc size:", np.shape(inout))
	return inout

def direction_calc(inout):
	if DEBUG:
		print("calculating directions")
	# np.arctan directions:
	#                y 90
	#                |
	#       -45      |      45
	#                |
	#                |
	#  -0.0          |          0.0
	# -x ------------|----------- x
	#                |
	#                |
	#                |
	#       45       |     -45
	#                | -90
	#               -y
	#
	# Therefore, complete vertical must be 90
	# in size is [:, 14]
	# remove overlaps, the bird will never NOT move in either x or y
	inout = inout[np.where(np.logical_and(inout[:, 1] != inout[:, 5], inout[:, 2] != inout[:, 6]))]
	inout = inout[np.where(np.logical_and(inout[:, 5] != inout[:, 9], inout[:, 6] != inout[:, 10]))]
	
	temp = np.column_stack((np.subtract(inout[:, 2], inout[:, 6]), np.subtract(inout[:, 1], inout[:, 5])))
	temp2 = np.column_stack((np.subtract(inout[:, 6], inout[:, 10]), np.subtract(inout[:, 5], inout[:, 9])))
	# First for n, n-1
	temp = np.column_stack((temp, np.where(temp[:, 1] == 0, np.where(temp[:, 0] > 0, 90, -90), np.arctan(np.divide(temp[:, 0], temp[:, 1]))*(180/np.pi))))
	# Do it again for n-1, n-2
	temp2 = np.column_stack((temp2, np.where(temp2[:, 1] == 0, np.where(temp2[:, 0] > 0, 0.5*np.pi*(180/np.pi), -0.5*np.pi*(180/np.pi)), np.arctan(np.divide(temp2[:, 0], temp2[:, 1]))*(180/np.pi))))
	
	# Correct value for quadrants such that:
	#
	#                y 90
	#                |
	#       135      |      45
	#                |
	#                |
	#   180          |          0.0
	# -x ------------|----------- x
	#                |
	#                |
	#                |
	#       225      |     315
	#                | 270
	#               -y
	#
	temp = np.column_stack((temp, np.where(temp[:, 1] < 0, 180 + temp[:, 2], np.where(temp[:, 0] < 0, 360 + temp[:, 2], temp[:, 2]))))
	temp2 = np.column_stack((temp2, np.where(temp2[:, 1] < 0, 180 + temp2[:, 2], np.where(temp2[:, 0] < 0, 360 + temp2[:, 2], temp2[:, 2]))))
	
	# Place this in the input-output array
	inout = np.column_stack((inout, temp[:, 3], temp2[:, 3]))
	
	if DEBUG:
		print("post dircalc size:", np.shape(inout))
	return inout

def test_dist(inout):
	# Test distance
	inout = inout[np.isclose(inout[:, 12], inout[:, 13], rtol=SPER, atol=SPEA)]
	if DEBUG:
		print("post disttest size:", np.shape(inout))
	return inout

def test_dir(inout):
	# Test direction
	inout = inout[np.isclose(inout[:, 14], inout[:, 15], rtol=ANGR, atol=ANGA)]
	if DEBUG:
		print("post dirtest size:", np.shape(inout))
	return inout

def test_rad(inout):
	# Radius check
	inout = inout[np.isclose(inout[:, 3], inout[:, 7], rtol=RADR, atol=RADA)]
	inout = inout[np.isclose(inout[:, 7], inout[:, 11], rtol=RADR, atol=RADA)]
	inout = inout[np.isclose(inout[:, 11], inout[:, 3], rtol=RADR, atol=RADA)]
	if DEBUG:
		print("post radtest size:", np.shape(inout))
	return inout

def isr_frames(indata=data_isr):
	"""
	This function pulls reference data from human moonwatching reports.  The input must be formatted such that
	the first and second columns are the first and last frames the bird appears on.
	
	:param indata: Path to the input dataset. Defaults to data_isr global.
	:type inout: str
	:return: israrr - 1D array of all frames which contain a human detected bird.
	:rtype: np.ndarray
	"""
	import csv
	if DEBUG:
		print("making a list of ISR frames")
	isrlist = []
	with open(indata) as csvfile:
		read = csv.reader(csvfile, delimiter=',')
		for row in read:
			for i in range(int(row[0]), int(row[1])+1):
				isrlist.append(i)
	israrr = np.asarray(isrlist)
	return israrr

def compare_to_isr(indata, israrr):
	"""
	This function performs a simple comparison of input data to the ISR human detected data. It simply reports
	the number of frames which match the ISR input.
	
	:param indata: Path to the input dataset. Defaults to data_isr global.
	:type inout: str
	:return: nothing
	"""
	indata = indata[np.where(np.isin(indata[:, 0], israrr[:]))]
	if DEBUG:
		print("newcpp shape of isr frames", np.shape(indata))
	return

def label_isr(inout, israrr):
	inout = np.column_stack((inout, np.where(np.isin(inout[:, 0], israrr), 1, 0)))
	return inout

def isr_misses(inout, israrr):
	inout = inout[np.where(np.isin(inout[:, 0], israrr))]
	if DEBUG:
		print("ISR MISS:", np.shape(inout))
	return inout

def birds_found_not_found(inout):
	cnt = 0
	israrr = np.loadtxt(data_isr, delimiter=',')
	for i in israrr[:]:
		temp = np.arange(int(i[0]), int(i[1])+1)
		if (np.sum(np.isin(temp[:], inout[:, 0])) == 0):
			cnt += 1
			if DEBUG:
				print("missing track: ", temp)
	if DEBUG:
		print("missed", cnt, "of", israrr.shape[0], "bird contour traces")
	return

#def cone_prediction(inout):
	## Make temp array of n0( x, y) and dist1, dist2, dir1, dir2
	## Direction ==0 for +x axis.  Negative values are +y axis
	#temp = np.column_stack((inout[:, 0:3], inout[:, 12:16]))
	## Direction of line with averages of dist and dir
	#temp = np.column_stack((temp, (temp[:, 3] + temp[:, 4])/2, (temp[:, 5] + temp[:, 6])/2))
	##temp = np.column_stack((temp, (temp[:, 3] + temp[:, 4])/2, np.where(((temp[:, 5] + temp[:, 6])/2) > 0, ((temp[:, 5] + temp[:, 6])/2), ((temp[:, 5] + temp[:, 6])/2)+180)))
	
	#circ = np.loadtxt(data_ell, delimiter=',')
	## "center" the moon by replacing the x y center
	#circ = np.column_stack((circ[:, 0], np.repeat(960, np.size(circ, 0), axis=0), np.repeat(540, np.size(circ, 0), axis=0), circ[:, 3:]))
	
	#tiled = np.hstack([temp, circ[temp[:, 0].astype(int), 1:]])
	#print("tiled shape:", tiled.shape)
	
	### X value of intersection between vector from point of interest to the ellipse
	#matharr = -(tiled[:, 12]/2*tiled[:, 13]*np.sqrt(((-tiled[:, 10]**2)+2*tiled[:, 1]*tiled[:, 10]-tiled[:, 1]**2+tiled[:, 12]**2)*np.tan(tiled[:, 8])**2+((2*tiled[:, 10]-2*tiled[:, 1])*tiled[:, 11]-2*tiled[:, 2]*tiled[:, 10]+2*tiled[:, 1]*tiled[:, 2])*np.tan(tiled[:, 8])-tiled[:, 11]**2+2*tiled[:, 2]*tiled[:, 11]-tiled[:, 2]**2+tiled[:, 13]**2)-tiled[:, 12]**2*tiled[:, 1]*np.tan(tiled[:, 8])**2+(tiled[:, 12]**2*tiled[:, 2]-tiled[:, 12]**2*tiled[:, 11])*np.tan(tiled[:, 8])-tiled[:, 13]**2*tiled[:, 10])/(tiled[:, 12]**2*np.tan(tiled[:, 8])**2+tiled[:, 13]**2)
	### Y value of the same
	#matharr2 = np.tan(tiled[:, 8])*matharr[:]+tiled[:, 2]-np.tan(tiled[:, 8])*tiled[:, 1]
	#inout = np.column_stack((tiled, matharr, matharr2))
	#print("shape with matharr:", inout.shape)

	
	## Max distance between point of interest and point on ellipse
	#inout = np.column_stack((inout, np.sqrt(np.square(np.subtract(inout[:, 1], inout[:, 14]))+np.square(np.subtract(inout[:, 2], inout[:, 15])))))
	
	#return inout

#def score_others(inout):
	## input has row width of 17
	## frame, x, y, dist1, dist2, dir1, dir2, distavg, diravg, ellcentx, ellcenty, ellax1, ellax2, ellang, matharrx, matharry, maxdist
	#print("prescore shape:", inout.shape)
	#scorearr = np.empty((0, 2))
	#jumparr = np.empty((0, 3))
	#for i in inout:
		## Select only future frames
		#temp = inout[np.where(inout[:, 0] > i[0])]
		## Select only frames within reasonable range
		#temp = temp[np.where(np.subtract(temp[:, 0], i[0]) <= np.divide(i[16], i[7]))]
		## The new point should be within a radius width of a distance prediction
		#temp = temp[np.isclose(np.sqrt(np.square(np.subtract(temp[:, 1], i[1]))+np.square(np.subtract(temp[:, 2], i[2]))), i[7], rtol=RADR*(1.1*np.divide(np.sqrt(np.square(np.subtract(temp[:, 1], i[1]))+np.square(np.subtract(temp[:, 2], i[2]))), i[7])), atol=RADA*(1.1*np.divide(np.sqrt(np.square(np.subtract(temp[:, 1], i[1]))+np.square(np.subtract(temp[:, 2], i[2]))), i[7])))]
		## The speed of object traveling should be close
		#temp = temp[np.isclose(i[7], np.divide(temp[:, 13]+temp[:, 14], 2), rtol=SPER, atol=SPEA)]
		## The direction the object is traveling should be close
		##temp = temp[np.isclose(i[8], np.divide(temp[:, 15]+temp[:, 16], 2), rtol=ANGR*2, atol=ANGA*2)]
		#which_jump = np.column_stack((np.repeat(i[0], temp[:, 0].shape[0]), temp[:, 0], np.divide(np.sqrt(np.square(np.subtract(temp[:, 1], i[1]))+np.square(np.subtract(temp[:, 2], i[2]))), i[7])))
		#scorearr = np.vstack((scorearr, np.array((i[0], np.shape(temp)[0]))))
		#jumparr = np.vstack((jumparr, which_jump))
	#scorearr = scorearr[np.where(scorearr[:, 1] > 0)]
	## This array stacks the starting frame, each unique next frame, and the average distance between them
	#thirdarr = np.empty((0, 3))
	#for i in np.unique(jumparr[:, 0]):
		#local = np.unique(jumps[np.where(jumps[:, 0] == i)][:, 1])
		#thirdarr = np.vstack((thirdarr, np.column_stack((np.repeat(i, local.shape), np.unique(jumps[np.where(jumps[:, 0] == i)][:, 1]), np.repeat(np.mean(jumps[np.where(jumps[:, 0] == i)][:, 2]), local.shape)))))
	#return scorearr, jumparr, thirdarr

def cleanup_dirs(inout):
	if DEBUG:
		print("correcting directional data for left and right orientation")
	# correct left and right side values
	inout = np.column_stack((inout[:, :14], np.where(inout[:, 1] > inout[:, 9], 180-inout[:, 14], inout[:, 14]), np.where(inout[:, 1] > inout[:, 9], 180-inout[:, 15], inout[:, 15]), inout[:, 16:]))
	# remove back and forth
	inout = inout[np.isclose(inout[:, 14], inout[:, 15], rtol=ANGR, atol=ANGA)]
	return inout

def avg_speed_dir(inout):
	if DEBUG:
		print("averaging speed and direction in short linear sets")
	# average the distance and direction calcs in column 17, 18
	inout = np.column_stack((inout[:], np.mean(inout[:, 12:14], axis=1), np.mean(inout[:, 14:16], axis=1)))
	# remove very slow contours
	inout = inout[np.where(inout[:, 17] > 10)]
	# change average angle to radians vector angle
	inout = np.column_stack((inout[:, :18], inout[:, 18]*(np.pi/180)))
	return inout

def fetch_ellipse(inout):
	if DEBUG:
		print("fetching ellipse data from:", data_ell)
	# get moon ellipse size
	circ = np.loadtxt(data_ell, delimiter=',')
	# "center" the moon by replacing the x y center
	circ = np.column_stack((circ[:, 0], np.repeat(960, np.size(circ, 0), axis=0), np.repeat(540, np.size(circ, 0), axis=0), circ[:, 3:]))
	# combine this with our inout data (adds ellx, elly, ellaxx, ellaxy, ellang)
	inout = np.hstack([inout, circ[inout[:, 0].astype(int), 1:]])
	return inout

def attach_links(inout):
	if DEBUG:
		print("attach_links input shape:", inout.shape)
	
	# SCARY MATH
	#-(a*b*sqrt(((-m^2)+2*j*m-j^2+a^2)*tan(t)^2+((2*m-2*j)*n-2*k*m+2*j*k)*tan(t)-n^2+2*k*n-k^2+b^2)-a^2*j*tan(t)^2+(a^2*k-a^2*n)*tan(t)-b^2*m)/(a^2*tan(t)^2+b^2)
	#or
	#(a*b*sqrt(((-m^2)+2*j*m-j^2+a^2)*tan(t)^2+((2*m-2*j)*n-2*k*m+2*j*k)*tan(t)-n^2+2*k*n-k^2+b^2)+a^2*j*tan(t)^2+(a^2*n-a^2*k)*tan(t)+b^2*m)/(a^2*tan(t)^2+b^2)
	
	# inout must use floats
	inout = inout.astype(float)
	# calculate endpoint xy of vector on ellipse
	matharr = -(inout[:, 21]/2*inout[:, 22]*np.sqrt(((-inout[:, 19]**2)+2*inout[:, 1]*inout[:, 19]-inout[:, 1]**2+inout[:, 21]**2)*np.tan(inout[:, 18])**2+((2*inout[:, 19]-2*inout[:, 1])*inout[:, 20]-2*inout[:, 2]*inout[:, 19]+2*inout[:, 1]*inout[:, 2])*np.tan(inout[:, 18])-inout[:, 20]**2+2*inout[:, 2]*inout[:, 20]-inout[:, 2]**2+inout[:, 22]**2)-inout[:, 21]**2*inout[:, 1]*np.tan(inout[:, 18])**2+(inout[:, 21]**2*inout[:, 2]-inout[:, 21]**2*inout[:, 20])*np.tan(inout[:, 18])-inout[:, 22]**2*inout[:, 19])/(inout[:, 21]**2*np.tan(inout[:, 18])**2+inout[:, 22]**2)
	## Y value of the same
	matharr = np.column_stack((matharr, np.tan(inout[:, 18])*matharr[:]+inout[:, 2]-np.tan(inout[:, 18])*inout[:, 1]))
	# stacking adds elltargetx, elltargety; switch back to int
	inout = np.column_stack((inout, matharr)).astype('i4')
	# calculate max frames away the final contour could be away from this one
	#inout = np.column_stack((inout, np.divide(np.sqrt(np.square(np.subtract(inout[:, 1], inout[:, 24]))+np.square(np.subtract(inout[:, 2], inout[:, 25]))), inout[:, 17]).astype(int)+2))
	inout = np.column_stack((inout, np.repeat(3, inout.shape[0])))
	if DEBUG:
		print("new shape with math arrays", inout[0])
	output = np.empty((0, 28))
	cnt = 0
	for i in np.unique(inout[:, 0]):
		if i > (cnt+1)*1000:
			if DEBUG:
				print("merging frame:", i)
			cnt += 1
		maxdist = min(np.max(inout[np.where(inout[:, 0] == i)][:, 26]), 10)
		print(i, "maxdist", maxdist, "with shape", inout[np.where(inout[:, 0] == i)].shape)
		# each have shape of (:, 27)
		## newer frames
		potential = inout[np.where(np.logical_and(inout[:, 0] > i, inout[:, 0] < (maxdist+i+1)))]
		thisframe = inout[np.where(inout[:, 0] == i)]
		## newer frames
		merged = np.column_stack((np.repeat(thisframe, np.size(potential, 0), axis=0), np.tile(potential, (np.size(thisframe, 0), 1))))
		## test dir
		merged = merged[np.isclose(merged[:, 18], merged[:, 18+27], rtol=ANGR, atol=ANGA)]
		merged = np.column_stack((merged, np.divide(np.absolute(np.multiply((merged[:, 25]-merged[:, 2]), merged[:, 1+27])-np.multiply((merged[:, 24]-merged[:, 1]), merged[:, 2+27])+(merged[:, 24]*merged[:, 2])-(merged[:, 25]*merged[:, 1])), np.sqrt((merged[:, 25]-merged[:, 2])**2+(merged[:, 24]-merged[:, 1])**2))))
		## only values near the line are kept
		merged = merged[np.where(merged[:, 54] < 10)]
		## count the potentials and add it to the list
		output = np.vstack((output, np.column_stack((thisframe, np.repeat(merged.shape[0], thisframe.shape[0])))))
		print("outputshape", output.shape)
	#for i in inout:
		## newer frames
		#potential = inout[np.where(np.logical_and(inout[:, 0] > inout[inout[:, 0], 1:], inout[:, 0] < (i[26]+i[0])))]
		## test dist
		
		## test dir
		#potential = potential[np.isclose(i[18], potential[:, 18], rtol=ANGR, atol=ANGA)]
		## calculate the distance from potential point to line
		#potential = np.column_stack((potential, np.divide(np.absolute(np.multiply((i[25]-i[2]), potential[:, 1])-np.multiply((i[24]-i[1]), potential[:, 2])+(i[24]*i[2])-(i[25]*i[1])), np.sqrt((i[25]-i[2])**2+(i[24]-i[1])**2))))
		## only values near the line are kept
		#potential = potential[np.where(potential[:, 27] < 10)]
		
		## count the potentials and add it to the list
		#output = np.vstack((output, np.hstack((i, potential.shape[0]))))
	return output

def draw_boxes_on_frames(inout):
	import cv2
	import glob
	if not glob.glob("./data/frames_labeled/"):
		import os
		os.mkdir("./data/frames_labeled")
	
	for i in inout:
		futurebirds = inout[np.where(np.logical_and(inout[:, 0] > i[0], inout[:, 0] < (i[26]+i[0])))]
		futurebirds = futurebirds[np.where(futurebirds[:, 27] < 10)]
		
		# open image for i[0] point
		filename = "./data/frames/{:08d}.png".format(int(i[0]))
		if not glob.glob(filename):
			continue
		img = cv2.imread(filename)
		img = cv2.arrowedLine(img, (int(i[1]), int(i[2])), (int(i[24]), int(i[25])), (0, 0, 255), 2)
		
		#cv2.putText(img, text, org, fontFace, fontScale, color[, thickness[, lineType[, bottomLeftOrigin]]])
		font = cv2.FONT_HERSHEY_SIMPLEX
		cv2.putText(img, "dist -1: " + str(i[12]), (10, 25), font, 1, (255, 255, 255))
		cv2.putText(img, "dist -2: " + str(i[13]), (10, 50), font, 1, (255, 255, 255))
		cv2.putText(img, "dir -1 : " + str(i[14]), (10, 100), font, 1, (255, 255, 255))
		cv2.putText(img, "dir -2 : " + str(i[15]), (10, 125), font, 1, (255, 255, 255))
		
		cv2.circle(img, (int(i[5]), int(i[6])), int(i[7]), (255, 0, 255), 2)
		cv2.circle(img, (int(i[9]), int(i[10])), int(i[11]), (255, 0, 255), 2)
		
		for j in futurebirds:
			cv2.circle(img, (int(j[1]), int(j[2])), int(j[3]), (255, 255, 0), 3)
			cv2.circle(img, (int(j[5]), int(j[6])), int(j[7]), (255, 255, 0), 3)
			cv2.circle(img, (int(j[9]), int(j[10])), int(j[11]), (255, 255, 0), 3)
		cv2.imwrite("./data/frames_labeled/{:08d}.png".format(int(i[0])), img)
	return

def isr_percent(inout):
	isrframes = np.loadtxt(data_isr, delimiter=',')
	ourframes = np.unique(np.hstack((np.hstack((inout[:, 0], inout[:, 4])), inout[:, 8])))
	truthvals = np.empty((0))
	for i in isrframes:
		temp = np.arange(i[0], i[1]+1)
		truthvals = np.hstack((truthvals, np.isin(temp, ourframes)))
	hitpercent = np.sum(truthvals)/np.shape(truthvals)[0]
	misspercent = (np.shape(truthvals)[0] - np.sum(truthvals))/np.shape(truthvals)[0]
	print("This code detected", hitpercent*100, "%% of the birds in ISR")
	return

def main():
	## in format: frame, x, y, r
	
	# Get frames
	out_cat = np.genfromtxt(data_cpp, delimiter=',')
	if DEBUG:
		print("cpp data shape", np.shape(out_cat))
	
	# Only the first 60k frames
	out_cat = frame_cutoff(out_cat)
	
	# Restrict datatypes
	out_cat = dtype_restrict(out_cat)
	
	# Remove contours that are 0.5px radius
	out_cat = no_tiny_rad(out_cat)
	
	# Only use frames that have less than a threshold of contours per frame
	out_cat = max_conts(out_cat)
	
	## Only keep contours with x/y coordinates that don't appear too often
	out_cat = unique_pull(out_cat)
	
	# remove duplicate contours
	out_cat = remove_dupes(out_cat)
	
	##Save Point
	np.savetxt(data_out, out_cat, delimiter=',', fmt='%i')
	
	# Store this entire list in an alt spot for later
	alt_cat = out_cat
	
	# expand data for nearby contours in time
	concat_x_deg(out_cat)
	
	# new format: n, n-1, n-2
	
	## Gen array of frames found by ISR
	#israrr = isr_frames()
	## Only keep frames found by ISR
	#compare_to_isr(out_cat, israrr)
	
	out_cat = filewise_dirdist_test()
	
	if DEBUG:
		print("Save point size:", np.shape(out_cat))
	
	## Get distance
	#out_cat = distance_calc(out_cat)
	## new format: (n) dist1 dist2
	
	## Get direction
	#out_cat = direction_calc(out_cat)
	## new format: (n) (dist) dir1 dir2
	
	## Perform "isclose" tests
	#if DEBUG:
		#print("performing isclose tests")
	#out_cat = test_dist(out_cat)
	#out_cat = test_dir(out_cat)
	#out_cat = test_rad(out_cat)
	
	# Gen array of frames found by ISR
	israrr = isr_frames()
	# Only keep frames found by ISR
	compare_to_isr(out_cat, israrr)
	
	# Label the hits and misses
	out_cat = label_isr(out_cat, israrr)
	
	##Save Point
	np.savetxt(data_out, out_cat, delimiter=',')
	
	# Create alt out file with misses from ISR
	alt_cat = isr_misses(alt_cat, israrr)
	np.savetxt("./data/isrmisses.csv", alt_cat, delimiter=',')

	if DEBUG:
		print("unique frames in hits:", np.shape(np.unique(out_cat[:, 0])))
		print("unique frames in misses:", np.shape(np.unique(alt_cat[:, 0])))
	
	# Check how many bird tracks we found from ISR
	birds_found_not_found(out_cat)
	
	# Load savepoint
	#out_cat = np.loadtxt("./data/output.csv", delimiter=',')
	
	# Sanitize directional input
	out_cat = cleanup_dirs(out_cat)
	
	# Average the speed and direction values for a linear segment and convert to radians
	out_cat = avg_speed_dir(out_cat)
	
	# Import the ellipse information from the CPP code
	out_cat = fetch_ellipse(out_cat)
	
	# Link longer timeframe data with matching linear patterns
	outscore = attach_links(out_cat)
	
	# Save new results
	np.savetxt("./data/testout.csv", outscore, delimiter=',')
	
	if USE_CV:
		draw_boxes_on_frames(outscore)
	
	# ISR hit or miss?
	isr_percent(outscore)
	
	
	hits = outscore[np.isin(outscore[:, 0], israrr)]
	miss = outscore[np.invert(np.isin(outscore[:, 0], israrr))]
	plt.style.use('seaborn-white')
	#fig = plt.figure(figsize=[11,8.5])
	lab = ["Hits", "Misses"]
	col = ["black", "red"]
	# Screen Parameters
	#plt.rcParams["figure.figsize"] = [20,10]
	params = {"figure.figsize": [11, 8.5],
			'legend.fontsize': 'x-large',
			'axes.labelsize': 'x-large',
			'axes.titlesize':'x-large',
			'xtick.labelsize':'x-large',
			'ytick.labelsize':'x-large'}
	plt.rcParams.update(params)
	plt.tight_layout()

	# Data init
	#hits = goodscore[:, 1]
	#miss = badscore[:, 1]
	bins = np.linspace(0, 20, 50)
	# Plot of how the major and minor axes of the ellipse changed in relation to number of hits.

	ax1 = plt.subplot(111)
	#plt.hist([hits, miss], bins, label=lab , color=col)
	plt.scatter(miss[:, 0], miss[:, 27], c=col[1], marker='o', s=2, alpha=0.5, label="Misses")
	plt.scatter(hits[:, 0], hits[:, 27], c=col[0], marker='o', s=2, alpha=0.5, label="Hits")
	axes = plt.gca()
	#axes.set_ylim([0,100])
	plt.xlabel("Frame Number")
	plt.ylabel("Score")
	plt.grid(True)
	plt.show()
	
	
	
	### Phase 2, longer range prediction and detection
	## For each trace, create a "cone" of prediction
	#out_cat2 = cone_prediction(out_cat)
	
	## For each cone, check to find other traces within the cone and apply them
	#outscore = score_others(out_cat2)
	
	## Check to see that the completed linear path crosses the moon
	#np.savetxt("./data/scores.csv", outscore, delimiter=',')
	
	#goodscore = outscore[np.isin(outscore[:, 0], israrr)]
	#badscore = outscore[np.invert(np.isin(outscore[:, 0], israrr))]
	
	
	
	
	
	## Load Save
	#out_cat = np.loadtxt(data_out, delimiter=',')
	#alt_cat = np.loadtxt("./data/isrmisses.csv", delimiter=',')
	
	#plt.style.use('seaborn-white')
	##fig = plt.figure(figsize=[11,8.5])
	#lab = ["Hits", "Misses"]
	#col = ["black", "red"]
	## Screen Parameters
	##plt.rcParams["figure.figsize"] = [20,10]
	#params = {"figure.figsize": [11, 8.5],
			#'legend.fontsize': 'x-large',
			#'axes.labelsize': 'x-large',
			#'axes.titlesize':'x-large',
			#'xtick.labelsize':'x-large',
			#'ytick.labelsize':'x-large'}
	#plt.rcParams.update(params)
	#plt.tight_layout()
	
	## Data init
	#hits = out_cat[np.where(out_cat[:, 16] == 1)]
	#miss = out_cat[np.where(out_cat[:, 16] == 0)]
	
	#hits_frame_unique, hits_frame_count = np.unique(hits[:, 0], return_counts=True)
	#miss_frame_unique, miss_frame_count = np.unique(miss[:, 0], return_counts=True)
	
	## Plot of how the major and minor axes of the ellipse changed in relation to number of hits.
	#ax1 = plt.subplot(111)
	
	#plt.scatter(miss_frame_unique, miss_frame_count, c=col[1], marker='o', s=2, alpha=0.5, label="Misses")
	#plt.scatter(hits_frame_unique, hits_frame_count, c=col[0], marker='o', s=2, alpha=0.5, label="Hits")
	#ax1.legend()
	#plt.xlabel("Frame Number")
	#plt.ylabel("Contours per Frame")
	#plt.grid(True)
	#plt.show()
	
	
	#hits = out_cat[:, 3]
	#miss = alt_cat[:, 3]
	#print("hits", np.histogram(out_cat[:, 3]))
	#print("misses", np.histogram(alt_cat[:, 3]))
	## Size of contours in hits and misses
	##bins = np.linspace(np.min(out_cat[:, 3])-1, np.max(out_cat[:, 3])+1, 50)
	#bins = np.linspace(0, 20, 50)
	#ax1 = plt.subplot(111)
	#plt.hist([hits, miss], bins, label=lab , color=col)
	#axes = plt.gca()
	##axes.set_ylim([0,bintop])
	##axes.set_xlim([0,np.max(lro[:, 22])+1000])
	#plt.xlabel("Size of Contour (pixels^2)")
	#plt.ylabel("Count")
	#plt.title("Bird Contour Size")
	##ax5.text(320000,7, "Bins = 500", bbox = {"pad":10, "facecolor":"white"})
	##coordinates, text
	#ax1.legend()
	#plt.grid(True)
	#plt.show()
	
	
	
	##alt_cat = np.loadtxt("./data/isrmisses.csv", delimiter=',')
	##ufram = unique_frames(alt_cat)
	##alt_cat = concat_x_deg(alt_cat, ufram)
	##np.savetxt("./data/isrmisses_exp.csv", alt_cat, delimiter=',')
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	

if __name__ == '__main__':
	main()
