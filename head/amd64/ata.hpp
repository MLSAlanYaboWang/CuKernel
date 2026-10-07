#if !defined(ATA_HPP)
#define ATA_HPP

#include "types.hpp"
#include "io.hpp"

// ============================================================
// 寄存器偏移（相对通道基址）
// ============================================================
static const uint16 ATA_REG_DATA     = 0;
static const uint16 ATA_REG_ERROR    = 1;
static const uint16 ATA_REG_FEATURES = 1;
static const uint16 ATA_REG_SECCOUNT = 2;
static const uint16 ATA_REG_LBA0     = 3;
static const uint16 ATA_REG_LBA1     = 4;
static const uint16 ATA_REG_LBA2     = 5;
static const uint16 ATA_REG_DRIVE    = 6;
static const uint16 ATA_REG_STATUS   = 7;
static const uint16 ATA_REG_COMMAND  = 7;

// 控制端口（固定，不是 base+偏移）
static const uint16 ATA_CTRL_PRIMARY   = 0x3F6;
static const uint16 ATA_CTRL_SECONDARY = 0x376;

// ============================================================
// Status 位
// ============================================================
static const uint8 ATA_SR_BSY  = 0x80;
static const uint8 ATA_SR_DRDY = 0x40;
static const uint8 ATA_SR_DF   = 0x20;
static const uint8 ATA_SR_DSC  = 0x10;
static const uint8 ATA_SR_DRQ  = 0x08;
static const uint8 ATA_SR_CORR = 0x04;
static const uint8 ATA_SR_IDX  = 0x02;
static const uint8 ATA_SR_ERR  = 0x01;

// ============================================================
// Error 位
// ============================================================
static const uint8 ATA_ER_BBK  = 0x80;
static const uint8 ATA_ER_UNC  = 0x40;
static const uint8 ATA_ER_MC   = 0x20;
static const uint8 ATA_ER_IDNF = 0x10;
static const uint8 ATA_ER_MCR  = 0x08;
static const uint8 ATA_ER_ABRT = 0x04;
static const uint8 ATA_ER_TK0NF= 0x02;
static const uint8 ATA_ER_AMNF = 0x01;

// ============================================================
// 命令
// ============================================================
static const uint8 ATA_CMD_READ_PIO     = 0x20;
static const uint8 ATA_CMD_WRITE_PIO    = 0x30;
static const uint8 ATA_CMD_CACHE_FLUSH  = 0xE7;
static const uint8 ATA_CMD_IDENTIFY     = 0xEC;
static const uint8 ATA_CMD_IDENTIFY_PKT = 0xA1;

// 探测用 drive/head 值（bit7-4 = 1010/1011）
static const uint8 ATA_PROBE_MASTER = 0xA0;
static const uint8 ATA_PROBE_SLAVE  = 0xB0;

// 读写用 drive/head 值（bit7-4 = 1110/1111）
static const uint8 ATA_RW_MASTER = 0xE0;
static const uint8 ATA_RW_SLAVE  = 0xF0;

// ============================================================
// 全局状态
// ============================================================
inline uint16 g_ataBase   = 0x1F0;
inline uint16 g_ataCtrl   = 0x3F6;
inline uint8  g_ataDrive  = 0xE0;
inline bool   g_ataReady  = false;

// ============================================================
// 400ns 延迟：读 status 4 次
// ============================================================
inline void ataDelay400(uint16 base) {
    for (int i = 0; i < 4; ++i) {
        asm volatile("" ::: "memory");
        (void)inb(base + ATA_REG_STATUS);
    }
}

// ============================================================
// 软复位（SRST）
// ============================================================
inline void ataSoftReset(uint16 ctrl) {
    outb(ctrl, 0x04);
    for (volatile int i = 0; i < 10000; ++i) {}
    outb(ctrl, 0x00);
    for (volatile int i = 0; i < 10000; ++i) {}
}

// ============================================================
// 等 BSY 清零
// ============================================================
inline bool ataWaitBsy(uint16 base) {
    uint32 t = 1000000;
    while ((inb(base + ATA_REG_STATUS) & ATA_SR_BSY) && --t) {}
    return t != 0;
}

