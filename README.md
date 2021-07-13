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

Gotta go fast. Moving Slack bot functionality into the Linux kernel was
the next logical evolution. For some reason.

## Setup

Follow the steps below to build and load `modgrot`.

**WARNING** Loading kernel modules has the potential to crash your machine
and I'm not very good at writing C. You have been warned!!!

Assumptions:

* You are running Ubuntu 20.04 LTS (Focal Fossa) as your host OS
* You will use an Ubuntu 20.04 LTS (Focal Fossa) as your guest OS (QEMU VM)

### Get modgrot source

```
git clone git@github.com:56quarters/modgrot.git && cd modgrot
```

### Get kernel source

The modgrot `Makefile` will download, unpack, and configure sources for the
5.4 Linux kernel (since this is what Ubuntu 20.04 LTS uses).

```
make linux-setup
```

Next, in the kernel sources directory, copy `Modules.symvers` from your local
modules to our copy of the kernel source.

```
cp /lib/modules/`uname -r`/build/Module.symvers .
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
user `ubuntu` and the password `ubuntu`, with a directory shared with the host.

Next step, load the module in your VM.

```
sudo -i
cd /mnt/shared
insmod grot.ko
```

Try reading the newly created device.

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
* https://cloudinit.readthedocs.io/en/latest/topics/examples.html
