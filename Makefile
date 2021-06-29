obj-m += grot.o

all:
	make -C linux M=grot modules

clean:
	make -C linux M=grot clean

# https://cloud-images.ubuntu.com/focal/current/focal-server-cloudimg-amd64-disk-kvm.img
# https://cloud-images.ubuntu.com/focal/current/focal-server-cloudimg-amd64.img

qemu-cloud-init:
	cloud-localds user-data.img cloud-init.yaml

qemu: qemu-cloud-init
	qemu-system-x86_64 \
		-enable-kvm \
		-m 2G \
		-cpu host \
		-vga virtio \
		-drive file=machine/focal-current.img,format=qcow2 \
		-drive file=user-data.img,format=raw \
		-virtfs local,path=.,mount_tag=host0,security_model=passthrough,id=host0
