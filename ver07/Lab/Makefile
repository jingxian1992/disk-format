# virtual machine
QEMU 		= qemu-system-i386

# images and rom
IMAGE_DIR	= .
FLOPPYA_IMG	= $(IMAGE_DIR)/fda.img
HDA_IMG		= $(IMAGE_DIR)/hda.img
HDB_IMG		= $(IMAGE_DIR)/hdb.img

# 参数
.PHONY: all 

#-hda $(HDA_IMG) -hdb $(HDB_IMG)
# 网卡配置: 
#	-net nic,vlan=0,model=rtl8139,macaddr=12:34:56:78:9a:be
# 网络模式：
#	1.User mode network(Slirp) :User网络
#		-net user
#	2.Tap/tun network : Tap网络
#		-net tap
# 		-net tap,vlan=0,ifname=tap0
#	example: -net nic,model=rtl8139 -net tap,ifname=tap0,script=no,downscript=no 
		
# 音频配置：
# 	a.使用蜂鸣器：-soundhw pcspk
#	b.使用声霸卡：-soundhw sb16
# 控制台串口调试： -serial stdio 

# 磁盘配置：
#	1. IDE DISK：-hda $(HDA_IMG) -hdb $(HDB_IMG) \
# 	2. AHCI DISK: -drive id=disk0,file=$(HDA_IMG),if=none \
		-drive id=disk1,file=$(HDB_IMG),if=none \
		-device ahci,id=ahci \
		-device ide-drive,drive=disk0,bus=ahci.0 \
		-device ide-drive,drive=disk1,bus=ahci.1 \


QEMU_ARGUMENT = -m 33M \
		-name "Linux charity kernel code study" \
		-fda $(FLOPPYA_IMG) \
		-hda $(HDA_IMG) -hdb $(HDB_IMG) \
		-boot order=ac

GDB_FLAGS = -S -s

# -S -s
		
#		-fda $(FLOPPYA_IMG) -hda $(HDA_IMG) -hdb $(HDB_IMG) -boot a \
#		-net nic,model=rtl8139 -net tap,ifname=tap0,script=no,downscript=no 

# qemu启动
run: 
	$(QEMU) $(QEMU_ARGUMENT)

gdb: 
	$(QEMU) $(QEMU_ARGUMENT) $(GDB_FLAGS)




