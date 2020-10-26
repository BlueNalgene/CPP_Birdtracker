# Eli headers
#f	x	y	pix	wt	md	dim	edge
# frame
# x coord
# y coord
# pix (radius?)
# wt (tier)
# md (moon diameter)
# dim (box dimensions)
# edge (is the moon on an edge?)

import configparser
import numpy as np

with open("settings.cfg") as f:
    file_content = '[dummy_section]\n' + f.read()

config_parser = configparser.RawConfigParser()
config_parser.read_string(file_content)

OUTDIR = config_parser["dummy_section"]["outputdir"]

ELLS = "." + OUTDIR + "data/ellipses.csv"
TIER_1 = "." + OUTDIR + "data/Tier1.csv"
TIER_2 = "." + OUTDIR + "data/Tier2.csv"
TIER_3 = "." + OUTDIR + "data/Tier3.csv"
TIER_4 = "." + OUTDIR + "data/Tier4.csv"
OUTPUT = "." + OUTDIR + "data/mixed_tiers.csv"

frame_ells = np.loadtxt(ELLS, delimiter=',', skiprows=1)
frame_t1 = np.loadtxt(TIER_1, delimiter=',', skiprows=1)
frame_t2 = np.loadtxt(TIER_2, delimiter=',', skiprows=1)
frame_t3 = np.loadtxt(TIER_3, delimiter=',', skiprows=1)
frame_t4 = np.loadtxt(TIER_4, delimiter=',', skiprows=1)

frame_t1 = np.column_stack((frame_t1, \
	np.full(np.shape(frame_t1)[0], 1)))
	#np.full(np.shape(frame_t1)[0], 1), \
		#(frame_ells[np.searchsorted(frame_ells[:, 0], frame_t1[:, 0]), 6]*2)))
frame_t2 = np.column_stack((frame_t2, \
	np.full(np.shape(frame_t2)[0], 2)))
	#np.full(np.shape(frame_t2)[0], 2), \
		#(frame_ells[np.searchsorted(frame_ells[:, 0], frame_t2[:, 0]), 6]*2)))
frame_t3 = np.column_stack((frame_t3, \
	np.full(np.shape(frame_t3)[0], 3)))
	#np.full(np.shape(frame_t3)[0], 3), \
		#(frame_ells[np.searchsorted(frame_ells[:, 0], frame_t3[:, 0]), 6]*2)))
frame_t4 = np.column_stack((frame_t4, \
	np.full(np.shape(frame_t4)[0], 4)))
	#np.full(np.shape(frame_t4)[0], 4), \
		#(frame_ells[np.searchsorted(frame_ells[:, 0], frame_t4[:, 0]), 6]*2)))

frame_all = np.vstack((frame_t1, frame_t3, frame_t4))
frame_all = frame_all[frame_all[:, 0].argsort()]

np.savetxt(OUTPUT, frame_all, delimiter=',', fmt=['%d', '%f', '%f', '%f', '%d'])
