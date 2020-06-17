import h5py
import matplotlib.pyplot as plt
import numpy as np
import os
import pandas as pd
import re

from sklearn.model_selection import train_test_split
from sklearn.discriminant_analysis import LinearDiscriminantAnalysis as LDA
from sklearn.metrics import confusion_matrix
from sklearn.metrics import accuracy_score
from sklearn.preprocessing import StandardScaler

data_cpp = "./test.csv"
data_isr = "/home/wes/Documents/Aeroecology_Biologging_Initiative/LunAero Project/Israel_vid1/osf/isrframes.csv"
data_out = "./data/output.csv"
data_ell = "./data/ellipses.csv"

USE_CV = False
DEBUG = True
STATS = True

with h5py.File("temp.hdf5", 'w') as HF:
	out_cat = np.genfromtxt(data_cpp, delimiter=',')
	if DEBUG:
		print("cpp data shape", np.shape(out_cat))
	HF.create_dataset("raw", data=out_cat, compression="gzip", chunks=True, maxshape=(None, None))

HF = h5py.File("temp.hdf5", 'a')



# "isclose" tolerance values
SPER = 1
SPEA = 10
ANGR = 5 #5e-1
ANGA = 15
RADR = 1
RADA = 10

# Column Titles to values
FRM0 = 0      # frame at n-0
XXX0 = 1      # contour x at n-0
YYY0 = 2      # contour y at n-0
RAD0 = 3      # contour radius at n-0
LAP0 = 4      # contour average laplacian at n-0
PRM0 = 5      # contour perimeter at n-0
DENS = 6      # contour density
DUPE = 7      # counter for duplicates

FRM1 = 8      # frame at n-1 .......
XXX1 = 9
YYY1 = 10
RAD1 = 11
LAP1 = 12
PRM1 = 13

FRM2 = 14
XXX2 = 15
YYY2 = 16
RAD2 = 17
LAP2 = 18
PRM2 = 19

DST01 = 20    # Distance from contours 0 to 1
DST12 = 21
DIR01 = 22    # Direction from contour 0 to 1
DIR12 = 23
SDST = 24     # Distance score
SDIR = 25     # Direction score
SRAD = 26     # Radius variance
SLAP = 27     # average Laplacian variance
SPRM = 28     # peimeter variance
ADST = 29     # averaged distance
ADIR = 30     # averaged direction
ARAD = 31     # average radius
EXXX = 32     # ellipse x center
EYYY = 33     # ellipse y center
EAAA = 34     # ellipse major axis
EBBB = 35     # ellipse minor axis
ETHT = 36     # ellipse angle theta
ISRH = 37     # ISR hit or miss
ELLX = 38     # x of target ellipse point
ELLY = 39     # y of target ellipse point
MAXD = 40     # max distance to edge of frame
STKS = 41     # number of potential hits on path to ellipse edge
SRAD = 42     # number of potential hits with similar radii
NDST = 43     # number of potential hits with similar speed
NDIR = 44     # number of potential hits with similar direction

def h5_frame_cutoff():
	# I only have data from this test video for the first 60k frames (40 minutes)
	inout = HF["raw"]
	inout = inout[np.where(inout[:, FRM0] < 60000)]
	#inout = inout[np.where(inout[:, FRM0] < 600)]
	if DEBUG:
		print("the first 60k frames:", np.shape(inout))
	HF.create_dataset("trunc", data=inout, compression="gzip", chunks=True, maxshape=(None, None))
	return

#def dtype_restrict(inout):
	#if np.max(inout[:, FRM0] > 65535):
		#print("the input frame value exceeds integer range!")
		#raise RuntimeError
	#oldbyte = inout.nbytes
	#inout = inout.astype('i4')
	#print("reduced floating point precision to uint32, array now takes up", inout.nbytes, "rather than", oldbyte, "bytes")
	#return inout

#def no_tiny_rad(inout, thresh=1):
	#"""
	#This function removes all contours from the input list which are smaller than a threshold.
	
	#:param inout: The input data set. Must be a Numpy array with shape (:, 4).
	#:param thresh: Radius threshold max (default=1)
	#:type inout: np.ndarray
	#:type thresh: int, float,...
	#:return: The output numpy array with shape (:, 4)
	#:rtype: np.ndarray
	#"""
	#inout = inout[np.where(inout[:, RAD0] > thresh)]
	#if DEBUG:
		#print("without ultrasmall contours:", np.shape(inout))
	#return inout

#def max_conts(inout):
	#"""
	#This function limits the number of contours which can appear in a single frame.  If the number of contours
	#in a frame is an outlier, the frame is rejected. Outliers are defined as :math: `\\overline{n} + (3\\,\\sigma_n)`.
	
	#:param inout: The input data set. Must be a Numpy array with shape (:, 4).
	#:type inout: np.ndarray
	#:return: The output numpy array with shape (:, 4)
	#:rtype: np.ndarray
	#"""
	## Find unique frames/counts of contours
	#unique, counts = np.unique(inout[:, FRM0], return_counts=True)
	## Remove outliers
	##maxarray = unique[np.where(counts <= (int(np.mean(counts) + (3*np.std(counts)))))]
	#maxarray = unique[np.where(counts <= 30)]
	#inout = inout[np.where(np.isin(inout[:, FRM0], maxarray))]
	#if DEBUG:
		#print("noise ceiling:", (int(np.mean(counts) + (1*np.std(counts)))))
		#print("noise ceiling shape:", np.shape(inout))
	#return inout

