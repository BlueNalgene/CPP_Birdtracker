"""
This script concats the individual tiered data from frame_extraction into a single file
"""


import argparse
import configparser
import numpy as np

def parse_settings():
	'''
	Parser allows for custom settings file locations
	returns settings string
	'''
	parser = argparse.ArgumentParser()
	parser.add_argument("-c", "--config", action="store_true", default="settings.cfg", \
		help="config file location")
	args = parser.parse_args()
	settings_location = args.config
	return settings_location

def main():
	'''
	Executes concatination script
	'''
	# The config parser requires sections which aren't compatible with the C++ cfg file.
	# We insert a dummy section to bypass this restriction
	with open(parse_settings()) as fff:
		file_content = '[dummy_section]\n' + fff.read()
	config_parser = configparser.RawConfigParser()
	config_parser.read_string(file_content)
	# Fetch output directory based on config
	outdir = config_parser["dummy_section"]["outputdir"]
	# Construct path to data files and output
	tier_1 = "." + outdir + "data/Tier1.csv"
	tier_2 = "." + outdir + "data/Tier2.csv"
	tier_3 = "." + outdir + "data/Tier3.csv"
	tier_4 = "." + outdir + "data/Tier4.csv"
	output = "." + outdir + "data/mixed_tiers.csv"
	# Open relevant files
	frame_t1 = np.loadtxt(tier_1, delimiter=',', skiprows=1)
	frame_t2 = np.loadtxt(tier_2, delimiter=',', skiprows=1)
	frame_t3 = np.loadtxt(tier_3, delimiter=',', skiprows=1)
	frame_t4 = np.loadtxt(tier_4, delimiter=',', skiprows=1)
	# Concat and label with tier number
	frame_t1 = np.column_stack((frame_t1, np.full(np.shape(frame_t1)[0], 1)))
	frame_t2 = np.column_stack((frame_t2, np.full(np.shape(frame_t2)[0], 2)))
	frame_t3 = np.column_stack((frame_t3, np.full(np.shape(frame_t3)[0], 3)))
	frame_t4 = np.column_stack((frame_t4, np.full(np.shape(frame_t4)[0], 4)))
	# Stack the tiers
	frame_all = np.vstack((frame_t1, frame_t2, frame_t3, frame_t4))
	# Sort based on the frame number
	frame_all = frame_all[frame_all[:, 0].argsort()]
	# Save
	np.savetxt(output, frame_all, delimiter=',', fmt=['%d', '%f', '%f', '%f', '%d'])
	return

main()
