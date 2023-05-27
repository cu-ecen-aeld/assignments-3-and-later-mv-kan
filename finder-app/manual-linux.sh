#!/bin/bash
sudo echo "starting manual compilation"

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15
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
# If OUTDIR already exists next command won't do any thing
mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
    # wget https://raw.githubusercontent.com/bwalle/ptxdist-vetero/f1332461242e3245a47b4685bc02153160c0a1dd/patches/linux-5.0/dtc-multiple-definition.patch
    # cd linux-stable
    # git apply ${OUTDIR}/dtc-multiple-definition.patch
    cd ${OUTDIR}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
    
    # TODO: Add your kernel build steps here
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH}  CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    # Skipping modules for this simple assignment
    # make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    # make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

mkdir -p "${OUTDIR}/rootfs/" && cd "${OUTDIR}/rootfs/"
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log
mkdir -p home

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


make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
#${CROSS_COMPILE}readelf -a /bin/busybox | grep "program interpreter"
#${CROSS_COMPILE}readelf -a /bin/busybox | grep "Shared library"
# TODO: Add library dependencies to rootfs

cd "${OUTDIR}/rootfs/"
# SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot )

# TODO: Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1
ln -sf dev/null dev/tty2

# Add dependencies
LIBS=$(aarch64-linux-gnu-readelf -a bin/busybox |grep "NEEDED"| grep -o -P '(?<=\[)[^\]]+(?=\])')
# Iterate over the library names
while IFS= read -r library; do
  # Use locate to find the library path
  library_path=$(locate -r "$library")

  # Find the line with "aarch64-linux-gnu" in the path
  target_line=$(echo "$library_path" | grep -m 1 "aarch64-linux-gnu")

  # Copy the library to the desired location
  output_lib="${OUTDIR}/rootfs/lib/${library}"
  if [ ! -f "${OUTDIR}/rootfs/lib/${library}" ] 
  then
    cp "$target_line" "${OUTDIR}/rootfs/lib/${library}"
  fi 
done <<< "$LIBS"
exit 1
# TODO: Clean and build the writer utility

cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs

cd ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/writer .
cp "${FINDER_APP_DIR}/finder.sh" .
cp ${FINDER_APP_DIR}/finder-test.sh .
cp ${FINDER_APP_DIR}/conf/username.txt .
cp ${FINDER_APP_DIR}/autorun-qemu.sh .

# TODO: Chown the root directory

cd ${OUTDIR}/rootfs
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz

find . | cpio -H newc -ov --owner root:root > ../initramfs.cpio
cd ..
gzip -f initramfs.cpio