def h5_contour_density():
	inout = HF["trunc"]
	unique, counts = np.unique(inout[:, FRM0], return_counts=True)
	idx = np.searchsorted(unique, inout[:, FRM0])
	HF["trunc"].resize((HF["trunc"].shape[1] + 1), axis=1)
	HF["trunc"][:, -1:] = counts[idx][:, np.newaxis]
	if DEBUG:
		print("Post contour density shape:", HF["trunc"].shape)
	return

#def unique_pull(inout):
	#"""
	#This function performs a pairwise test for unique contour centers across all frames in the input data. If
	#an (x, y) center of a contour appears a large number of times such that it is an outlier (again using
	#:math: `\\overline{n} + (3\\,\\sigma_n)`), we remove all contours that occur at that center.
	
	#:param inout: The input data set. Must be a Numpy array with shape (:, 4).
	#:type inout: np.ndarray
	#:return: The output numpy array with shape (:, 4)
	#:rtype: np.ndarray
	#"""
	#if DEBUG:
		#max_cpp_frame = np.max(inout[:, FRM0])
		#print("cpp max frame reached:", max_cpp_frame)
	## Create test array of x, y points
	#test_arr = np.column_stack((inout[:, XXX0], inout[:, YYY0]))
	## find unique pairs
	#unique, count = np.unique(test_arr, return_counts=True, axis=0)
	## Remove outliers
	#unique = unique[np.where(count[:] < (np.mean(count) + (3 * np.std(count))))]
	## Void and ravel to compare the x,y points across the axis
	#void_dt = np.dtype((np.void, test_arr.dtype.itemsize * test_arr.shape[1]))
	#test_arr = test_arr.view(void_dt).ravel()
	#unique = unique.view(void_dt).ravel()
	#test_arr = np.isin(test_arr, unique)
	## Only keep the good stuff
	#inout = inout[test_arr]
	#if DEBUG:
		#print("newcpp shape", np.shape(inout))
		#print("newcpp median:", np.median(inout[:, RAD0]))
	#return inout

def h5_tag_and_remove_dupes():
	if DEBUG:
		print("tagging duplicates")
	inout = HF["trunc"]
	output = np.empty((0, np.shape(inout)[1]+1))
	for i in np.unique(inout[:, FRM0]):
		temp = inout[np.where(inout[:, FRM0] == i)]
		unique, counts = np.unique(temp[:, XXX0:YYY0+1], return_counts=True, axis=0)
		for j, k in zip(unique, counts):
			mmm = np.max(temp[np.all(temp[:, XXX0:YYY0+1] == j, axis=1)])
			nnn = temp[np.where(temp[:, RAD0] == np.max(temp[np.all(temp[:, XXX0:YYY0+1] == j, axis=1)][:, RAD0]))][:, LAP0:]
			output = np.vstack((output, np.hstack((np.hstack((i, j[0], j[1], mmm)), nnn[0], k))))
	HF.create_dataset("tag", data=output, compression="gzip", chunks=True, maxshape=(None, None))
	if DEBUG:
		print("post tagging shape:", HF["tag"].shape)
	return

#def remove_dupes(inout):
	#"""
	#This function removes duplicate contours from a single frame, keeping the one with the maximum radius.
	#OpenCV returns contours for both "inside" and "outside" a continuous shape.  Normally, the OpenCV code
	#would sanitize the input with contour hierarchy, but this is not practical for our case.  Using hierarchy
	#may do strange things to the contour of the moon, especially in frames where the camera is moving.  So,
	#we must use this work around.  This is a computationally intensive step for large datasets.
	
	#:param inout: The input data set. Must be a Numpy array with shape (:, 4).
	#:type inout: np.ndarray
	#:return: The output numpy array with shape (:, 4)
	#:rtype: np.ndarray
	#"""
	#if DEBUG:
		#print("removing duplicates")
	#indf = pd.DataFrame(inout)
	#outdf = indf.groupby([FRM0, XXX0, YYY0])[RAD0].max().reset_index().values
	#if DEBUG:
		#print("reduced from", inout.shape[0], "to", outdf.shape[0], "entries")
	#return outdf

