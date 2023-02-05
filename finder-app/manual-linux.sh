#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.
# edited by Malola Simman Srinivasan Kannan
# MailID: masr4788@colorado.edu

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

# Checking which directory
if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}
if [ ! -d "${OUTDIR}" ]; then
	echo "${OUTDIR} Directory does not exists" 
fi
cd "$OUTDIR"

if [ ! -d "${OUTDIR}/linux-stable" ]; then
   	#Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
   cd linux-stable
   echo "Checking out version ${KERNEL_VERSION}"
   git checkout ${KERNEL_VERSION}

   # TODO: Add your kernel build steps here
   make ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE} mrproper
   echo "Completed make arm-gcc mrproper"
   make ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE} defconfig
   echo "Completed make arm-gcc defconfig"
   make -j4 ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE} all
   echo "Completed make arm-gcc all"
   make ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE} modules
   echo "Completed make arm-gcc modules"
   make ARCH=arm64 CROSS_COMPILE=${CROSS_COMPILE} dtbs
   echo "Completed make arm-gcc dtbs"
fi

echo "Added the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}
echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
	sudo rm  -rf ${OUTDIR}/rootfs
fi
echo "Created the staging directory for the root filesystem"

# TODO: Create necessary base directories
mkdir -p ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p home/conf
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log
echo "Completed Creating necessary base directories"

# steps for Busy box
cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
	git clone git://busybox.net/busybox.git
	cd busybox
	git checkout ${BUSYBOX_VERSION}
	# TODO:  Configure busybox
	make distclean
	make defconfig
else
	cd busybox
fi
echo "Completed busy box steps"

# TODO: Make and install busybox
make -j4 CONFIG_PREFIX=$OUTDIR/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
echo "Completed Make and Installed busybox"

# Reading binaries
cd "${OUTDIR}/rootfs"

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"
echo "Completed Reading binaries"

# TODO: Add library dependencies to rootfs
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
cd "${SYSROOT}"
echo "Completed Adding library dependencies to rootfs"
cd "${OUTDIR}/rootfs"
sudo cp -L ${SYSROOT}/lib/ld-linux-aarch64.* lib
sudo cp -L ${SYSROOT}/lib64/libm.so.* lib64
sudo cp -L ${SYSROOT}/lib64/libresolv.so.* lib64
sudo cp -L ${SYSROOT}/lib64/libc.so.* lib64
echo "Completed copying files to Qemu"


# TODO: Make device nodes
cd "${OUTDIR}/rootfs"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1
ls -l dev
echo "Completed Make device nodes"

# TODO: Clean and build the writer utility
cd "${FINDER_APP_DIR}"
make clean
make CROSS_COMPILE=${CROSS_COMPILE}
echo "Completed build  wrter utility"

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cd "${FINDER_APP_DIR}"
echo "${FINDER_APP_DIR}"
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/finder.sh  ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/conf/username.txt ${OUTDIR}/rootfs/home/conf/
cp ${FINDER_APP_DIR}/conf/assignment.txt ${OUTDIR}/rootfs/home/conf/
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home/
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home/
echo "Completed copying files to QEMU"

# TODO: Chown the root directory
cd ${OUTDIR}/rootfs
sudo chown -R root:root *
echo "Completed chown step"


# TODO: Create initramfs.cpio.gz
cd "${OUTDIR}/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ..
echo "$(pwd)"
gzip -f initramfs.cpio
echo "completed Initramfs and gzip"

