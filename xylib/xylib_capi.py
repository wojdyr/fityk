#!/usr/bin/env python

"""
ctypes-based Python interface to C API of xylib
"""

from ctypes import cdll, c_char_p, c_double

xylib = cdll.LoadLibrary("libxy.so.1")

get_version = xylib.xylib_get_version
get_version.restype = c_char_p

load_file = xylib.xylib_load_file

get_block = xylib.xylib_get_block

count_columns = xylib.xylib_count_columns

count_rows = xylib.xylib_count_rows

get_data = xylib.xylib_get_data
get_data.restype = c_double

dataset_metadata = xylib.xylib_dataset_metadata
dataset_metadata.restype = c_char_p

block_metadata = xylib.xylib_block_metadata
block_metadata.restype = c_char_p

free_dataset = xylib.xylib_free_dataset


if __name__ == '__main__':
    import sys

    print "xylib version:", get_version()

    filename = (len(sys.argv) > 1 and sys.argv[1] or "BT86.raw")
    dataset = load_file(filename, None)
    block = get_block(dataset, 0)

    ncol = count_columns(block)
    print "number of columns:", ncol
    print "number of rows (-1 means it's a generator):",
    for i in range(ncol):
        print count_rows(block, i+1),
    print

    print "data: ",
    n = min(count_rows(block, 2), 20)
    for i in range(n):
        print "(%g, %g) " % (get_data(block, 1, i), get_data(block, 2, i)),
    print "..."

    print "measured at:", dataset_metadata(dataset, "MEASURE_DATE")
    print "lambda:", block_metadata(block, "USED_LAMBDA")

    free_dataset(dataset)