def h5_concat_x_deg():
	"""
	This function concatenates contours across 3 frames (the frame of interest and the two previous ones).
	Since this function tiles and repeats all contours in nearby frames, it may potentially create a super
	large dataset.  It must only be called if the input has been sanitized to remove repeated and junk data.
	
	:param inout: The input data set. Must be a Numpy array with shape (:, 4).
	:type inout: np.ndarray
	:return: The output numpy array with shape (:, 12)
	:rtype: np.ndarray
	"""
	data = HF["tag"]
	unique, counts = np.unique(data[:, FRM0], return_counts=True)
	if DEBUG:
		print("Concat/tile data")
		print("Unique frames:", unique.shape)
	output = np.empty((0, 24))
	
	for i, j in enumerate(unique[:]):
		if not np.isin(j-1, unique[:]) and np.isin(j-2, unique[:]):
			continue
		temp1 = data[np.where(data[:, FRM0] == j)]
		temp2 = data[np.where(data[:, FRM0] == j-1)]
		temp1 = np.column_stack((np.repeat(temp1, np.size(temp2, 0), axis=0), np.tile(temp2, (np.size(temp1, 0), 1))))
		temp2 = data[np.where(data[:, FRM0] == j-2)]
		temp1 = np.column_stack((np.repeat(temp1, np.size(temp2, 0), axis=0), np.tile(temp2, (np.size(temp1, 0), 1))))
		output = np.vstack((output, temp1))
	#cnt = 0
	#oldcnt_1 = 1
	#oldcnt_2 = 1
	#cntcnt = 0
	#for i, j in enumerate(unique[:]):
		##print(cntcnt+oldcnt_1*oldcnt_2*counts[i])
		#if (cntcnt+(oldcnt_1*oldcnt_2*counts[i])) > 50000:
			#if DEBUG:
				#print("frame:", j)
			#if cnt > 0:
				#HF["tag"].resize((HF["tag"].shape[0] + output.shape[0]), axis=0)
				#HF["tag"][-output.shape[0]:] = output
				##np.savetxt("./data/temp-{:06d}.csv".format(cnt), output, delimiter=',', fmt='%i')
				#output = np.empty((0, 21))
			#oldcnt_1 = 1
			#oldcnt_2 = 1
			#cntcnt = 0
			#cnt += 1
		#if not np.isin(j-1, unique[:]) and np.isin(j-2, unique[:]):
			#continue
		#temp1 = data[np.where(data[:, FRM0] == j)]
		#temp2 = data[np.where(data[:, FRM0] == j-1)]
		#temp1 = np.column_stack((np.repeat(temp1, np.size(temp2, 0), axis=0), np.tile(temp2, (np.size(temp1, 0), 1))))
		#temp2 = data[np.where(data[:, FRM0] == j-2)]
		#temp1 = np.column_stack((np.repeat(temp1, np.size(temp2, 0), axis=0), np.tile(temp2, (np.size(temp1, 0), 1))))
		#output = np.vstack((output, temp1))
		#oldcnt_2 = oldcnt_1
		#oldcnt_1 = counts[i]
		#cntcnt += counts[i]
	output = np.column_stack((output[:, FRM0:PRM1+1], output[:, FRM2+2:FRM2+5+2+1]))
	HF.create_dataset("default", data=output, compression="gzip", chunks=True, maxshape=(None, None))
	if DEBUG:
		print("generated h5 file with shape:", HF["default"].shape)
		
	# Save output values:
	# n, x, y, r, meanL, cntL, n-1, x, y, r, meanL, cntL, n-2, x, y, r, meanL, cntL
	return






#def distance_calc(inout):
	## in size is [:, 12]
	## Distance formula for each item
	## First for n, n-1
	#inout = np.column_stack((inout, np.sqrt(np.square(np.subtract(inout[:, XXX0], inout[:, XXX1])) + np.square(np.subtract(inout[:, YYY0], inout[:, YYY1])))))
	## Do it again for n-1, n-2
	#inout = np.column_stack((inout, np.sqrt(np.square(np.subtract(inout[:, XXX1], inout[:, XXX2])) + np.square(np.subtract(inout[:, YYY1], inout[:, YYY2])))))
	## The value at dist passes a test, we treat the row as true, else false row.
	## This is broadcast back to our out
	##out = out[np.where((out[:, 8]/10 > smin) & (out[:, 8]/10 < smax), True, False)]
	#if DEBUG:
		#print("post distcalc size:", np.shape(inout))
	#return inout

#def direction_calc(inout):
	#if DEBUG:
		#print("calculating directions")
	## np.arctan directions:
	##                y 90
	##                |
	##       -45      |      45
	##                |
	##                |
	##  -0.0          |          0.0
	## -x ------------|----------- x
	##                |
	##                |
	##                |
	##       45       |     -45
	##                | -90
	##               -y
	##
	## Therefore, complete vertical must be 90
	## in size is [:, 14]
	## remove overlaps, the bird will never NOT move in either x or y
	#inout = inout[np.where(np.logical_and(inout[:, XXX0] != inout[:, XXX1], inout[:, YYY0] != inout[:, YYY1]))]
	#inout = inout[np.where(np.logical_and(inout[:, XXX1] != inout[:, XXX2], inout[:, YYY1] != inout[:, YYY2]))]
	
	#temp = np.column_stack((np.subtract(inout[:, YYY0], inout[:, YYY1]), np.subtract(inout[:, XXX0], inout[:, XXX1])))
	#temp2 = np.column_stack((np.subtract(inout[:, YYY1], inout[:, YYY2]), np.subtract(inout[:, XXX1], inout[:, XXX2])))
	## First for n, n-1
	#temp = np.column_stack((temp, np.where(temp[:, XXX0] == 0, np.where(temp[:, FRM0] > 0, 90, -90), np.arctan(np.divide(temp[:, FRM0], temp[:, XXX0]))*(180/np.pi))))
	## Do it again for n-1, n-2
	#temp2 = np.column_stack((temp2, np.where(temp2[:, XXX0] == 0, np.where(temp2[:, FRM0] > 0, 0.5*np.pi*(180/np.pi), -0.5*np.pi*(180/np.pi)), np.arctan(np.divide(temp2[:, FRM0], temp2[:, XXX0]))*(180/np.pi))))
	
	## Correct value for quadrants such that:
	##
	##                y 90
	##                |
	##       135      |      45
	##                |
	##                |
	##   180          |          0.0
	## -x ------------|----------- x
	##                |
	##                |
	##                |
	##       225      |     315
	##                | 270
	##               -y
	##
	#temp = np.column_stack((temp, np.where(temp[:, XXX0] < 0, 180 + temp[:, YYY0], np.where(temp[:, FRM0] < 0, 360 + temp[:, YYY0], temp[:, YYY0]))))
	#temp2 = np.column_stack((temp2, np.where(temp2[:, XXX0] < 0, 180 + temp2[:, YYY0], np.where(temp2[:, FRM0] < 0, 360 + temp2[:, YYY0], temp2[:, YYY0]))))
	
	## Place this in the input-output array
	#inout = np.column_stack((inout, temp[:, 3], temp2[:, 3]))
	
	#if DEBUG:
		#print("post dircalc size:", np.shape(inout))
	
	#if DEBUG:
		#print("correcting directional data for left and right orientation")
	## correct left and right side values
	#inout = np.column_stack((inout[:, :DIR01], np.where(inout[:, XXX0] > inout[:, XXX2], 180-inout[:, DIR01], inout[:, DIR01]), np.where(inout[:, XXX0] > inout[:, XXX2], 180-inout[:, DIR12], inout[:, DIR12]), inout[:, (DIR12+1):]))
	#if DEBUG:
		#print("size after removing conflicting directions:", np.shape(inout))
	#return inout

