
  git clone https://github.com/masc-ucsc/dromajo.git
  cd dromajo/
  mkdir build
  cd build
  cmake -DCMAKE_BUILD_TYPE=Debug -DSIMPOINT=On ../
  make -j
  cd ../run/
  wget https://github.com/buildroot/buildroot/archive/refs/tags/2021.08.tar.gz
  # earlier version: wget https://github.com/buildroot/buildroot/archive/2020.05.1.tar.gz
  tar xvf 2021.08.tar.gz
  mv buildroot-2021.08 buildroot
  cp config-buildroot-2021.08 buildroot/.config
  y | make -j$(nproc) -C buildroot
  riscv64-linux-gnu-gcc -Wall -Os -static roi.c -o buildroot/output/target/sbin/roi
  cp -f S50bench buildroot/output/target/etc/init.d/
  make -j$(nproc) -C buildroot
  export CROSS_COMPILE=riscv64-linux-gnu-
  wget https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/snapshot/linux-5.15.tar.gz
  # earlier version: wget -nc https://git.kernel.org/torvalds/t/linux-5.8-rc4.tar.gz
  # or other version froms: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
  tar -xf linux-5.15.tar.gz
  mv linux-5.15 linux
  make -C linux ARCH=riscv defconfig
  make -C linux ARCH=riscv -j$(nproc)
  git clone https://github.com/riscv-software-src/opensbi.git
  cd opensbi
  git checkout tags/v0.9 -b temp2
  # earlier version: git checkout bd355213bfbb209c047e8cc0df56936f6705477f -b temp
  make PLATFORM=generic
  cd ..
  cp buildroot/output/images/rootfs.cpio .
  cp linux/arch/riscv/boot/Image .
  cp opensbi/build/platform/generic/firmware/fw_jump.bin .
  ../build/dromajo boot.cfg
