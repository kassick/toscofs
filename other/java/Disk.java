// 
// File: "Disk.java"
// Created: "Qui, 12 Nov 2015 15:03:58 -0200 (kassick)"
// Updated: "Sex, 20 Nov 2015 21:43:44 -0200 (kassick)"
// $Id$
// Copyright (C) 2015, Rodrigo Virote Kassick <rvkassick@inf.ufrgs.br> 

import java.io.*;
import java.util.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.ByteBuffer;
import java.nio.file.FileSystems;
import java.nio.file.FileSystem;
import java.nio.file.StandardOpenOption;

public class Disk {

    public static final int SECTOR_SIZE = 4096;

    int size;
    String backing_fname;
    FileChannel ch;

    public Disk(String backing_fname, int size) throws IOException, FileNotFoundException
    {
        this.size = size;
        this.backing_fname = backing_fname;

        java.nio.file.FileSystem fs = FileSystems.getDefault();

        ch = FileChannel.open(fs.getPath(backing_fname),
                               EnumSet.of(StandardOpenOption.CREATE,
                                          StandardOpenOption.READ,
                                          StandardOpenOption.WRITE));
    }

    public int write_sector(int sector, byte[] data) {
        if (data.length != SECTOR_SIZE) {
            System.out.println("Sector must by 512 int");
            return -1;
        }

        int offset = sector * SECTOR_SIZE;
        try {
            ch.position(offset);
            ch.write(ByteBuffer.wrap(data));
        } catch (IOException e) {
            return -1;
        }

        return 0;
    }
    
    
    public int read_sector(int sector, byte[] data) throws IOException {
        if (data.length != SECTOR_SIZE) {
            System.out.println("Sector must by 512 long");
            return -1;
        }

        for (int i = 0; i < SECTOR_SIZE; i++)
            data[i] = 0;

        int offset = sector * SECTOR_SIZE;
        try {
            ch.position(offset);
            ch.read(ByteBuffer.wrap(data));
        } catch (IOException e) {
            return -1;
        }
        
        return 0;
    }
}