#def remove_zero_speed(inout):
	#inout = inout[np.where(inout[:, DST01] != 0)]
	#inout = inout[np.where(inout[:, DST12] != 0)]
	#if DEBUG:
		#print("removed zero speed instances, size now:", np.shape(inout))
	#return inout

#def score_dist(inout):
	#inout = np.column_stack((inout, np.divide(np.amax(inout[:, DST01:DST12+1]), np.abs(np.subtract(inout[:, DST01], inout[:, DST12])+SPEA))))
	#if DEBUG:
		#print("post dist score size:", np.shape(inout))
	#return inout

#def score_dir(inout):
	#inout = np.column_stack((inout, np.divide(np.amax(inout[:, DIR01:DIR12+1]), np.abs(np.subtract(inout[:, DIR01], inout[:, DIR12])+ANGA))))
	#if DEBUG:
		#print("post dir score size:", np.shape(inout))
	#return inout

#def score_rad(inout):
	## 3, 10, 17
	#inout = np.column_stack((inout, np.var(np.column_stack((inout[:, RAD0], inout[:, RAD1], inout[:, RAD2])), axis=1)))
	#if DEBUG:
		#print("post rad score size:", np.shape(inout))
	#return inout

#def score_mean_lap(inout):
	#inout = np.column_stack((inout, np.var(np.column_stack((inout[:, LAP0], inout[:, LAP1], inout[:, LAP2])), axis=1)))
	#if DEBUG:
		#print("post lap score size:", np.shape(inout))
	#return inout

#def score_perim(inout):
	#inout = np.column_stack((inout, np.var(np.column_stack((inout[:, PRM0], inout[:, PRM1], inout[:, PRM2])), axis=1)))
	#if DEBUG:
		#print("post perim score size:", np.shape(inout))
	#return inout

def test_isclose(inout):
	# remove back and forth
	inout = inout[np.isclose(inout[:, DIR01], inout[:, DIR12], rtol=ANGR, atol=ANGA)]
	# keep only similar distances
	inout = inout[np.isclose(inout[:, DST01], inout[:, DST12], rtol=SPER, atol=SPEA)]
	if DEBUG:
		print("closeness tests reduced to size:", inout.shape)
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
	indata = indata[np.where(np.isin(indata[:, FRM0], israrr[:]))]
	if DEBUG:
		print("newcpp shape of isr frames", np.shape(indata))
	return

def label_isr(inout, israrr):
	inout = np.column_stack((inout, np.where(np.isin(inout[:, FRM0], israrr), 1, 0)))
	return inout

def isr_misses(inout, israrr):
	inout = inout[np.where(np.isin(inout[:, FRM0], israrr))]
	if DEBUG:
		print("ISR MISS:", np.shape(inout))
	return inout

def birds_found_not_found(inout):
	cnt = 0
	israrr = np.loadtxt(data_isr, delimiter=',')
	for i in israrr[:]:
		temp = np.arange(int(i[0]), int(i[1])+1)
		if (np.sum(np.isin(temp[:], inout[:, FRM0])) == 0):
			cnt += 1
			if DEBUG:
				print("missing track: ", temp)
	if DEBUG:
		print("missed", cnt, "of", israrr.shape[0], "bird contour traces")
	return



	#HF["trunc"].resize((HF["trunc"].shape[1] + 1), axis=1)
	#HF["trunc"][:, -1:] = counts[idx][:, np.newaxis]
