# Makefile
# File: "Makefile"
# Created: "Sex, 20 Nov 2015 16:54:44 -0200 (kassick)"
# Updated: "2018-08-09 16:03:29 kassick"
# $Id$
# Copyright (C) 2015, Rodrigo Virote Kassick <kassick@gmail.com>

CFLAGS=-g
LDFLAGS=-g
LD=gcc
TOSCO_DEPS=tosco_fs.o disk.o bitmap.o

all: tfs_create_files tfs_test_read_again bitmap_ut

bitmap_ut: bitmap.c bitmap.h
	$(CC) -DBM_UNIT_TEST bitmap.c -o $@

tfs_create_files: tfs_create_files.o $(TOSCO_DEPS)
	$(LD) $(LDFLAGS) tfs_create_files.o $(TOSCO_DEPS) -o $@

tfs_test_read_again: tfs_test_read_again.o $(TOSCO_DEPS)
	$(LD) $(LDFLAGS) tfs_test_read_again.o $(TOSCO_DEPS) -o $@

.PHONY: clean

clean:
	rm *.o
	rm tfs_create_files tfs_test_read_again bitmap_ut
