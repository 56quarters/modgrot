obj-m += grot.o

all:
	make -C linux M=grot modules

clean:
	make -C linux M=grot clean

linux.tar.gz:
	wget -O linux.tar.gz 'https://github.com/torvalds/linux/archive/refs/tags/v5.4.tar.gz'

linux/grot: linux.tar.gz
	tar -xf linux.tar.gz
	mv linux-5.4 linux
	cd linux && ln -s .. grot

linux-setup: linux/grot
	make -C linux olddefconfig
	make -C linux prepare
	make -C linux scripts

machine/focal-current.img:
	mkdir -p machine
	wget -O machine/focal-current.img 'https://cloud-images.ubuntu.com/focal/current/focal-server-cloudimg-amd64.img'
	qemu-img resize machine/focal-current.img 20G

machine/user-data.img: cloud-init.yaml
	mkdir -p machine
	cloud-localds machine/user-data.img cloud-init.yaml

qemu-download: machine/focal-current.img

qemu-cloud-init: machine/user-data.img

qemu: qemu-cloud-init qemu-download
	qemu-system-x86_64 \
		-enable-kvm \
		-m 2G \
		-cpu host \
		-vga virtio \
		-drive file=machine/focal-current.img,format=qcow2 \
		-drive file=machine/user-data.img,format=raw \
		-virtfs local,path=.,mount_tag=host0,security_model=passthrough,id=host0

# in guest:
# $ mkdir /mnt/shared
# $ mount -t 9p -o trans=virtio host0 /mnt/shared
# $ cd /mnt/shared
# $ insmod grot.ko
# $ grep grot /proc/devices
# $ mknod /dev/grot c 242 0