def h5_dirdist_test():
	if DEBUG:
		print("input shape=", HF["default"].shape)
	# Get distance
	# in size is [:, 12]
	# Distance formula for each item
	# First for n, n-1
	inout = HF["default"]
	output = np.sqrt(np.square(np.subtract(inout[:, XXX0], inout[:, XXX1])) + np.square(np.subtract(inout[:, YYY0], inout[:, YYY1])))
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = output[:, np.newaxis]
	# Do it again for n-1, n-2
	output = np.sqrt(np.square(np.subtract(inout[:, XXX1], inout[:, XXX2])) + np.square(np.subtract(inout[:, YYY1], inout[:, YYY2])))
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = output[:, np.newaxis]
	# The value at dist passes a test, we treat the row as true, else false row.
	# This is broadcast back to our out
	#out = out[np.where((out[:, 8]/10 > smin) & (out[:, 8]/10 < smax), True, False)]
	if DEBUG:
		print("post distcalc size:", np.shape(inout))
	# new format: (n) dist1 dist2
	# Get direction
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
	inout = HF["default"]
	HF["temp"] = inout[np.where(np.logical_and(inout[:, XXX0] != inout[:, XXX1], inout[:, YYY0] != inout[:, YYY1]))]
	del HF["default"]
	HF["default"] = HF["temp"]
	del HF["temp"]
	inout = HF["default"]
	HF["temp"] = inout[np.where(np.logical_and(inout[:, XXX1] != inout[:, XXX2], inout[:, YYY1] != inout[:, YYY2]))]
	del HF["default"]
	HF.create_dataset("default", data=HF["temp"], compression="gzip", chunks=True, maxshape=(None, None))
	del HF["temp"]
	
	inout = HF["default"]
	HF.create_dataset("temp", data=np.column_stack((HF["default"][:, FRM0], HF["default"][:, FRM0])), compression="gzip", chunks=True, maxshape=(None, None))
	HF["temp"].resize((HF["temp"].shape[1] + 1), axis=1)
	HF["temp"][:, -1:] = np.subtract(inout[:, YYY0], inout[:, YYY1])[:, np.newaxis]
	HF["temp"].resize((HF["temp"].shape[1] + 1), axis=1)
	HF["temp"][:, -1:] = np.subtract(inout[:, XXX0], inout[:, XXX1])[:, np.newaxis]
	HF["temp"].resize((HF["temp"].shape[1] + 1), axis=1)
	HF["temp"][:, -1:] = np.subtract(inout[:, YYY1], inout[:, YYY2])[:, np.newaxis]
	HF["temp"].resize((HF["temp"].shape[1] + 1), axis=1)
	HF["temp"][:, -1:] = np.subtract(inout[:, XXX1], inout[:, XXX2])[:, np.newaxis]
	HF["temp"].resize((HF["temp"].shape[1] + 1), axis=1)
	
	
	
	HF["temp"][:, -1:] = np.where(HF["temp"][:, 3] == 0, np.where(HF["temp"][:, 2] > 0, 90, -90), np.arctan(np.divide(HF["temp"][:, 2], HF["temp"][:, 3]))*(180/np.pi))[:, np.newaxis]
	HF["temp"].resize((HF["temp"].shape[1] + 1), axis=1)
	HF["temp"][:, -1:] = np.where(HF["temp"][:, 3] == 0, np.where(HF["temp"][:, 2] > 0, 0.5*np.pi*(180/np.pi), -0.5*np.pi*(180/np.pi)), np.arctan(np.divide(HF["temp"][:, 2], HF["temp"][:, 3]))*(180/np.pi))[:, np.newaxis]
	
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = HF["temp"][:, 5][:, np.newaxis]
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = HF["temp"][:, 6][:, np.newaxis]

	del HF["temp"]
	
	inout = HF["default"]
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = np.subtract(inout[:, XXX0], inout[:, XXX1])[:, np.newaxis]
	
	inout = HF["default"]
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = np.where(inout[:, XXX0] == 0, np.where(inout[:, FRM0] > 0, 90, -90), np.arctan(np.divide(inout[:, FRM0], inout[:, XXX0]))*(180/np.pi))[:, np.newaxis]
	
	inout = HF["default"]
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = np.subtract(inout[:, YYY1], inout[:, YYY2])[:, np.newaxis]
	
	inout = HF["default"]
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = np.subtract(inout[:, XXX1], inout[:, XXX2])[:, np.newaxis]
	
	inout = HF["default"]
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = np.where(inout[:, XXX0] == 0, np.where(inout[:, FRM0] > 0, 0.5*np.pi*(180/np.pi), -0.5*np.pi*(180/np.pi)), np.arctan(np.divide(inout[:, FRM0], inout[:, XXX0]))*(180/np.pi))[:, np.newaxis]
	
	
	if DEBUG:
		print("post dircalc size:", np.shape(inout))
	
	if DEBUG:
		print("correcting directional data for left and right orientation")
	
	# correct left and right side values
	inout = HF["default"]
	inout = np.column_stack((inout[:, :DIR01], np.where(inout[:, XXX0] > inout[:, XXX2], 180-inout[:, DIR01], inout[:, DIR01]), np.where(inout[:, XXX0] > inout[:, XXX2], 180-inout[:, DIR12], inout[:, DIR12]), inout[:, (DIR12+1):]))
	if DEBUG:
		print("size after removing conflicting directions:", np.shape(inout))
	
	# new format: (n) (dist) dir1 dir2
	# remove zero speed
	inout = inout[np.where(inout[:, DST01] != 0)]
	inout = inout[np.where(inout[:, DST12] != 0)]
	HF.create_dataset("temp", data=inout, compression="gzip", chunks=True, maxshape=(None, None))
	del HF["default"]
	HF["default"] = HF["temp"]
	del HF["temp"]
	if DEBUG:
		print("removed zero speed instances, size now:", np.shape(inout))
	
	# Score distance
	inout = HF["default"]
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = np.divide(np.amax(inout[:, DST01:DST12+1]), np.abs(np.subtract(inout[:, DST01], inout[:, DST12])+SPEA))[:, np.newaxis]
	if DEBUG:
		print("post dist score size:", np.shape(inout))
	
	# Score dir
	inout = HF["default"]
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = np.divide(np.amax(inout[:, DIR01:DIR12+1]), np.abs(np.subtract(inout[:, DIR01], inout[:, DIR12])+ANGA))[:, np.newaxis]
	if DEBUG:
		print("post dir score size:", np.shape(inout))
	
	# Score rad
	# 3, 10, 17
	inout = HF["default"]
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = np.var(np.column_stack((inout[:, RAD0], inout[:, RAD1], inout[:, RAD2])), axis=1)[:, np.newaxis]
	if DEBUG:
		print("post rad score size:", np.shape(inout))
	
	# Score mean Lap
	inout = HF["default"]
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = np.var(np.column_stack((inout[:, LAP0], inout[:, LAP1], inout[:, LAP2])), axis=1)[:, np.newaxis]
	if DEBUG:
		print("post lap score size:", np.shape(inout))
	
	# Score perim
	inout = HF["default"]
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = np.var(np.column_stack((inout[:, PRM0], inout[:, PRM1], inout[:, PRM2])), axis=1)[:, np.newaxis]
	if DEBUG:
		print("post perim score size:", np.shape(inout))
	return inout

