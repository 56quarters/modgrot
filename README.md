# modgrot

Yes, hello

`modgrot` is a Linux kernel module implementation of several critical
functions performed by a Slack bot (`grot`) at Grafana.

## What?

`modgrot` creates a character device that can do two things:

* Say "Yes, hello" when read.
* Accept writes (up to 255 bytes) and reply with that data the next time
  the device is read.

That's it.

## Why?

Gotta go fast.

## Setup

Follow the steps below to build and load `modgrot`.

**WARNING** Loading kernel modules has the potential to crash your machine
and I'm not very good at writing C. You have been warned!!!

### Get modgrot source

```
git clone git@github.com:56quarters/modgrot.git && cd modgrot
```

### Get kernel source

If you're on Ubuntu, this is a package called `linux-source` and should match
the version of the kernel you're running.

Installing it will leave you with a tarball of source code at 
`/usr/src/linux-source-5.4.0.tar.bz2`. Copy this tarball, unzip the contents
in your `modgrot` repo.

```
cp /usr/src/linux-source-5.4.0.tar.bz2 .
tar -xf linux-source-5.4.0.tar.bz2
```

Rename the sources directory and create a symlink back to `modgrot` sources.

```
mv linux-source-5.4.0 linux
cd linux
ln -s .. grot
```

### Kernel setup

In the kernel sources directory, run the following commands to get it
roughly setup.

```
make oldconfig
make prepare
make scripts
```
### Build

If everything worked, you should be able to build the module now from the
root of the `modgrot` repo.

```
make
```

### Running

To run the kernel module in a VM, you'll need

* KVM enabled
* QEMU installed
* `cloud-image-utils` installed

After installing everything, run:

```
make qemu-download
make qemu-cloud-init
sudo make qemu
```

If everything worked, you'll have an Ubuntu VM starting up with the default
user `ubuntu` and the password `ubuntu`.

Inside the **running VM** run the following commands.

```
sudo -i
mkdir /mnt/shared
mount -t 9p -o trans=virtio host0 /mnt/shared
cd /mnt/shared
ls
```

If that worked you should see files from the `modgrot` repo. Next step, load
the module in your VM.

```
insmod grot.ko
grep grot /proc/devices
mknod /dev/grot c $NUMBER_FROM_ABOVE 0
```

Try reading.

```
cat /dev/grot
Yes, hello
```

Bam.

## References

* https://lyngvaer.no/log/writing-pseudo-device-driver
* https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html
* https://powersj.io/posts/ubuntu-qemu-cli/
* https://superuser.com/questions/628169/how-to-share-a-directory-with-the-host-without-networking-in-qemu
* https://www.kernel.org/doc/htmldocs/kernel-hacking/
