#!/usr/bin/env python3

import os
import filecmp
import time

# Define the paths of the two binary files
file1_path = "server"
file2_path = "server"

# Check if both files exist
if not (os.path.exists(file1_path) and os.path.exists(file2_path)):
    print("One or both files do not exist.")
else:
    # Get the current modification times of the files
    file1_mtime = os.path.getmtime(file1_path)
    file2_mtime = os.path.getmtime(file2_path)

    # Calculate the time difference in seconds
    time_difference = abs(file1_mtime - file2_mtime)

    # Check if the time difference is less than or equal to 60 seconds (1 minute)
    if time_difference <= 60:
        print("The two binary files are the same or were modified within the last minute.")
    else:
        print("The two binary files are different or were not modified within the last minute.")