def h5_avg_speed_dir_rad():
	inout = HF["default"]
	# average the distance and direction calcs in column ADST ADIR
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = np.mean(inout[:, DST01:DST12+1], axis=1)[:, np.newaxis]
	
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = np.mean(inout[:, DIR01:DIR12+1], axis=1)[:, np.newaxis]
	# remove very slow contours
	inout = HF["default"]
	inout = inout[np.where(inout[:, ADST] > 10)]
	# change average angle to radians vector angle
	HF["default"][:, ADIR] = HF["default"][:, ADIR]*(np.pi/180)
	# average radii
	inout = HF["default"]
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = np.average(np.column_stack((inout[:, RAD0], inout[:, RAD1], inout[:, RAD2])), axis=1)[:, np.newaxis]
	if DEBUG:
		print("averages, new size:", HF["default"].shape)
	return


def h5_fetch_ellipse():
	# get moon ellipse size
	circ = np.loadtxt(data_ell, delimiter=',')
	# "center" the moon by replacing the x y center
	circ = np.column_stack((circ[:, 0], np.repeat(960, np.size(circ, 0), axis=0), np.repeat(540, np.size(circ, 0), axis=0), circ[:, 3:]))
	# combine this with our inout data (adds ellx, elly, ellaxx, ellaxy, ellang)
	inout = HF["default"]
	inout = np.hstack([inout, circ[inout[:, FRM0].astype(int), 1:]])
	return


def h5_label_isr(israrr):
	inout = HF["default"]
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = np.where(np.isin(inout[:, FRM0], israrr), 1, 0)[:, np.newaxis]
	return


def h5_test_isclose():
	inout = HF["default"]
	# remove back and forth
	HF["temp"] = inout[np.tile(np.isclose(inout[:, DIR01], inout[:, DIR12], rtol=ANGR, atol=ANGA)[:, np.newaxis], inout.shape[1])].reshape(-1, inout.shape[1])
	del HF["default"]
	HF["default"] = HF["temp"]
	del HF["temp"]
	# keep only similar distances
	inout = HF["default"]
	HF["temp"] = inout[np.tile(np.isclose(inout[:, DST01], inout[:, DST12], rtol=SPER, atol=SPEA)[:, np.newaxis], inout.shape[1])].reshape(-1, inout.shape[1])
	del HF["default"]
	HF["default"] = HF["temp"]
	del HF["temp"]
	if DEBUG:
		print("closeness tests reduced to size:", HF["default"].shape)
	return





#def filewise_dirdist_test(inout):
	#if DEBUG:
		#print("type:", inout.dtype)
		#print("input shape=", inout.shape)
	## Get distance
	#inout = distance_calc(inout)
	## new format: (n) dist1 dist2
	## Get direction
	#inout = direction_calc(inout)
	## new format: (n) (dist) dir1 dir2
	#inout = remove_zero_speed(inout)
	#inout = score_dist(inout)
	#inout = score_dir(inout)
	#inout = score_rad(inout)
	#inout = score_mean_lap(inout)
	#inout = score_perim(inout)
	#if DEBUG:
		#print("output shape = ", inout.shape)
	#return inout

#def avg_speed_dir_rad(inout):
	## average the distance and direction calcs in column 17, 18
	#inout = np.column_stack((inout[:], np.mean(inout[:, DST01:DST12+1], axis=1), np.mean(inout[:, DIR01:DIR12+1], axis=1)))
	## remove very slow contours
	#inout = inout[np.where(inout[:, ADST] > 10)]
	## change average angle to radians vector angle
	#inout = np.column_stack((inout[:, :ADIR], inout[:, ADIR]*(np.pi/180)))
	## average radii
	#inout = np.column_stack((inout, np.average(np.column_stack((inout[:, RAD0], inout[:, RAD1], inout[:, RAD2])), axis=1)))
	#return inout


