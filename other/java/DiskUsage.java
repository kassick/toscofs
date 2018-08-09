// 
// File: "DiskUsage.java"
// Created: "Qui, 12 Nov 2015 15:24:49 -0200 (kassick)"
// Updated: "Sex, 20 Nov 2015 21:44:46 -0200 (kassick)"
// $Id$
// Copyright (C) 2015, Rodrigo Virote Kassick <rvkassick@inf.ufrgs.br> 

public class DiskUsage {

    public static byte[] strToByte(String s) {
        byte[] strbytes = s.getBytes();
        if (strbytes.length > Disk.SECTOR_SIZE) {
            System.out.println("Too long a string for this test");
            return null;
        }

        byte[] sector_bytes = new byte[Disk.SECTOR_SIZE];
        int i;
        for (i = 0; i < strbytes.length; i++) {
            sector_bytes[i] = strbytes[i];
        }

        for(; i < Disk.SECTOR_SIZE; i++)
            sector_bytes[i] = 0;

        return sector_bytes;
    }

    public static void main(String... args) throws Exception {
        Disk d = new Disk("disk.data", 4*1024*1024*Disk.SECTOR_SIZE);
        d.write_sector(2, strToByte("Hello!"));
        /*
        d.write_sector(1, strToByte("Hello!"));

        */
        byte[] data = new byte[Disk.SECTOR_SIZE];
        d.read_sector(1, data);
        System.out.println("Sector 1 has data " + new String(data));
        d.read_sector(2, data);
        System.out.println("Sector 1 has data " + new String(data));
    }
}
