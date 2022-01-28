#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

# Check if directory was created successfully
if [ ! -d "${OUTDIR}" ]
then
	echo "${OUTDIR} not created"
	exit 1
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
    echo "Build Step 1: Deep Clean the Kernel Build Tree"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    
    echo "Build Step 2: Make the defconfig for virt arm dev board"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
       
    echo "Build Step 3: Build a kernel image for booting with QEMU"
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all   
    
    echo "Build Step 4: Build the kernel modules"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules   
    
    echo "Build Step 5: Build the device tree"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
    
fi 

echo "Adding the Image in outdir"

# Copy the contents to outdir
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
 mkdir ${OUTDIR}/rootfs
 
# Check if directory was created successfully
if [ ! -d "${OUTDIR}/rootfs" ]
then
	echo "${OUTDIR}/rootfs not created"
	exit 1
fi	

cd ${OUTDIR}/rootfs
mkdir bin dev etc home lib proc sbin sys tmp usr var lib64
mkdir usr/bin usr/lib usr/sbin
mkdir -p var/log


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    echo "Configuring busybox"
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and insatll busybox
echo "Making and installing busybox"
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

cd ${OUTDIR}/rootfs

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"


# TODO: Add library dependencies to rootfs
echo "Adding library dependencies to rootfs"
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
cp $SYSROOT/lib/ld-linux-aarch64.so.1 lib
cp $SYSROOT/lib64/libm.so.6 lib64
cp $SYSROOT/lib64/libresolv.so.2 lib64
cp $SYSROOT/lib64/libc.so.6 lib64


# TODO: Make device nodes
echo "Making device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1


# TODO: Clean and build the writer utility
echo "Cleaning and building the writer utility"
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE} all


# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home
cp -r ${FINDER_APP_DIR}/conf/ ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home


# TODO: Chown the root directory
echo "Giving the rootfs directory root permission"
cd ${OUTDIR}/rootfs
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
echo "Creating initramfs.cpio.gz"
cd "$OUTDIR/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ..
gzip -f initramfs.cpio