#def fetch_ellipse(inout):
	## get moon ellipse size
	#circ = np.loadtxt(data_ell, delimiter=',')
	## "center" the moon by replacing the x y center
	#circ = np.column_stack((circ[:, 0], np.repeat(960, np.size(circ, 0), axis=0), np.repeat(540, np.size(circ, 0), axis=0), circ[:, 3:]))
	## combine this with our inout data (adds ellx, elly, ellaxx, ellaxy, ellang)
	#inout = np.hstack([inout, circ[inout[:, FRM0].astype(int), 1:]])
	#return inout

def h5_attach_links():
	HF["temp"] = HF["default"]
	del HF["default"]
	HF.create_dataset("default", data=HF["temp"], compression="gzip", chunks=True, maxshape=(None, None))
	del HF["temp"]
	inout = HF["default"]
	if DEBUG:
		print("attach_links input shape:", inout.shape)
	
	# SCARY MATH
	#-(a*b*sqrt(((-m^2)+2*j*m-j^2+a^2)*tan(t)^2+((2*m-2*j)*n-2*k*m+2*j*k)*tan(t)-n^2+2*k*n-k^2+b^2)-a^2*j*tan(t)^2+(a^2*k-a^2*n)*tan(t)-b^2*m)/(a^2*tan(t)^2+b^2)
	#or
	#(a*b*sqrt(((-m^2)+2*j*m-j^2+a^2)*tan(t)^2+((2*m-2*j)*n-2*k*m+2*j*k)*tan(t)-n^2+2*k*n-k^2+b^2)+a^2*j*tan(t)^2+(a^2*n-a^2*k)*tan(t)+b^2*m)/(a^2*tan(t)^2+b^2)
	
	# calculate endpoint xy of vector on ellipse
	matharr = -(inout[:, EAAA]/2*inout[:, EBBB]*np.sqrt(((-inout[:, EXXX]**2)+2*inout[:, XXX0]*inout[:, EXXX]-inout[:, XXX0]**2+inout[:, EAAA]**2)*np.tan(inout[:, ADIR])**2+((2*inout[:, EXXX]-2*inout[:, XXX0])*inout[:, EYYY]-2*inout[:, YYY0]*inout[:, EXXX]+2*inout[:, XXX0]*inout[:, YYY0])*np.tan(inout[:, ADIR])-inout[:, EYYY]**2+2*inout[:, YYY0]*inout[:, EYYY]-inout[:, YYY0]**2+inout[:, EBBB]**2)-inout[:, EAAA]**2*inout[:, XXX0]*np.tan(inout[:, ADIR])**2+(inout[:, EAAA]**2*inout[:, YYY0]-inout[:, EAAA]**2*inout[:, EYYY])*np.tan(inout[:, ADIR])-inout[:, EBBB]**2*inout[:, EXXX])/(inout[:, EAAA]**2*np.tan(inout[:, ADIR])**2+inout[:, EBBB]**2)
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = matharr[:, np.newaxis]
	## Y value of the same
	inout = HF["default"]
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = (np.tan(inout[:, ADIR])*matharr[:]+inout[:, YYY0]-np.tan(inout[:, ADIR])*inout[:, XXX0])[:, np.newaxis]
	# stacking adds elltargetx, elltargety; switch back to int
	# calculate max frames away the final contour could be away from this one
	# A good version
	#inout = np.column_stack((inout, np.divide(np.sqrt(np.square(np.subtract(inout[:, XXX0], inout[:, ELLX]))+np.square(np.subtract(inout[:, YYY0], inout[:, ELLY]))), inout[:, ADST]).astype(int)+2))
	# A cheap and easy version
	inout = HF["default"]
	HF["default"].resize((HF["default"].shape[1] + 1), axis=1)
	HF["default"][:, -1:] = np.repeat(3, inout.shape[0])[:, np.newaxis]
	if DEBUG:
		print("new shape with math arrays", inout[0])
	# Stack the maximum distance
	#potstack = np.empty((0, 4))
	inout = HF["default"]
	HF.create_dataset("potstack", shape=(0, 4), compression="gzip", chunks=True, maxshape=(None, None))
	for i in inout:
		potential = inout[np.where(np.logical_and(inout[:, FRM0] > i[FRM0], inout[:, FRM0] < i[MAXD]+i[FRM0]+1))]
		nearrad = potential[np.isclose(potential[:, ARAD], i[ARAD], atol=RADA, rtol=RADR)].shape[0]
		neardst = potential[np.isclose(potential[:, ADST], i[ADST], atol=SPEA, rtol=SPER)].shape[0]
		neardir = potential[np.isclose(potential[:, ADIR], i[ADIR], atol=ANGA, rtol=ANGR)].shape[0]
		HF["potstack"].resize((HF["potstack"].shape[0] + 1), axis=0)
		HF["potstack"][-1:] = np.column_stack((potential.shape[0], nearrad, neardst, neardir))
	HF["default"].resize((HF["default"].shape[1] + 4), axis=1)
	HF["default"][:, -4:] = HF["potstack"]
	return