// ============================================================
// 等 DRQ 置位（检查 ERR/DF）
// ============================================================
inline bool ataWaitDrq(uint16 base) {
    uint32 t = 1000000;
    while (t--) {
        uint8 s = inb(base + ATA_REG_STATUS);
        if (s & (ATA_SR_ERR | ATA_SR_DF)) return false;
        if (s & ATA_SR_DRQ) return true;
    }
    return false;
}

// ============================================================
// 探测某个位置是否有 ATA 盘
//   probeDrive: 0xA0 (master) 或 0xB0 (slave)
// ============================================================
inline bool ataIsDisk(uint16 base, uint8 probeDrive) {
    // 1. 选盘 + 延迟
    outb(base + ATA_REG_DRIVE, probeDrive);
    ataDelay400(base);

    // 2. status 判空
    uint8 s = inb(base + ATA_REG_STATUS);
    if (s == 0x00 || s == 0xFF) return false;

    // 3. 清寄存器
    outb(base + ATA_REG_SECCOUNT, 0);
    outb(base + ATA_REG_LBA0, 0);
    outb(base + ATA_REG_LBA1, 0);
    outb(base + ATA_REG_LBA2, 0);

    // 4. 发 IDENTIFY
    outb(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    // 5. status = 0 则无设备
    s = inb(base + ATA_REG_STATUS);
    if (s == 0x00) return false;

    // 6. 等 BSY
    if (!ataWaitBsy(base)) return false;

    // 7. LBA1/LBA2 非 0 → ATAPI/SATA
    if (inb(base + ATA_REG_LBA1) != 0 || inb(base + ATA_REG_LBA2) != 0)
        return false;

    // 8. 等 DRQ
    if (!ataWaitDrq(base)) return false;

    // 9. 读 256 字 IDENTIFY 数据
    uint16 id[256];
    for (int i = 0; i < 256; ++i)
        id[i] = inw(base + ATA_REG_DATA);

    // 10. 检查 word 0
    if (id[0] == 0x0000) return false;
    if (id[0] == 0xEB14 || id[0] == 0xC33C) return false;

    return true;
}

// ============================================================
// 探测所有 4 个位置
// ============================================================
inline bool ataDetect() {
    struct {
        uint16 base;
        uint16 ctrl;
        uint8  probe;
        uint8  rw;
    } cands[] = {
        {0x1F0, ATA_CTRL_PRIMARY,   ATA_PROBE_MASTER, ATA_RW_MASTER},
        {0x1F0, ATA_CTRL_PRIMARY,   ATA_PROBE_SLAVE,  ATA_RW_SLAVE },
        {0x170, ATA_CTRL_SECONDARY, ATA_PROBE_MASTER, ATA_RW_MASTER},
        {0x170, ATA_CTRL_SECONDARY, ATA_PROBE_SLAVE,  ATA_RW_SLAVE },
    };

    for (auto& c : cands) {
        ataSoftReset(c.ctrl);
        if (ataIsDisk(c.base, c.probe)) {
            g_ataBase  = c.base;
            g_ataCtrl  = c.ctrl;
            g_ataDrive = c.rw;
            g_ataReady = true;
            return true;
        }
    }
    g_ataReady = false;
    return false;
}

// ============================================================
// LBA28 读一个扇区
// ============================================================
inline bool ataRead28(uint32 lba, uint8* buf) {
    if (!g_ataReady) return false;
    if (lba > 0x0FFFFFFF) return false;

    uint16 base  = g_ataBase;
    uint8  drive = g_ataDrive | ((lba >> 24) & 0x0F);

    if (!ataWaitBsy(base)) return false;

    outb(base + ATA_REG_DRIVE, drive);
    ataDelay400(base);

    outb(base + ATA_REG_SECCOUNT, 1);
    outb(base + ATA_REG_LBA0, lba & 0xFF);
    outb(base + ATA_REG_LBA1, (lba >> 8)  & 0xFF);
    outb(base + ATA_REG_LBA2, (lba >> 16) & 0xFF);
    outb(base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    ataDelay400(base);

    if (!ataWaitBsy(base)) return false;
    if (!ataWaitDrq(base)) return false;

    for (int i = 0; i < 256; ++i)
        ((uint16*)buf)[i] = inw(base + ATA_REG_DATA);

    return true;
}

// ============================================================
// LBA28 写一个扇区
// ============================================================
inline bool ataWrite28(uint32 lba, const uint8* buf) {
    if (!g_ataReady) return false;
    if (lba > 0x0FFFFFFF) return false;

    uint16 base  = g_ataBase;
    uint8  drive = g_ataDrive | ((lba >> 24) & 0x0F);

    if (!ataWaitBsy(base)) return false;

    outb(base + ATA_REG_DRIVE, drive);
    ataDelay400(base);

    outb(base + ATA_REG_SECCOUNT, 1);
    outb(base + ATA_REG_LBA0, lba & 0xFF);
    outb(base + ATA_REG_LBA1, (lba >> 8)  & 0xFF);
    outb(base + ATA_REG_LBA2, (lba >> 16) & 0xFF);
    outb(base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    ataDelay400(base);

    if (!ataWaitBsy(base)) return false;
    if (!ataWaitDrq(base)) return false;

    for (int i = 0; i < 256; ++i)
        outw(base + ATA_REG_DATA, ((const uint16*)buf)[i]);

    outb(base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    ataDelay400(base);
    return ataWaitBsy(base);
}

// ============================================================
// 多扇区读（LBA28，PIO）
// ============================================================
inline bool ataRead28Multi(uint32 lba, uint32 count, uint8* buf) {
    if (!g_ataReady) return false;
    if (count == 0 || count > 256) return false;
    if (lba + count > 0x10000000) return false;

    uint16 base  = g_ataBase;
    uint8  drive = g_ataDrive | ((lba >> 24) & 0x0F);

    if (!ataWaitBsy(base)) return false;
    outb(base + ATA_REG_DRIVE, drive);
    ataDelay400(base);

    outb(base + ATA_REG_SECCOUNT, (uint8)count);
    outb(base + ATA_REG_LBA0, lba & 0xFF);
    outb(base + ATA_REG_LBA1, (lba >> 8)  & 0xFF);
    outb(base + ATA_REG_LBA2, (lba >> 16) & 0xFF);
    outb(base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    ataDelay400(base);

    for (uint32 sec = 0; sec < count; ++sec) {
        if (!ataWaitBsy(base)) return false;
        if (!ataWaitDrq(base)) return false;
        for (int i = 0; i < 256; ++i)
            ((uint16*)buf)[sec * 256 + i] = inw(base + ATA_REG_DATA);
    }
    return true;
}

// ============================================================
// 多扇区写（LBA28，PIO）
// ============================================================
inline bool ataWrite28Multi(uint32 lba, uint32 count, const uint8* buf) {
    if (!g_ataReady) return false;
    if (count == 0 || count > 256) return false;
    if (lba + count > 0x10000000) return false;

    uint16 base  = g_ataBase;
    uint8  drive = g_ataDrive | ((lba >> 24) & 0x0F);

    if (!ataWaitBsy(base)) return false;
    outb(base + ATA_REG_DRIVE, drive);
    ataDelay400(base);

    outb(base + ATA_REG_SECCOUNT, (uint8)count);
    outb(base + ATA_REG_LBA0, lba & 0xFF);
    outb(base + ATA_REG_LBA1, (lba >> 8)  & 0xFF);
    outb(base + ATA_REG_LBA2, (lba >> 16) & 0xFF);
    outb(base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    ataDelay400(base);

    for (uint32 sec = 0; sec < count; ++sec) {
        if (!ataWaitBsy(base)) return false;
        if (!ataWaitDrq(base)) return false;
        for (int i = 0; i < 256; ++i)
            outw(base + ATA_REG_DATA, ((const uint16*)buf)[sec * 256 + i]);
    }

    outb(base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    ataDelay400(base);
    return ataWaitBsy(base);
}

#endif