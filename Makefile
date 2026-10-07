NASM     := nasm
CXX      := g++
GDC      := gdc
LD       := ld
OBJCOPY  := objcopy
GENISO   := genisoimage
QEMU     := qemu-system-x86_64

BOOT_LOAD_SIZE := 128
KERNEL_SECTORS := 64
DISK_SIZE      := 64M

BOOT_ASM   := source/boot.asm
ISR_ASM    := source/isr.asm
KERNEL_CPP := source/kernel.cpp
VGA_CPP    := source/VGA_C_API.cpp
DEMO_D     := source/exp/demo.d
LINKER_LD  := linker.ld

CXXFLAGS := -m64 -std=c++17 -ffreestanding -fno-exceptions -fno-rtti \
            -fno-stack-protector -fno-pic -mno-red-zone -O2 -Wall -Wextra \
            -Ihead -Werror

GDCFLAGS := -m64 -O2 -ffreestanding -fno-druntime -fno-exceptions \
            -fno-moduleinfo -fno-pic -mno-red-zone -Wall \
            -Ihead

LDFLAGS  := -m elf_x86_64 -T $(LINKER_LD)

BUILD := build
ISO   := $(BUILD)/iso

all: $(BUILD)/livecd.iso

# ---- 目标文件 ----
$(BUILD)/kernel.o: $(KERNEL_CPP) | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/vga_c_api.o: $(VGA_CPP) | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/demo.o: $(DEMO_D) | $(BUILD)
	$(GDC) $(GDCFLAGS) -c $< -o $@

$(BUILD)/isr.o: $(ISR_ASM) | $(BUILD)
	$(NASM) -f elf64 $< -o $@

KERNEL_OBJS := $(BUILD)/kernel.o \
               $(BUILD)/vga_c_api.o \
               $(BUILD)/demo.o \
               $(BUILD)/isr.o

$(BUILD)/kernel.elf: $(KERNEL_OBJS) $(LINKER_LD)
	$(LD) $(LDFLAGS) -o $@ --start-group $(KERNEL_OBJS) --end-group

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@
	@echo "[kernel] $$(stat -c%s $@) bytes"

$(BUILD)/boot.bin: $(BOOT_ASM) | $(BUILD)
	$(NASM) -f bin -dKERNEL_SECTORS=$(KERNEL_SECTORS) $< -o $@
	@echo "[boot]   $$(stat -c%s $@) bytes"

$(BUILD)/boot.img: $(BUILD)/boot.bin $(BUILD)/kernel.bin
	cat $^ > $@
	@echo "[boot.img] $$(stat -c%s $@) bytes"
	@need=$$(( $(BOOT_LOAD_SIZE) * 512 )); \
	have=$$(stat -c%s $@); \
	if [ $$have -gt $$need ]; then \
	    echo "ERROR: boot.img 超过 $$need 字节"; \
	    exit 1; \
	fi

$(BUILD)/livecd.iso: $(BUILD)/boot.img
	mkdir -p $(ISO)/boot
	cp $(BUILD)/boot.img $(ISO)/boot/boot.img
	$(GENISO) -R \
	    -b boot/boot.img -no-emul-boot \
	    -boot-load-size $(BOOT_LOAD_SIZE) \
	    -o $@ $(ISO)
	@echo "[iso]    $$(stat -c%s $@) bytes"

$(BUILD)/disk.img:
	@echo "[disk] creating $(DISK_SIZE) FAT32 image..."
	dd if=/dev/zero of=$@ bs=1M count=$(shell echo $(DISK_SIZE) | sed 's/M//') 2>/dev/null
	mkfs.fat -F 32 $@ >/dev/null
	@echo "[disk] done"

run: $(BUILD)/livecd.iso $(BUILD)/disk.img
	$(QEMU) \
	  -cdrom $(BUILD)/livecd.iso \
	  -drive file=$(BUILD)/disk.img,format=raw,if=none,id=hd0 \
	  -device ahci,id=ahci \
	  -device ide-hd,drive=hd0,bus=ahci.0 \
	  -boot d

debug: $(BUILD)/livecd.iso $(BUILD)/disk.img
	$(QEMU) \
	  -cdrom $(BUILD)/livecd.iso \
	  -drive file=$(BUILD)/disk.img,format=raw,if=none,id=hd0 \
	  -device ahci,id=ahci \
	  -device ide-hd,drive=hd0,bus=ahci.0 \
	  -boot d \
	  -s -S

clean:
	rm -rf $(BUILD)

$(BUILD):
	mkdir -p $(BUILD)

.PHONY: all run debug clean