def draw_boxes_on_frames(inout):
	import cv2
	import glob
	if not glob.glob("./data/frames_labeled/"):
		os.mkdir("./data/frames_labeled")
	
	for i in inout:
		futurebirds = inout[np.where(np.logical_and(inout[:, FRM0] > i[0], inout[:, FRM0] < (i[26]+i[0])))]
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
	ourframes = np.unique(np.hstack((np.hstack((inout[:, FRM0], inout[:, 4])), inout[:, 8])))
	truthvals = np.empty((0))
	for i in isrframes:
		temp = np.arange(i[0], i[1]+1)
		truthvals = np.hstack((truthvals, np.isin(temp, ourframes)))
	hitpercent = np.sum(truthvals)/np.shape(truthvals)[0]
	misspercent = (np.shape(truthvals)[0] - np.sum(truthvals))/np.shape(truthvals)[0]
	print("This code detected", hitpercent*100, "%% of the birds in ISR")
	return

def main():
	# Gen array of frames found by ISR
	israrr = isr_frames()
	
	# Only the first 60k frames
	h5_frame_cutoff()

	# Count the number of contours on this frame and store in column
	h5_contour_density()
	
	# remove duplicate contours
	#out_cat = remove_dupes(out_cat)
	h5_tag_and_remove_dupes()
	
	###Save Point
	#np.savetxt(data_out, out_cat, delimiter=',', fmt='%i')
	
	## Store this entire list in an alt spot for later
	#alt_cat = out_cat
	
	# expand data for nearby contours in time
	h5_concat_x_deg()
	
	h5_dirdist_test()
	h5_avg_speed_dir_rad()
	h5_fetch_ellipse()
	h5_label_isr(israrr)
	h5_test_isclose()
	
	#for fff in sorted(os.listdir("./data")):
		#if re.match(r"temp-[0-9]{6}\.csv", fff):
			#out_cat = np.loadtxt("./data/"+fff, delimiter=',').astype('i4')
			#if DEBUG:
				#print("testing temp files and concat")
			#out_cat = filewise_dirdist_test(out_cat)
			#if DEBUG:
				#print("averaging speed and direction in short linear sets")
			#out_cat = avg_speed_dir_rad(out_cat)
			#if DEBUG:
				#print("fetching ellipse data from:", data_ell)
			#out_cat = fetch_ellipse(out_cat)
			#if DEBUG:
				#print("labeling hits")
			#out_cat = label_isr(out_cat, israrr)
			#if DEBUG:
				#print("Testing for closeness")
			#out_cat = test_isclose(out_cat)
			#if DEBUG:
				#print("saving", fff)
			## Replace infinite values with max 16 bit signed int value
			#out_cat[out_cat[:, :] == np.inf] = 32767
			## Replace nan with min 16 bit signed int value
			#out_cat[out_cat[:, :] == np.nan] = -32768
			#try:
				#np.savetxt("./data/"+fff, out_cat, delimiter=',', fmt='%i')
			#except ValueError:
				#np.savetxt("./data/CRUD"+fff, out_cat, delimiter=',')
	
	#fulldata = np.empty((0, ISRH+1))
	#try:
		#for fff in sorted(os.listdir("./data")):
			#if re.match(r"temp-[0-9]{6}\.csv", fff):
				#out_cat = np.loadtxt("./data/"+fff, delimiter=',').astype('i4')
				#print(np.shape(out_cat))
				#fulldata = np.vstack((fulldata, out_cat))
				#print("stacked file:", fff, "into fulldata")
		#np.savetxt("./data/testout.csv", fulldata, delimiter=',', fmt='%i')
		##outscore = attach_links(fulldata)
	#except MemoryError:
		#print("ran out of memory")
		#quit()

	h5_attach_links()
	#np.savetxt("./data/testout.csv", outscore, delimiter=',', fmt='%i')
	
	## Save new results
	#
	
	#if USE_CV:
		#draw_boxes_on_frames(outscore)
	
	## ISR hit or miss?
	#isr_percent(outscore)
	
	
	
	#hits = outscore[np.isin(outscore[:, FRM0], israrr)]
	#miss = outscore[np.invert(np.isin(outscore[:, FRM0], israrr))]
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
	##hits = goodscore[:, 1]
	##miss = badscore[:, 1]
	#bins = np.linspace(0, 20, 50)
	## Plot of how the major and minor axes of the ellipse changed in relation to number of hits.

	#ax1 = plt.subplot(111)
	##plt.hist([hits, miss], bins, label=lab , color=col)
	#plt.scatter(miss[:, FRM0], miss[:, 27], c=col[1], marker='o', s=2, alpha=0.5, label="Misses")
	#plt.scatter(hits[:, FRM0], hits[:, 27], c=col[0], marker='o', s=2, alpha=0.5, label="Hits")
	#axes = plt.gca()
	##axes.set_ylim([0,100])
	#plt.xlabel("Frame Number")
	#plt.ylabel("Score")
	#plt.grid(True)
	#plt.show()
	
	
	
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
	
	
	
	
	# Perform LDA analysis
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	

def cleanup_temp_files():
	for f in sorted(os.listdir("./data")):
		if re.match(r"temp-[0-9]{6}\.csv", f):
			os.remove("./data/"+f)

if __name__ == '__main__':
	try:
		main()
	finally:
		#cleanup_temp_files()
		HF.close()
		pass
