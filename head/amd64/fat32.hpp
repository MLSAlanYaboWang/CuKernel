#if !defined(FAT32_HPP)
#define FAT32_HPP

#include "types.hpp"
#include "ahci.hpp"
#include "heap.hpp"
#include "VGA_console.hpp"

struct FAT32 {
    AhciPort* disk;
    uint32 bytesPerSector;
    uint32 sectorsPerCluster;
    uint32 reservedSectors;
    uint32 numFats;
    uint32 fatSize;
    uint32 rootCluster;
    uint32 fatStart;
    uint32 dataStart;
    uint32 clusterSize;
};

struct FAT32DirEntry {
    uint8  name[8];
    uint8  ext[3];
    uint8  attr;
    uint8  reserved;
    uint8  createTimeTenth;
    uint16 createTime;
    uint16 createDate;
    uint16 accessDate;
    uint16 clusterHigh;
    uint16 modifyTime;
    uint16 modifyDate;
    uint16 clusterLow;
    uint32 fileSize;
} __attribute__((packed));

inline bool fat32Init(FAT32* fs, AhciPort* disk) {
    uint8* bpb = (uint8*)kmalloc(512);
    if (!ahciRead(disk, 0, 1, bpb)) { kfree(bpb); return false; }
    if (bpb[510] != 0x55 || bpb[511] != 0xAA) { kfree(bpb); return false; }

    fs->disk = disk;
    fs->bytesPerSector    = *(uint16*)(bpb + 0x0B);
    fs->sectorsPerCluster = *(uint8*)(bpb + 0x0D);
    fs->reservedSectors   = *(uint16*)(bpb + 0x0E);
    fs->numFats           = *(uint8*)(bpb + 0x10);
    fs->fatSize           = *(uint32*)(bpb + 0x24);
    fs->rootCluster       = *(uint32*)(bpb + 0x2C);

    fs->fatStart  = fs->reservedSectors;
    fs->dataStart = fs->reservedSectors + fs->numFats * fs->fatSize;
    fs->clusterSize = fs->bytesPerSector * fs->sectorsPerCluster;

    kfree(bpb);
    return true;
}

inline uint32 fat32ClusterToLba(FAT32* fs, uint32 cluster) {
    return fs->dataStart + (cluster - 2) * fs->sectorsPerCluster;
}

inline bool fat32ReadCluster(FAT32* fs, uint32 cluster, uint8* buf) {
    uint32 lba = fat32ClusterToLba(fs, cluster);
    return ahciRead(fs->disk, lba, fs->sectorsPerCluster, buf);
}

inline uint32 fat32NextCluster(FAT32* fs, uint32 cluster) {
    uint32 fatOffset = cluster * 4;
    uint32 fatSector = fs->fatStart + (fatOffset / fs->bytesPerSector);
    uint32 offsetInSector = fatOffset % fs->bytesPerSector;

    uint8* buf = (uint8*)kmalloc(fs->bytesPerSector);
    ahciRead(fs->disk, fatSector, 1, buf);
    uint32 val = *(uint32*)(buf + offsetInSector) & 0x0FFFFFFF;
    kfree(buf);
    return val;
}

// ★ 前置声明
inline uint32 fat32ReadFile(FAT32* fs, uint32 startCluster, uint32 fileSize, uint8* buf);

// ★ 实现
inline uint32 fat32ReadFile(FAT32* fs, uint32 startCluster, uint32 fileSize, uint8* buf) {
    uint32 read = 0;
    uint32 cluster = startCluster;
    uint8* clusterBuf = (uint8*)kmalloc(fs->clusterSize);

    while (cluster < 0x0FFFFFF8 && read < fileSize) {
        if (!fat32ReadCluster(fs, cluster, clusterBuf)) break;

        uint32 toCopy = fs->clusterSize;
        if (read + toCopy > fileSize) toCopy = fileSize - read;

        for (uint32 i = 0; i < toCopy; ++i) {
            buf[read + i] = clusterBuf[i];
        }
        read += toCopy;

        cluster = fat32NextCluster(fs, cluster);
    }

    kfree(clusterBuf);
    return read;
}

inline void fat32PrintName(FAT32DirEntry* e) {
    for (int i = 0; i < 8; ++i) {
        if (e->name[i] == ' ') break;
        printChar(e->name[i]);
    }
    if (e->ext[0] != ' ') {
        printChar('.');
        for (int i = 0; i < 3; ++i) {
            if (e->ext[i] == ' ') break;
            printChar(e->ext[i]);
        }
    }
}

inline void fat32ListDir(FAT32* fs, uint32 startCluster) {
    uint8* buf = (uint8*)kmalloc(fs->clusterSize);
    uint32 cluster = startCluster;

    while (cluster < 0x0FFFFFF8) {
        if (!fat32ReadCluster(fs, cluster, buf)) break;

        uint32 entries = fs->clusterSize / 32;
        for (uint32 i = 0; i < entries; ++i) {
            FAT32DirEntry* e = (FAT32DirEntry*)(buf + i * 32);

            if (e->name[0] == 0x00) { kfree(buf); return; }
            if (e->name[0] == 0xE5) continue;
            if (e->attr == 0x0F) continue;
            if (e->attr & 0x08) continue;

            printString("  ");
            fat32PrintName(e);
            printString("  size=");
            printHex<8>(e->fileSize);
            printString(" cluster=");
            uint32 fileCluster = ((uint32)e->clusterHigh << 16) | e->clusterLow;
            printHex<8>(fileCluster);

            if (e->attr & 0x10) {
                printString(" [DIR]");
            } else {
                uint32 size = e->fileSize;
                if (size > 0 && fileCluster >= 2) {
                    uint8* fbuf = (uint8*)kmalloc(size + 1);
                    uint32 n = fat32ReadFile(fs, fileCluster, size, fbuf);
                    fbuf[n] = 0;
                    printString("  content: ");
                    printString((cstring)fbuf);
                    kfree(fbuf);
                }
            }

            printString("\n");
        }

        cluster = fat32NextCluster(fs, cluster);
    }
    kfree(buf);
}

#endif