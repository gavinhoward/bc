# grafland toolchain, Copyright, 2015-2017. see LICENSE for copying details

LOGUSER = cmgraff
BUILDNO = 1
LOGDIR = logdir/
LOGHOST = $(shell gcc -dumpmachine)
LOGHOSTUNAME = $(shell uname -a )
SHELL = /bin/sh
STAGE0OPT = CFLAGS="-O0 -g0" CXXFLAGS="-O0 -g0"
MAKEINFO = false
TEXINFO = false
OM_PACK = $(shell printf "%s\n" $(pack) | dd conv=ucase)
OM_ARCH = i686
OM_SUBTARG = $(OM_ARCH)
KBUILDAREA = $(shell pwd)/kernel
OM_MIDDLE = -pc-linux-
OM_LIBC = musl

OM_CONF = --disable-libmpx --disable-multilib
OM_LINUXARCH = i386
OM_BACKWARDCOMPAT = i386
OM_HOST = $(shell uname -m)
SPWD = $(shell pwd)
OM_PREF = $(SPWD)/$(OM_TARG)-toolchain
OM_SRC = $(SPWD)/src
OM_PATCH = $(SPWD)/patches
OM_SFX = tar.gz
OM_BINUTILS = binutils-2.25
#OM_BINUTILS = binutils-2.26
#OM_BINUTILS = binutils-2.28
#OM_GCC = gcc-4.9.2
OM_GCC = gcc-5.3.0
#OM_GCC = gcc-6.1.0
#OM_GCC = gcc-6.3.0
#OM_GCC = gcc-7.1.0
OM_GMP = gmp-5.1.3
OM_MPC = mpc-1.0.2
OM_MPFR = mpfr-3.1.2
#OM_MPFR = mpfr-3.1.4
#OM_LINUX = linux-3.9 # earliest kernel that will build, not tested
OM_LINUX = linux-4.1
#OM_MUSL = musl-1.1.14
#OM_MUSL = musl-1.1.16
OM_MUSL = musl-1.1.17
OM_LANGS = c,c++
#KERNEL_MRR = https://www.kernel.org/pub/linux/kernel/v3.0/
KERNEL_MRR = https://www.kernel.org/pub/linux/kernel/v4.x/
MUSL_MRR = http://www.musl-libc.org/releases/
GNU_MRR = https://ftp.gnu.org/gnu/
GENERIC_MRR = http://www.csit.parkland.edu/~cgraff1/src/
#OM_UCLIBC_VER = 1.0.13
OM_UCLIBC_VER = 1.0.24
OM_UCLIBC = uClibc-ng-$(OM_UCLIBC_VER)
UCLIBC_MRR = http://downloads.uclibc-ng.org/releases/$(OM_UCLIBC_VER)/
OM_GLIBC = glibc-2.25
OM_PERL_MRR = http://www.cpan.org/src/5.0/
OM_PERL_CROSS_MRR = https://github.com/arsv/perl-cross/releases/download/1.0.2/

OM_LLVM = http://releases.llvm.org/5.0.0/llvm-5.0.0.src.tar.xz
OM_CLANG = http://releases.llvm.org/5.0.0/cfe-5.0.0.src.tar.xz
OM_OPENMP = http://releases.llvm.org/5.0.0/openmp-5.0.0.src.tar.xz


-include config.mak
OM_TARG = $(OM_ARCH)$(OM_MIDDLE)$(OM_LIBC)

# Self bootstrapping
OM_BOOTSRC = $(shell pwd)/bootstrap

# Self building:
OM_SELFBLD = $(shell pwd)/$(OM_TARG)-host
OM_SELFBLDWRK = $(shell pwd)/$(OM_TARG)-host-work


# packages
OM_LIBROOT = $(shell pwd)/libroot
OM_NCURSES = ncurses-6.0
OM_NCURSES_MRR = https://ftp.gnu.org/gnu/ncurses/
OM_NETBSDCURSES = netbsd-curses-0.0.2
OM_NETBSDCURSES_MRR = http://www.csit.parkland.edu/~cgraff1/src/
OM_PDCURSES = PDCurses-3.4
OM_PDCURSES_MRR = https://sourceforge.net/projects/pdcurses/files/pdcurses/3.4/
OM_HTOP = htop-2.0.1
OM_HTOP_MRR = https://hisham.hm/htop/releases/2.0.1/
OM_COREUTILS = coreutils-8.25
OM_COREUTILS_MRR = $(GENERIC_MRR)
OM_VIM = vim-7.3.547
OM_UTILLINUX = util-linux-2.26.2
OM_DHCP = dhcp-4.1-ESV-R11
OM_DASH = dash-0.5.8
OM_STRACE = strace-4.9
OM_BUSYBOX = busybox-1.21.1
OM_DROPBEAR = dropbear-2015.67
OM_ZLIB = zlib-1.2.8
OM_PARTED = parted-3.1
OM_PARTED_MRR = $(GENERIC_MRR)
OM_E2FSPROGS = e2fsprogs-1.42.13
OM_GRUB = grub-2.00
OM_LINKS = links-2.9
OM_GDB = gdb-7.11
OM_PERL = perl-5.22.1
OM_PERL_CROSS = perl-5.22.1-cross-1.0.2
OM_PYTHON = Python-2.7.9
OM_BASH = bash-4.3.30
OM_BISON = bison-3.0.4
OM_GIT = git-4.3.20
OM_OCRAD = ocrad-0.25
OM_XORRISO = xorriso-1.4.0
OM_NANO = nano-2.5.3
OM_GROFF = groff-1.22.3
OM_BC = bc-1.06
OM_CPIO = cpio-2.12
OM_RSYNC = rsync-3.1.0
OM_RSYNC_MRR = https://download.samba.org/pub/rsync/src/
OM_LIBICONV = libiconv-1.14
OM_LIBUNISTRING = libunistring-0.9.6
OM_LIBTOOL = libtool-2.4
OM_LIBGC = gc-7.4.0
OM_LIBGC_MRR = http://www.hboehm.info/gc/gc_source/
OM_LIBATOMIC_OPS = libatomic_ops-7.4.2
OM_LIBATOMIC_OPS_MRR = http://www.ivmaisoft.com/_bin/atomic_ops/
OM_LIBFFI = libffi-3.2
OM_LIBFFI_MRR = https://www.mirrorservice.org/sites/sourceware.org/pub/libffi/
OM_SCREEN = screen-4.3.1
OM_OPENSSL = openssl-1.0.2g
OM_OPENSSL_MRR = https://www.openssl.org/source/
OM_LIBRESSL = libressl-2.3.2
OM_LIBRESSL_MRR = http://ftp.openbsd.org/pub/OpenBSD/LibreSSL/
OM_EMACS = emacs-24.5
OM_GAWK = gawk-4.1.3
OM_GETTEXT = gettext-0.19.7
OM_OCTAVE = octave-4.0.0
OM_PCRE = pcre-8.38
OM_PCRE_MRR = https://sourceforge.net/projects/pcre/files/pcre/8.38/
OM_BLAS = BLAS-3.6.0
OM_OPENSSH = openssh-7.0p1
OM_OPENSSH_MRR = http://mirrors.mit.edu/pub/OpenBSD/OpenSSH/portable/
OM_HURD = hurd-0.7
OM_LIGHTNING = lightning-2.1.0
OM_PATCH = patch-2.7.5
OM_WAYLAND = wayland-1.10.0
OM_WESTON = weston-1.10.0
OM_EXPAT = expat-2.1.0
OM_EXPAT_MRR = http://downloads.sourceforge.net/expat/
OM_LIBXML2 = libxml2-2.9.3
OM_LIBXML2_MRR = ftp://xmlsoft.org/libxml2/
OM_IRSSI = irssi-0.8.18
OM_IRSSI_MRR = https://github.com/irssi/irssi/releases/tag/0.8.18
OM_GLIB = glib-2.46.0
OM_GLIB_MRR = http://ftp.gnome.org/pub/gnome/sources/glib/2.46/
OM_SUDO = sudo-1.8.16
OM_SUDO_MRR = http://www.sudo.ws/dist/
OM_WICD = wicd-1.7.4
OM_WICD_MRR = https://launchpad.net/wicd/1.7/1.7.4/+download/
OM_WIRELESSTOOLS = wireless_tools.29
OM_WIRELESSTOOLS_MRR = http://www.hpl.hp.com/personal/Jean_Tourrilhes/Linux/
OM_LIBPAM = Linux-PAM-1.2.0
OM_LIBPAM_MRR = http://linux-pam.org/library/
OM_NETTLE = nettle-3.2
OM_NETTLE_MRR = https://ftp.gnu.org/gnu/nettle/
OM_WEECHAT = weechat-1.4
OM_WEECHAT_MRR = http://weechat.org/files/src/
OM_GNUCRYPT = gnu-crypto-2.1.0-rc1
OM_GNUCRYPT_MRR = ftp://ftp.gnupg.org/gcrypt/gnu-crypto/
OM_IRCII = ircii-20151120
OM_IRCII_MRR = http://ircii.warped.com/
OM_MPLAYER = MPlayer-1.3.0
OM_MPLAYER_MRR = http://www.mplayerhq.hu/MPlayer/releases/
OM_MUTT = mutt-1.4.2.3
OM_MUTT_MRR = ftp://ftp.mutt.org/pub/mutt/
OM_INETUTILS = inetutils-1.9.4
OM_INETUTILS_MRR = https://ftp.gnu.org/gnu/inetutils/
OM_ZSH = zsh-5.2
OM_ZSH_MRR = http://sourceforge.net/projects/zsh/files/zsh/5.2/
OM_YASM = yasm-1.3.0
OM_YASM_MRR = http://www.tortall.net/projects/yasm/releases/
OM_XZ = xz-5.2.2
OM_XZ_MRR = http://tukaani.org/xz/
OM_GPTFDISK = gptfdisk-0.8.10
OM_GPTFDISK_MRR = http://downloads.sourceforge.net/project/gptfdisk/gptfdisk/0.8.10/
OM_POPT = popt-1.16
OM_POPT_MRR = http://rpm5.org/files/popt/
OM_PDKSH = pdksh-5.3
OM_PDKSH_MRR = http://www.csit.parkland.edu/~cgraff1/src/
OM_TMUX = tmux-2.1
OM_TMUX_MRR = https://github.com/tmux/tmux/releases/download/2.1/
OM_VALGRIND = valgrind-3.11.0
OM_VALGRIND_MRR = http://www.csit.parkland.edu/~cgraff1/src/

# core
OM_MAKE = make-4.1
OM_MAKE_MRR = http://ftp.gnu.org/gnu/make/
OM_GENEXTFS = genext2fs-1.4.1
OM_GENEXTFS_MRR = https://sourceforge.net/projects/genext2fs/files/genext2fs/1.4.1/
OM_M4 = m4-1.4.17
OM_M4_MRR = https://ftp.gnu.org/gnu/m4/

# logging
LOGNAME = build_$(BUILDNO)__$(OM_GCC)__$(LOGHOST)

# environment
INACROSS = \
	CROSS_COMPILE=$(OM_TARG)- \
	CC='$(OM_TARG)-gcc -lm' \
	LD=$(OM_TARG)-ld \
	RANLIB=$(OM_TARG)-ranlib \
	AS=$(OM_TARG)-as \
	CXX=$(OM_TARG)-g++ \
	NM=$(OM_TARG)-nm \
	STRIP=$(OM_TARG)-strip \
	OBJDUMP=$(OM_TARG)-objdump \
	AR=$(OM_TARG)-ar \
	F77=$(OM_TARG)-gfortran \
	CC_FOR_BUILD=gcc \
	HOSTCC=gcc
	

# openssl needs a special environment 
OM_OPENSSL_CROSS_ENV = \
	TARGETMACH=$(OM_TARG) \
	BUILDMACH=$(OM_HOST) \
	CROSS=$(OM_TARG) \
	CC=$(OM_TARG)-gcc \
	LD=$(OM_TARG)-ld \
	AS=$(OM_TARG)-as \
	AR=$(OM_TARG)-ar \
	MAKEDEPEND='makedepend -- $(OM_TARG)/$(OM_TARG)'

all:
	-mkdir -p $(LOGDIR)
	-$(MAKE) primary > $(LOGDIR)/$(OM_TARG)-build-log 2>&1
	-$(MAKE) trivtest

primary:

	$(MAKE) -i populate
	#$(MAKE) generate_host_tools
	$(MAKE) install_path
	$(MAKE) binutils_stage_make
	$(MAKE) binutils_stage_install
	$(MAKE) linux_stage
	$(MAKE) gcc_stage_make
	$(MAKE) gcc_stage_install
ifeq ($(OM_LIBC),musl)
	$(MAKE) musl_stage
endif
ifeq ($(OM_LIBC),musleabi)
	$(MAKE) musl_stage
endif
ifeq ($(OM_LIBC),uclibc)
	-$(MAKE) uclibc_stage_one
	-$(MAKE) uclibc_stage_two
	-$(MAKE) uclibc_stage_three
endif
ifeq ($(OM_LIBC),gnu)
	-$(MAKE) -i gnu_stage_one
	-$(MAKE) -i gnu_stage_two
	-$(MAKE) -i gnu_stage_three
endif

install_path: host_path

	$(eval PATH=$(OM_PREF)/bin:$(PATH))
	-@mkdir -p $(OM_PREF)/$(OM_TARG)/usr/include
	-@mkdir -p $(OM_PREF)/$(OM_TARG)/include

binutils_stage_make: host_path

	@cd src && test -e $(OM_BINUTILS).$(OM_SFX) || wget $(GNU_MRR)/binutils/$(OM_BINUTILS).$(OM_SFX)
	@test -d $(OM_BINUTILS) || tar -xf $(OM_SRC)/$(OM_BINUTILS).$(OM_SFX) -C $(SPWD)
	
	@mkdir -p $(SPWD)/$(OM_BINUTILS)-build
	-@cd $(SPWD)/$(OM_BINUTILS)-build && $(SPWD)/$(OM_BINUTILS)/configure \
	--prefix=$(OM_PREF) \
	--target=$(OM_TARG) \
	$(OM_CONF)
	$(MAKE) -C $(SPWD)/$(OM_BINUTILS)-build

binutils_stage_install: host_path

	$(MAKE) -C $(SPWD)/$(OM_BINUTILS)-build install 

linux_stage: host_path

	@cd src && test -e $(OM_LINUX).$(OM_SFX) || wget $(KERNEL_MRR)/$(OM_LINUX).$(OM_SFX)
	@test -d $(OM_LINUX) || tar -xf $(OM_SRC)/$(OM_LINUX).$(OM_SFX) -C $(SPWD)
	
	$(MAKE) -C $(SPWD)/$(OM_LINUX) ARCH=$(OM_LINUXARCH) \
	INSTALL_HDR_PATH=$(OM_PREF)/$(OM_TARG)/ \
	headers_install O=$(KBUILDAREA)

gcc_stage_make: host_path

	@cd patches && test -e gcc-4.7.4-musl.diff || wget $(GENERIC_MRR)/gcc-4.7.4-musl.diff
	@cd patches && test -e gcc-4.8.5-musl.diff || wget $(GENERIC_MRR)/gcc-4.8.5-musl.diff
	@cd patches && test -e gcc-4.9.2-musl.diff || wget $(GENERIC_MRR)/gcc-4.9.2-musl.diff
	@cd patches && test -e gcc-4.9.3-musl.diff || wget $(GENERIC_MRR)/gcc-4.9.3-musl.diff
	@cd patches && test -e gcc-5.3.0-musl.diff || wget $(GENERIC_MRR)/gcc-5.3.0-musl.diff
	@cd src && test -e $(OM_GMP).$(OM_SFX) || wget $(GNU_MRR)/gmp/$(OM_GMP).$(OM_SFX)
	@cd src && test -e $(OM_MPFR).$(OM_SFX) || wget $(GNU_MRR)/mpfr/$(OM_MPFR).$(OM_SFX)
	@cd src && test -e $(OM_MPC).$(OM_SFX) || wget $(GNU_MRR)/mpc/$(OM_MPC).$(OM_SFX)
	@cd src && test -e $(OM_GCC).$(OM_SFX) || wget $(GNU_MRR)/gcc/$(OM_GCC)/$(OM_GCC).$(OM_SFX)
	@test -d $(OM_GCC) || tar -xf $(OM_SRC)/$(OM_GCC).$(OM_SFX) -C $(SPWD)

	@tar -xf $(OM_SRC)/$(OM_GMP).$(OM_SFX) -C $(SPWD)
	@tar -xf $(OM_SRC)/$(OM_MPFR).$(OM_SFX) -C $(SPWD) 
	@tar -xf $(OM_SRC)/$(OM_MPC).$(OM_SFX) -C $(SPWD) 

ifeq ($(OM_LIBC),musl)
	-@cd $(SPWD)/$(OM_GCC) && patch -p1 -N < $(SPWD)/patches/$(OM_GCC)-musl.diff
endif
	
	@cd $(SPWD)/$(OM_GCC) && ln -s $(SPWD)/$(OM_MPFR) mpfr 
	@cd $(SPWD)/$(OM_GCC) && ln -s $(SPWD)/$(OM_GMP) gmp 
	@cd $(SPWD)/$(OM_GCC) && ln -s $(SPWD)/$(OM_MPC) mpc

	@mkdir -p $(SPWD)/$(OM_GCC)-build
	@cd $(SPWD)/$(OM_GCC)-build && $(SPWD)/$(OM_GCC)/configure \
	--prefix=$(OM_PREF) \
	--target=$(OM_TARG) \
	--enable-languages=c \
	--with-newlib \
	--disable-libssp \
	--disable-nls \
	--disable-libquadmath \
	--disable-threads \
	--disable-decimal-float \
	--disable-shared \
	--disable-libmudflap \
	--disable-libgomp \
	--disable-libatomic \
	$(OM_CONF)

	$(STAGE0OPT) $(MAKE) -C $(SPWD)/$(OM_GCC)-build

gcc_stage_install: host_path

	$(MAKE) -C $(SPWD)/$(OM_GCC)-build install 

musl_stage: install_path

	@cd src && test -e $(OM_MUSL).$(OM_SFX) || wget $(MUSL_MRR)/$(OM_MUSL).$(OM_SFX)
	-@rm -rf $(SPWD)/$(OM_MUSL)
	@tar -xf $(OM_SRC)/$(OM_MUSL).$(OM_SFX) -C $(SPWD)
	@printf "%s\n" $(PATH)
	@cd $(SPWD)/$(OM_MUSL) && $(SPWD)/$(OM_MUSL)/configure \
	--prefix=/ \
	--enable-debug \
	--enable-optimize \
       	CROSS_COMPILE=$(OM_TARG)- CC=$(OM_TARG)-gcc 
	$(MAKE) -C $(SPWD)/$(OM_MUSL)
	$(MAKE) -C $(SPWD)/$(OM_MUSL) install DESTDIR=$(OM_PREF)/$(OM_TARG)/

	-@rm -rf $(SPWD)/$(OM_GCC)-build-second
	@mkdir -p $(SPWD)/$(OM_GCC)-build-second
	@cd $(SPWD)/$(OM_GCC)-build-second && $(SPWD)/$(OM_GCC)/configure \
	--prefix=$(OM_PREF) \
	--target=$(OM_TARG) \
	--enable-languages=$(OM_LANGS) \
	--disable-libmudflap \
	--disable-libsanitizer \
	--with-headers=$(OM_PREF)/$(OM_TARG)/include \
	--disable-nls \
	--with-ld=$(OM_PREF)/bin/$(OM_TARG)-ld \
	--with-as=$(OM_PREF)/bin/$(OM_TARG)-as \
	$(OM_CONF) 

	$(MAKE) -C $(SPWD)/$(OM_GCC)-build-second
	$(MAKE) -C $(SPWD)/$(OM_GCC)-build-second install

	$(MAKE) -C $(SPWD)/$(OM_MUSL) install DESTDIR=$(OM_PREF)/$(OM_TARG)/

uclibc_stage_one: install_path

	@cd src && test -e $(OM_UCLIBC).$(OM_SFX) || wget $(UCLIBC_MRR)/$(OM_UCLIBC).$(OM_SFX)
	@rm -rf $(SPWD)/$(OM_UCLIBC)
	@tar -xf $(OM_SRC)/$(OM_UCLIBC).$(OM_SFX) -C $(SPWD)
	-@cd $(SPWD)/$(OM_UCLIBC) 
	@cd $(SPWD)/$(OM_UCLIBC) && mkdir -p $(OM_PREF)/$(OM_TARG)/usr/src/linux
	@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" "KERNEL_HEADERS=\"$(OM_PREF)/$(OM_TARG)/include\"" >> .config 
	@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" "TARGET_$(OM_BACKWARDCOMPAT)=y" >> .config 
	@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" "RUNTIME_PREFIX=\"$(OM_PREF)/$(OM_TARG)/\"" >> .config 
	@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" "DEVEL_PREFIX=\"$(OM_PREF)/$(OM_TARG)/\"" >> .config 
	@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" "HAVE_SHARED=y" >> .config 
	@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" "UCLIBC_HAS_THREADS_NATIVE=y" >> .config 
	#@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" "UCLIBC_HAS_LINUXTHREADS=y" >> .config
	#@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" "UCLIBC_HAS_UTMPX=y" >> .config 
	#@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" "UCLIBC_HAS_UTMP=y" >> .config 
	#@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" "UCLIBC_SUSV3_LEGACY=y" >> .config 
	#@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" "UCLIBC_HAS_WCHAR=y" >> .config 
	#@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" "UCLIBC_HAS_NETWORK_SUPPORT=y" >> .config 
	#@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" "UCLIBC_HAS_FULL_RPC=y" >> .config 
	#@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" "UCLIBC_HAS_REENTRANT_RPC=y" >> .config 
	#@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" "UCLIBC_HAS_RPC=y" >> .config
	-@cd $(SPWD)/$(OM_UCLIBC) && printf "%s\n" y | make CROSS="$(OM_TARG)-"  oldconfig
	$(MAKE) -C $(SPWD)/$(OM_UCLIBC) CROSS="$(OM_TARG)-" PREFIX="" install

uclibc_stage_two: install_path 

	@rm -rf $(SPWD)/$(OM_GCC)-build-second
	@mkdir -p $(SPWD)/$(OM_GCC)-build-second

	-@cd $(SPWD)/$(OM_GCC)-build-second && $(SPWD)/$(OM_GCC)/configure \
	--prefix=$(OM_PREF) \
	--target=$(OM_TARG) \
	--enable-languages=$(OM_LANGS) \
	--disable-werror \
	--disable-libquadmath \
	--disable-libmudflap \
	--disable-libsanitizer \
	--with-ld="$(OM_PREF)/bin/$(OM_TARG)-ld" \
	--with-as="$(OM_PREF)/bin/$(OM_TARG)-as" \
	--with-headers="$(OM_PREF)/$(OM_TARG)/include" \
	--disable-nls \
	$(OM_CONF)
	#--disable-threads \

	$(MAKE) -i -C $(SPWD)/$(OM_GCC)-build-second

uclibc_stage_three: install_path

	$(MAKE) -i -C $(SPWD)/$(OM_GCC)-build-second install

gnu_stage_one: install_path

	@cd src && test -e $(OM_GLIBC).$(OM_SFX) || wget $(GNU_MRR)/glibc/$(OM_GLIBC).$(OM_SFX)
	@rm -rf $(SPWD)/$(OM_GLIBC)-build/
	@tar -xf $(OM_SRC)/$(OM_GLIBC).$(OM_SFX) -C $(SPWD)
	-@mkdir -p $(SPWD)/$(OM_GLIBC)-build

	@cd $(SPWD)/$(OM_GLIBC)-build && $(SPWD)/$(OM_GLIBC)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_TARG) \
	--build=$(MACHTYPE) \
	--target=$(OM_TARG) \
	--with-headers=$(OM_PREF)/$(OM_TARG)/include \
	--disable-werror \
	--enable-obsolete-rpc \
	--enable-kernel=2.6.32 \
	--enable-add-ons \
	libc_cv_forced_unwind=yes \
	libc_cv_c_cleanup=yes \
	libc_cv_ctors_header=yes \
	$(OM_CONF)

	$(MAKE) -C $(SPWD)/$(OM_GLIBC)-build 
	$(MAKE) -C $(SPWD)/$(OM_GLIBC)-build install

gnu_stage_two:
	#-@touch $(OM_PREF)/$(OM_TARG)/include/gnu/stubs.h 

	@rm -rf $(SPWD)/$(OM_GCC)-build-second
	@mkdir -p $(SPWD)/$(OM_GCC)-build-second
	-@cd $(SPWD)/$(OM_GCC)-build-second && $(SPWD)/$(OM_GCC)/configure \
	--prefix=$(OM_PREF) \
	--target=$(OM_TARG) \
	--enable-languages=$(OM_LANGS) \
	--with-headers=$(OM_PREF)/$(OM_TARG)/include \
	--with-ld=$(OM_PREF)/bin/$(OM_TARG)-ld \
	--with-as=$(OM_PREF)/bin/$(OM_TARG)-as \
	--disable-werror \
	--disable-shared \
	$(OM_CONF) 

	$(MAKE) -C $(OM_GCC)-build-second 
	$(MAKE) -C $(OM_GCC)-build-second install 

gnu_stage_three:

	$(MAKE) -C $(SPWD)/$(OM_GLIBC)-build
	$(MAKE) -C $(SPWD)/$(OM_GLIBC)-build install 

ncurses_host: install_path 

	@cd src && test -e $(OM_NCURSES).$(OM_SFX) || wget $(OM_NCURSES_MRR)/$(OM_NCURSES).$(OM_SFX)
	rm -rf $(OM_NCURSES) 
	@tar -xf $(OM_SRC)/$(OM_NCURSES).$(OM_SFX) -C $(SPWD) 
	cd $(SPWD)/$(OM_NCURSES) && $(INACROSS) $(SPWD)/$(OM_NCURSES)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_HOST) \
	--enable-widec \
	$(INACROSS)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_NCURSES)/
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_NCURSES)/ install

	cp -r $(OM_PREF)/$(OM_TARG)/include/ncurses/* $(OM_PREF)/$(OM_TARG)/include/
	cd $(OM_PREF)/$(OM_TARG)/lib/ && ln libncurses.a libtinfo.a

htop_target: install_path 

	@cd src && test -e $(OM_HTOP).$(OM_SFX) || wget $(OM_HTOP_MRR)/$(OM_HTOP).$(OM_SFX)
	-$(MAKE) -i ncurses_host
	rm -rf $(OM_HTOP)
	tar -xf $(OM_SRC)/$(OM_HTOP).$(OM_SFX) -C $(SPWD)
	mkdir -p $(OM_LIBROOT)
	-cd $(SPWD)/$(OM_HTOP) && $(SPWD)/$(OM_HTOP)/autogen.sh $(INACROSS)
	-cd $(SPWD)/$(OM_HTOP) && $(INACROSS) $(SPWD)/$(OM_HTOP)/configure \
	--prefix=$(OM_LIBROOT) \
	--host=$(OM_HOST) 
	#--disable-unicode \
	#$(INACROSS) 
	
	$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_HTOP)
	$(MAKE) -C $(SPWD)/$(OM_HTOP) install 

coreutils_target: install_path

	@cd src && test -e $(OM_COREUTILS).$(OM_SFX) || wget $(OM_COREUTILS_MRR)/$(OM_COREUTILS).$(OM_SFX)
	rm -rf $(OM_COREUTILS)
	tar -xf $(OM_SRC)/$(OM_COREUTILS).$(OM_SFX) -C $(SPWD) 
	mkdir -p $(OM_LIBROOT)
	-cd $(SPWD)/$(OM_COREUTILS) && $(SPWD)/$(OM_COREUTILS)/configure \
	--prefix=$(OM_LIBROOT) \
	--host=$(OM_HOST) \
	$(INACROSS) \
	PERL=missing \
	MAKEINFO=false

	-$(MAKE) -C $(SPWD)/$(OM_COREUTILS)
	-$(MAKE) -C $(SPWD)/$(OM_COREUTILS) install

vim_target: install_path 

	@cd src && test -e $(OM_VIM).$(OM_SFX) || wget $(GENERIC_MRR)/$(OM_VIM).$(OM_SFX)
	-$(MAKE) -i ncurses_host
	rm -rf $(OM_VIM)
	tar -xf $(OM_SRC)/$(OM_VIM).$(OM_SFX) -C $(SPWD)
	mkdir -p $(OM_LIBROOT)
	-cd $(SPWD)/$(OM_VIM) && $(SPWD)/$(OM_VIM)/configure \
	--prefix=$(OM_LIBROOT) \
	--host=$(OM_TARG) \
	$(INACROSS) \
	vim_cv_toupper_broken=yes \
	--with-tlib=ncurses \
	vim_cv_terminfo=yes \
	vim_cv_tty_group=yes \
	vim_cv_tty_mode=yes  \
	vim_cv_getcwd_broken=yes \
	vim_cv_stat_ignores_slash=yes \
	vim_cv_memmove_handles_overlap=yes 

	-$(MAKE) -C $(SPWD)/$(OM_VIM) 
	-$(MAKE) -C $(SPWD)/$(OM_VIM) install

utillinux_target: install_path

	rm -rf $(OM_UTILLINUX)
	tar -xf $(OM_SRC)/$(OM_UTILLINUX).$(OM_SFX) -C $(SPWD)
	 mkdir -p $(OM_LIBROOT)
	-cd $(SPWD)/$(OM_UTILLINUX) && $(SPWD)/$(OM_UTILLINUX)/configure \
	--prefix=$(OM_LIBROOT) \
	--host=$(OM_TARG) \
	$(INACROSS) \
	MAKEINFO=/bin/false

	-$(MAKE) -C $(SPWD)/$(OM_UTILLINUX) 
	-$(MAKE) -C $(SPWD)/$(OM_UTILLINUX) install

dhcp_target: install_path

	@cd src && test -e $(OM_DHCP).$(OM_SFX) || wget $(GENERIC_MRR)/$(OM_DHCP).$(OM_SFX)
	rm -rf $(OM_DHCP)
	tar -xf $(OM_SRC)/$(OM_DHCP).$(OM_SFX) -C $(SPWD)
	 mkdir -p $(OM_LIBROOT)
	-cd $(SPWD)/$(OM_DHCP) && $(SPWD)/$(OM_DHCP)/configure \
	--prefix=$(OM_LIBROOT) \
	--host=$(OM_TARG) \
	--target=$(OM_TARG) ac_cv_file__dev_random=yes 

	-$(MAKE) -C $(SPWD)/$(OM_DHCP)
	-$(MAKE) -C $(SPWD)/$(OM_DHCP) install


native_linux_target: install_path 

	-mkdir kernel
	-$(MAKE) install_path
	-$(MAKE) linux_stage
	@test -d $(OM_LINUX) || tar -xf $(OM_SRC)/$(OM_LINUX).$(OM_SFX) -C $(SPWD)
	-mkdir -p $(OM_LIBROOT)/boot
	-$(MAKE) -C $(SPWD)/$(OM_LINUX) ARCH=$(OM_BACKWARDCOMPAT) defconfig O=$(KBUILDAREA)
	#sed -i 's|# CONFIG_8139CP is not set|CONFIG_8139CP=y|g' $(SPWD)/$(OM_LINUX)/.config
	sed -i 's|# CONFIG_8139CP is not set|CONFIG_8139CP=y|g' $(KBUILDAREA)/.config
	-$(MAKE) -C $(SPWD)/$(OM_LINUX) \
	ARCH=$(OM_LINUXARCH) \
	KCONFIG_CONFIG=$(KBUILDAREA)/.config \
	O=$(KBUILDAREA)


	-$(MAKE) -C $(SPWD)/$(OM_LINUX) \
	INSTALL_PATH=$(OM_LIBROOT)/boot \
	install ARCH=$(OM_LINUXARCH) \
	O=$(KBUILDAREA)

	-cd $(OM_LIBROOT)/boot && ln vmlinu* vmlinuz

linux_target: install_path 

	
	@test -d $(OM_LINUX) || tar -xf $(OM_SRC)/$(OM_LINUX).$(OM_SFX) -C $(SPWD)
	-mkdir -p $(OM_LIBROOT)/boot
	-mkdir -p $(KBUILDAREA)-target
	-$(MAKE) -C $(SPWD)/$(OM_LINUX) ARCH=$(OM_LINUXARCH) defconfig O=$(KBUILDAREA)-target
	#sed -i 's|# CONFIG_8139CP is not set|CONFIG_8139CP=y|g' $(SPWD)/$(OM_LINUX)/.config
	sed -i 's|# CONFIG_8139CP is not set|CONFIG_8139CP=y|g' $(KBUILDAREA)-target/.config
	-$(MAKE) -C $(SPWD)/$(OM_LINUX) \
	ARCH=$(OM_LINUXARCH) \
	CROSS_COMPILE=$(OM_TARG)- \
	KCONFIG_CONFIG=$(KBUILDAREA)-target/.config \
	O=$(KBUILDAREA)-target


	-$(MAKE) -C $(SPWD)/$(OM_LINUX) \
	INSTALL_PATH=$(OM_LIBROOT)/boot \
	install ARCH=$(OM_LINUXARCH) \
	CROSS_COMPILE=$(OM_TARG)- \
	O=$(KBUILDAREA)-target

	-cd $(OM_LIBROOT)/boot && ln vmlinu* vmlinuz

dash_target: install_path

	-rm -rf $(OM_DASH)
	-@tar -xf $(OM_SRC)/$(OM_DASH).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_DASH) && $(SPWD)/$(OM_DASH)/configure \
	--prefix=$(OM_LIBROOT) \
	--host=$(OM_TARG) 
      
	-$(MAKE) -C $(SPWD)/$(OM_DASH)
	-$(MAKE) -C $(SPWD)/$(OM_DASH) install

strace_target: install_path

	-rm -rf $(OM_STRACE)
	-@tar -xf $(OM_SRC)/$(OM_STRACE).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_STRACE) && $(SPWD)/$(OM_STRACE)/configure \
	--prefix=$(OM_LIBROOT) \
	--host=$(OM_TARG)

	-$(MAKE) -C $(SPWD)/$(OM_STRACE)
	-$(MAKE) -C $(SPWD)/$(OM_STRACE) install 

busybox_target: install_path

	@cd src && test -e omnicross-busybox-minimal.1 || wget $(GENERIC_MRR)/omnicross-busybox-minimal.1
	@cd src && test -e $(OM_BUSYBOX).$(OM_SFX) || wget $(GENERIC_MRR)/$(OM_BUSYBOX).$(OM_SFX)
	-mkdir -p $(OM_LIBROOT)
	-rm -rf $(OM_BUSYBOX)
	-@tar -xf $(OM_SRC)/$(OM_BUSYBOX).$(OM_SFX) -C $(SPWD) 
	cp $(OM_SRC)/omnicross-busybox-minimal.1 $(OM_BUSYBOX)/.config
	sed -i 's|CONFIG_SPLIT=y|CONFIG_SPLIT=n|g' $(SPWD)/$(OM_BUSYBOX)/.config
	-$(MAKE) -C $(SPWD)/$(OM_BUSYBOX) CONFIG_PREFIX=$(OM_LIBROOT) $(INACROSS)
	-$(MAKE) -C $(SPWD)/$(OM_BUSYBOX) CONFIG_PREFIX=$(OM_LIBROOT) $(INACROSS) install

zlib_host: install_path

	-rm -rf $(OM_ZLIB)
	-@tar -xf $(OM_SRC)/$(OM_ZLIB).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_ZLIB) && $(INACROSS) $(SPWD)/$(OM_ZLIB)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) 
	
	-$(MAKE) -C $(SPWD)/$(OM_ZLIB)
	-$(MAKE) -C $(SPWD)/$(OM_ZLIB) install

dropbear_target: install_path

	$(MAKE) -i zlib_host

	-@tar -xf $(OM_SRC)/$(OM_DROPBEAR).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_DROPBEAR) && $(INACROSS) $(SPWD)/$(OM_DROPBEAR)/configure \
	--prefix=/ \
	--target=$(OM_SUBTARG) \
	--host=$(OM_SUBTARG) 
	
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_DROPBEAR)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_DROPBEAR) install DESTDIR=$(OM_LIBROOT)

gmp_host: install_path

	@cd src && test -e $(OM_GMP).$(OM_SFX) || wget $(GNU_MRR)/gmp/$(OM_GMP).$(OM_SFX)
	-@tar -xf $(OM_SRC)/$(OM_GMP).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_GMP) && $(INACROSS) $(SPWD)/$(OM_GMP)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_SUBTARG)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GMP)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GMP) install

mpc_host: install_path

	@cd src && test -e $(OM_MPC).$(OM_SFX) || wget $(GNU_MRR)/mpc/$(OM_MPC).$(OM_SFX)
	-@tar -xf $(OM_SRC)/$(OM_MPC).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_MPC) && $(INACROSS) $(SPWD)/$(OM_MPC)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_SUBTARG)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_MPC)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_MPC) install

mpfr_host: install_path

	@cd src && test -e $(OM_MPFR).$(OM_SFX) || wget $(GNU_MRR)/mpfr/$(OM_MPFR).$(OM_SFX)
	-@tar -xf $(OM_SRC)/$(OM_MPFR).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_MPFR) && $(INACROSS) $(SPWD)/$(OM_MPFR)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_SUBTARG)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_MPFR)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_MPFR) install 

binutils_target: install_path

	rm -rf $(OM_BINUTILS)-build-target 
	mkdir -p $(OM_BINUTILS)-build-target
	@tar -xf $(OM_SRC)/$(OM_BINUTILS).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_BINUTILS)-build-target && $(INACROSS) $(SPWD)/$(OM_BINUTILS)/configure \
	--prefix=$(OM_LIBROOT) \
	--host=$(OM_TARG) \
	--disable-werror 
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_BINUTILS)-build-target
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_BINUTILS)-build-target install


gnu_gcc_target: install_path

	$(MAKE) -i gmp_host
	$(MAKE) -i mpfr_host
	$(MAKE) -i mpc_host
	-$(MAKE) -i binutils_target

	rm -rf $(OM_GCC) $(OM_GCC)-build-target
	mkdir -p $(OM_GCC)-build-target
	@tar -xf $(OM_SRC)/$(OM_GCC).$(OM_SFX) -C $(SPWD)

ifeq ($(OM_LIBC),musl)
	-@cd $(SPWD)/$(OM_GCC) && patch -p1 < $(SPWD)/patches/$(OM_GCC)-musl.diff
endif

	-cd $(SPWD)/$(OM_GCC)-build-target && $(INACROSS) $(SPWD)/$(OM_GCC)/configure \
	--prefix=$(OM_LIBROOT) \
	--enable-languages=c \
	--disable-libssp \
	--disable-nls \
	--disable-libquadmath \
	--disable-threads \
	--disable-decimal-float \
	--disable-shared \
	--disable-libmudflap \
	--disable-libgomp \
	--disable-libatomic \
	--disable-werror \
	--host=$(OM_TARG) \
	--target=$(OM_TARG)

	-CFLAGS="-O0 -g0" CXXFLAGS="-O0 -g0" $(INACROSS) $(MAKE) -i -C $(SPWD)/$(OM_GCC)-build-target

	-CFLAGS="-O0 -g0" CXXFLAGS="-O0 -g0" $(INACROSS) $(MAKE) -i -C $(SPWD)/$(OM_GCC)-build-target install

	-$(INACROSS) $(MAKE) -i -C $(SPWD)/$(OM_GCC)-build-target install DESTDIR=$(OM_LIBROOT)
	cp $(OM_PREF)/lib/gcc/$(OM_TARG)/4.9.2/* $(OM_LIBROOT)/lib/gcc/$(OM_TARG)/4.9.2/

	#-mkdir -p $(OM_LIBROOT)/lib/gcc/$(OM_TARG)/4.9.2/
	#cp $(OM_PREF)/lib/gcc/$(OM_TARG)/4.9.2/* $(OM_LIBROOT)/lib/gcc/$(OM_TARG)/4.9.2/


gcc_target: install_path

	$(MAKE) -i gmp_host
	$(MAKE) -i mpfr_host
	$(MAKE) -i mpc_host

	-$(MAKE) -i binutils_target
	rm -rf $(OM_GCC) $(OM_GCC)-build-target
	mkdir -p $(OM_GCC)-build-target
	@tar -xf $(OM_SRC)/$(OM_GCC).$(OM_SFX) -C $(SPWD)

ifeq ($(OM_LIBC),musl)
	-@cd $(SPWD)/$(OM_GCC) && patch -p1 < $(SPWD)/patches/$(OM_GCC)-musl.diff
endif
	# in-tree binutils
	#@tar -xf $(OM_SRC)/$(OM_BINUTILS).$(OM_SFX) -C $(SPWD)
	#@cd $(SPWD)/$(OM_GCC) && ln -s $(SPWD)/$(OM_BINUTILS)/* .

	-cd $(SPWD)/$(OM_GCC)-build-target && $(INACROSS) $(SPWD)/$(OM_GCC)/configure \
	--prefix=$(OM_LIBROOT) \
	--enable-languages=c,c++ \
	--disable-libssp \
	--host=$(OM_TARG) \
	--target=$(OM_TARG)

	-CFLAGS="-O0 -g0" CXXFLAGS="-O0 -g0" $(INACROSS) $(MAKE) -i -C $(SPWD)/$(OM_GCC)-build-target

	-$(INACROSS) $(MAKE) -i -C $(SPWD)/$(OM_GCC)-build-target install
	-$(INACROSS) $(MAKE) -i -C $(SPWD)/$(OM_GCC)-build-target install DESTDIR=$(OM_LIBROOT)
	
	
e2fsprogs_host: install_path

	rm -rf $(OM_E2FSPROGS)
	mkdir -p $(OM_E2FSPROGS)
	@tar -xf $(OM_SRC)/$(OM_E2FSPROGS).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_E2FSPROGS) && $(INACROSS) $(SPWD)/$(OM_E2FSPROGS)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_TARG) \
	--target=$(OM_TARG) \
	 --disable-tls \
	MAKEINFO=false \
	TEXI2DVI=false

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_E2FSPROGS)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_E2FSPROGS) install-libs

e2fsprogs_target: install_path

	rm -rf $(OM_E2FSPROGS)
	rm -rf $(OM_E2FSPROGS)-build-target
	mkdir -p $(OM_E2FSPROGS)-build-target 
	
	@tar -xf $(OM_SRC)/$(OM_E2FSPROGS).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_E2FSPROGS)-build-target && $(INACROSS) $(SPWD)/$(OM_E2FSPROGS)/configure \
	--prefix=/ \
	--host=$(OM_TARG) \
	--target=$(OM_TARG) \
	--bindir=/bin \
	--with-root-prefix="" \
	--disable-werror \
	MAKEINFO=false \
	TEXI2DVI=false 

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_E2FSPROGS)-build-target
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_E2FSPROGS)-build-target install DESTDIR=$(OM_LIBROOT)

parted_target: install_path

	@cd src && test -e $(OM_PARTED).$(OM_SFX) || wget $(OM_PARTED_MRR)/$(OM_PARTED).$(OM_SFX)
	rm -rf $(OM_PARTED)
	mkdir -p $(OM_PARTED)
	@tar -xf $(OM_SRC)/$(OM_PARTED).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_PARTED) && $(INACROSS) $(SPWD)/$(OM_PARTED)/configure \
	--host=$(OM_SUBTARG)-pc-linux-gnu \
	--target=$(OM_SUBTARG)-pc-linux-gnu \
	--without-readline \
	--disable-device-mapper 

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_PARTED)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_PARTED) install  DESTDIR=$(OM_LIBROOT)

grub_target: install_path

	@cd patches && test -e grub-2.00-musl.patch || wget $(GENERIC_MRR)/grub-2.00-musl.patch
	@cd src && test -e $(OM_GRUB).$(OM_SFX) || wget $(GNU_MRR)/grub/$(OM_GRUB).$(OM_SFX)
	rm -rf $(OM_GRUB)
	mkdir -p $(OM_GRUB) 
	-@tar -xf $(OM_SRC)/$(OM_GRUB).$(OM_SFX) -C $(SPWD) 
ifeq ($(OM_LIBC),musl)
	-cd $(SPWD)/$(OM_GRUB) &&  patch -p1 < $(SPWD)/patches/grub-2.00-musl.patch
endif
	-cd $(SPWD)/$(OM_GRUB) && $(INACROSS) $(SPWD)/$(OM_GRUB)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG) \
	--disable-werror \
	--disable-grub-emu-usb \
	--disable-efiemu \
	--disable-device-mapper \
	--disable-hostdisk

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GRUB) 

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GRUB) install  DESTDIR=$(OM_LIBROOT) 

populate:

	-@mkdir -p $(SPWD)/src/
	-@mkdir -p $(SPWD)/patches/

install: install_path 

	-@mkdir -p $(DESTDIR)/
ifeq ($(OM_LIBC),musl)
	$(MAKE) -C $(SPWD)/$(OM_MUSL) install
endif

ifeq ($(OM_LIBC),musleabi)
	$(MAKE) -C $(SPWD)/$(OM_MUSL) install
endif

ifeq ($(OM_LIBC),uclibc)
	$(MAKE) -C "$(SPWD)/$(OM_UCLIBC)" \
	CROSS="$(OM_TARG)-" \
	RUNTIME_PREFIX="/" \
	DEVEL_PREFIX="/" \
	install
endif

ifeq ($(OM_LIBC),gnu)
	
	mkdir -p $(OM_GLIBC)-build-final
	 @cd $(SPWD)/$(OM_GLIBC)-build-final && $(SPWD)/$(OM_GLIBC)/configure \
	--prefix=/ \
	--host=$(OM_TARG) \
	--build=$(MACHTYPE) \
	--target=$(OM_TARG) \
	--with-headers=$(OM_PREF)/$(OM_TARG)/include \
	--disable-werror \
	--enable-obsolete-rpc \
	--enable-kernel=2.6.32 \
	--enable-add-ons \
	libc_cv_forced_unwind=yes \
	libc_cv_c_cleanup=yes \
	libc_cv_ctors_header=yes \
	$(OM_CONF)
	$(MAKE) -C $(SPWD)/$(OM_GLIBC)-build-final install  
endif

install_full:

	#$(MAKE) -C $(SPWD)/$(OM_MUSL) install
	mkdir -p $(DESTDIR)/
	@cp -r $(OM_PREF)/$(OM_TARG)/* $(DESTDIR)/


bootstrap:

	-mkdir -p $(OM_LIBROOT)
	-cp -r ../tree/* $(OM_LIBROOT)
	-$(MAKE) -i gcc_target
	-$(MAKE) -i linux_target
	-$(MAKE) -i install DESTDIR=$(OM_LIBROOT)
	-$(MAKE) -i busybox_target
	-$(MAKE) -i coreutils_target
	-$(MAKE) -i strace_target
	-rm $(OM_LIBROOT)/usr/bin/awk
	-$(MAKE) -i gawk_target
	-$(MAKE) -i make_target
	-$(MAKE) -i m4_target
	-cp -r ../cbin ../clib ../editor ../irc $(OM_LIBROOT)
	-mkdir -p $(OM_LIBROOT)/toolchain/src/
	-cp Makefile $(OM_LIBROOT)/toolchain/
	-cp -r patches $(OM_LIBROOT)/toolchain/

bootstrap_src:

	-mkdir -p $(OM_BOOTSRC)
	-cp -r $(SPWD)/../tree $(OM_BOOTSRC)
	-cp $(OM_SRC)/$(OM_LINUX).$(OM_SFX) $(OM_BOOTSRC)
	-cp $(OM_SRC)/$(OM_BINUTILS).$(OM_SFX) $(OM_BOOTSRC)
	-cp $(OM_SRC)/$(OM_GCC).$(OM_SFX) $(OM_BOOTSRC)
	-cp $(OM_SRC)/$(OM_GMP).$(OM_SFX) $(OM_BOOTSRC)
	-cp $(OM_SRC)/$(OM_MPFR).$(OM_SFX) $(OM_BOOTSRC)
	-cp $(OM_SRC)/$(OM_MPC).$(OM_SFX) $(OM_BOOTSRC)
	-cp $(OM_SRC)/$(OM_MUSL).$(OM_SFX) $(OM_BOOTSRC)
	-cp $(OM_SRC)/$(OM_PATCH).$(OM_SFX) $(OM_BOOTSRC)
	-cp $(OM_SRC)/$(OM_GAWK).$(OM_SFX) $(OM_BOOTSRC)
	-cp $(OM_SRC)/$(OM_MAKE).$(OM_SFX) $(OM_BOOTSRC)
	-cp $(OM_SRC)/$(OM_BUSYBOX).$(OM_SFX) $(OM_BOOTSRC)
	-cp $(OM_SRC)/$(OM_M4).$(OM_SFX) $(OM_BOOTSRC)
	-cp $(OM_SRC)/omnicross-busybox-minimal.1 $(OM_BOOTSRC)
	-cp $(OM_SRC)/$(OM_COREUTILS).$(OM_SFX) $(OM_BOOTSRC)
	-cp $(OM_SRC)/$(OM_GENEXTFS).$(OM_SFX) $(OM_BOOTSRC)
	-$(MAKE) -i genext2fs_host
	$(OM_PREF)/bin/genext2fs -b 1000000 -d $(OM_BOOTSRC) bootsrc.img


test_i386:

	-$(MAKE) -i bootstrap_src
	dd if=/dev/zero of=drive.img bs=1M count=8000
	dd if=/dev/zero of=output.img bs=1M count=1000
	/sbin/mkfs.ext2 -F drive.img
	/sbin/mkfs.ext2 -F output.img
	$(OM_PREF)/bin/genext2fs -b 1000000 -d $(OM_LIBROOT) binary.img

	qemu-system-$(OM_BACKWARDCOMPAT) -m 1000 -curses \
	-kernel $(OM_LIBROOT)/boot/vmlinuz \
	-append "root=/dev/sda" \
	-hda binary.img \
	-hdb bootsrc.img \
	-hdc drive.img \
	-hdd output.img

testonly:

	-$(MAKE) -i bootstrap_src
	$(OM_PREF)/bin/genext2fs -b 1000000 -d $(OM_LIBROOT) binary.img
	qemu-system-$(OM_BACKWARDCOMPAT) -m 1000 -curses \
	-kernel $(OM_LIBROOT)/boot/vmlinuz \
	-append "root=/dev/sda" \
	-hda binary.img \
	-hdb bootsrc.img \
	-hdc drive.img \
	-hdd output.img

test_final:

	dd if=/dev/zero of=newoutput.img bs=1M count=1000
	/sbin/mkfs.ext2 -F newoutput.img
	
	dd if=/dev/zero of=newdrive.img bs=1M count=8000
	/sbin/mkfs.ext2 -F newdrive.img

	qemu-system-$(OM_BACKWARDCOMPAT) -m 1000 -curses \
	-kernel $(OM_LIBROOT)/boot/vmlinuz \
	-append "root=/dev/sda init=/bin/sh" \
	-hda output.img \
	-hdb bootsrc.img \
	-hdc newdrive.img \
	-hdd newoutput.img

test_aarch64:

	-qemu-system-aarch64 \
	-machine virt \
	-cpu cortex-a57 \
	-machine type=virt \
	-nographic -smp 1 \
	-m 2048 \
	-kernel libroot/boot/vmlinuz \
	-initrd backup.cpio.gz \
	-append "root=/dev/ram rdinit=/bin/sh console=ttyAMA0"

test_aarch64_2:

	
	-mkdir $(PWD)/libroot
	-$(MAKE)
	-$(MAKE) install DESTDIR=$(PWD)/libroot
	#-@cp -r $(OM_PREF)/$(OM_TARG)/* $(OM_LIBROOT)
	-$(MAKE) linux_target > logfile
	-$(MAKE) busybox_target >> logfile

	-cd libroot && find . | cpio -H newc -o > ../backup.cpio
	-cat backup.cpio | gzip > backup.cpio.gz
	-qemu-system-aarch64 \
	-machine virt \
	-cpu cortex-a57 \
	-machine type=virt \
	-nographic -smp 1 \
	-m 2048 \
	-kernel libroot/boot/vmlinuz \
	-initrd backup.cpio.gz \
	-append "init=/bin/sh root=/dev/ram rdinit=/bin/sh console=ttyAMA0"

gnu_compiler:

	-$(MAKE) -i gnu_gcc_target
	-$(MAKE) -i install DESTDIR=$(OM_LIBROOT)

distribute:

	$(MAKE) -i ncurses_host
	$(MAKE) -i zlib_host
	$(MAKE) -i dash_target
	$(MAKE) -i linux_target
	$(MAKE) -i busybox_target
	$(MAKE) -i htop_target
	$(MAKE) -i dhcp_target
	$(MAKE) -i coreutils_target
	$(MAKE) -i dropbear_target
	$(MAKE) -i strace_target
	$(MAKE) -i vim_target
	$(MAKE) -i gmp_host
	$(MAKE) -i mpfr_host
	$(MAKE) -i mpc_host
	$(MAKE) -i e2fsprogs_host
	$(MAKE) -i e2fsprogs_target
	$(MAKE) -i parted_target
	$(MAKE) -i grub_target

	-@cp -r $(OM_PREF)/$(OM_TARG)/* $(OM_LIBROOT)
	# this needs a bootloader and filesystem creation mechanism
	# one used to be supplied but it was not of sufficient stability

clean:

	@rm -rf $(SPWD)/$(OM_MUSL)/
	@rm -rf $(SPWD)/$(OM_UCLIBC)/
	@rm -rf $(SPWD)/$(OM_GLIBC)/
	@rm -rf $(SPWD)/$(OM_GLIBC)-build
	@rm -rf $(SPWD)/$(OM_GCC)/
	@rm -rf $(SPWD)/$(OM_GCC)-build/
	@rm -rf $(SPWD)/$(OM_GCC)-build-target/
	@rm -rf $(SPWD)/$(OM_GCC)-build-second/
	@rm -rf $(SPWD)/$(OM_BINUTILS)/
	@rm -rf $(SPWD)/$(OM_BINUTILS)-build/
	@rm -rf $(SPWD)/$(OM_BINUTILS)-build-target/
	@rm -rf $(SPWD)/$(OM_GMP)/
	@rm -rf $(SPWD)/$(OM_GMP)-build/
	@rm -rf $(SPWD)/$(OM_MPC)/
	@rm -rf $(SPWD)/$(OM_MPC)-build/
	@rm -rf $(SPWD)/$(OM_MPFR)/
	@rm -rf $(SPWD)/$(OM_MPFR)-build/
	@rm -rf $(SPWD)/$(OM_LINUX)/
	@rm -rf $(SPWD)/toolchain/
	@rm -rf $(SPWD)/works
	@rm -rf $(SPWD)/works.c
	@rm -rf $(OM_VIM)
	@rm -rf $(OM_COREUTILS)
	@rm -rf $(OM_BUSYBOX)
	@rm -rf $(OM_BUTILLINUX)
	@rm -rf $(OM_DASH)
	@rm -rf $(OM_HTOP)
	@rm -rf $(OM_DROPBEAR)
	@rm -rf $(OM_DHCP)
	@rm -rf $(OM_STRACE)
	@rm -rf $(OM_ZLIB)
	@rm -rf $(OM_NCURSES)
	@rm -rf $(OM_PARTED)
	@rm -rf $(OM_GRUB)
	@rm -rf $(OM_E2FSPROGS)
	@rm -rf $(OM_PYTHON)
	@rm -rf $(OM_PERL)
	@rm -rf $(OM_XORRISO)
	@rm -rf $(OM_CPIO)
	@rm -rf $(OM_BC)
	@rm -rf $(OM_GROFF)
	@rm -rf $(OM_NANO)
	@rm -rf $(OM_GIT)
	@rm -rf $(OM_BISON)
	@rm -rf $(OM_BASH)
	@rm -rf $(OM_GDB)
	@rm -rf $(OM_GDB)-build-host
	@rm -rf $(OM_OCRAD)
	@rm -rf $(OM_TARG)
	@rm -rf $(OM_TARG)-toolchain
	@rm -rf kernel


buildenvars:

	@printf "%s\n" CC="${OM_TARG}-gcc "
	@printf "%s\n" LD="${OM_TARG}-ld "
	@printf "%s\n" RANLIB="${OM_TARG}-ranlib "
	@printf "%s\n" AS="${OM_TARG}-as "
	@printf "%s\n" CPP="${OM_TARG}-cpp "
	@printf "%s\n" CXX="${OM_TARG}-g++ "
	@printf "%s\n" NM="${OM_TARG}-nm "
	@printf "%s\n" STRIP="${OM_TARG}-strip "
	@printf "%s\n" PATH="${PATH}:${OM_PREF}/bin/ "
	@printf "%s\n" HOST_CC="gcc "
	@printf "%s\n" CROSS_COMPILE="${OM_TARG}- "
	@printf "%s\n" CC_FOR_BUILD="gcc "

clearenvars:

	@printf "%s\n" CC="gcc"
	@printf "%s\n" LD="ld"
	@printf "%s\n" RANLIB="ranlib"
	@printf "%s\n" AS="as"
	@printf "%s\n" CPP="cpp"
	@printf "%s\n" CXX="g++"
	@printf "%s\n" NM="nm"
	@printf "%s\n" STRIP="strip"
	@printf "%s\n" PATH="/usr/local/bin:/usr/bin:/bin:/usr/local/games:/usr/games" 


links_target: install_path

	$(MAKE) -i ncurses_host
	-rm -rf $(OM_LINKS)
	-mkdir -p $(OM_LINKS)
	@tar -xf $(OM_SRC)/$(OM_LINKS).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_LINKS) && $(INACROSS) $(SPWD)/$(OM_LINKS)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_LINKS)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_LINKS) install  DESTDIR=$(OM_LIBROOT) 


gdb_target: install_path

	@cd src && test -e $(OM_GDB).$(OM_SFX) || wget $(GNU_MRR)/gdb/$(OM_GDB).$(OM_SFX)
	-rm -rf $(OM_GDB)
	-mkdir -p $(OM_GDB)
	@tar -xf $(OM_SRC)/$(OM_GDB).$(OM_SFX) -C $(SPWD) 
	
	-cd $(SPWD)/$(OM_GDB) && $(INACROSS) $(SPWD)/$(OM_GDB)/configure \
	--host=$(OM_TARG) \
	--target=$(OM_TARG) \
	--disable-gnulib \
	MAKEINFO=false \
	--without-readline

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GDB)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GDB) install  DESTDIR=$(OM_LIBROOT)


gdb_host: install_path

	-rm -rf $(OM_GDB)
	@tar -xf $(OM_SRC)/$(OM_GDB).$(OM_SFX) -C $(SPWD)

	-cd $(SPWD)/$(OM_GDB) && $(SPWD)/$(OM_GDB)/configure \
	--prefix=$(OM_PREF) \
	--target=$(OM_TARG)

	-$(MAKE) -C $(SPWD)/$(OM_GDB)
	-$(MAKE) -C $(SPWD)/$(OM_GDB) install


python_target: install_path

	
	@cd patches && test -e $(OM_PYTHON)-xcompile_improvements.patch || \
			wget $(GENERIC_MRR)/$(OM_PYTHON)-xcompile_improvements.patch
	-rm -rf $(OM_PYTHON)
	-mkdir -p $(OM_PYTHON)
	@tar -xf $(OM_SRC)/$(OM_PYTHON).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_PYTHON) && $(MAKE) distclean
	-cd $(SPWD)/$(OM_PYTHON) && rm -rf python_for_build Parser/pgen_for_build
	-cd $(SPWD)/$(OM_PYTHON) && ./configure
	-cd $(SPWD)/$(OM_PYTHON) && $(MAKE) python Parser/pgen
	-cd $(SPWD)/$(OM_PYTHON) && mv python python_for_build
	-cd $(SPWD)/$(OM_PYTHON) && mv Parser/pgen Parser/pgen_for_build
	-cd $(SPWD)/$(OM_PYTHON) && patch -p1 < $(SPWD)/patches/Python-2.7.9-xcompile_improvements.patch
	-cd $(SPWD)/$(OM_PYTHON) && $(INACROSS) $(SPWD)/$(OM_PYTHON)/configure \
	--build=$(OM_HOST) \
	--host=$(OM_TARG) \
	--target=$(OM_TARG) \
	--disable-ipv6 \
	ac_cv_file__dev_ptmx=no \
	ac_cv_file__dev_ptc=no

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_PYTHON)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_PYTHON) install  DESTDIR=$(OM_LIBROOT)


perl_target: install_path

	# this must be built at -j1
	@cd src && test -e $(OM_PERL).$(OM_SFX) || wget $(OM_PERL_MRR)/$(OM_PERL).$(OM_SFX)
	#@cd src && test -e $(OM_PERL_CROSS).$(OM_SFX) || wget $(OM_PERL_CROSS_MRR)/$(OM_PERL_CROSS).$(OM_SFX)
	-rm -rf $(OM_PERL)
	-mkdir -p $(OM_PERL)
	@tar -xf $(OM_SRC)/$(OM_PERL).$(OM_SFX) -C $(SPWD)
	#@tar -xf $(OM_SRC)/$(OM_PERL_CROSS).$(OM_SFX) -C $(SPWD)

	-cd $(SPWD)/$(OM_PERL) && $(INACROSS) $(SPWD)/$(OM_PERL)/configure \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_PERL)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_PERL) install DESTDIR=$(OM_LIBROOT)


bash_target: install_path

	@cd src && test -e $(OM_BASH).$(OM_SFX) || wget $(GNU_MRR)/bash/$(OM_BASH).$(OM_SFX)
	-rm -rf $(OM_BASH)
	-mkdir -p $(OM_BASH)
	@tar -xf $(OM_SRC)/$(OM_BASH).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_BASH) && $(INACROSS) $(SPWD)/$(OM_BASH)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_BASH)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_BASH) install DESTDIR=$(OM_LIBROOT)


bison_target: install_path

	@cd src && test -e $(OM_BISON).$(OM_SFX) || wget $(GNU_MRR)/bison/$(OM_BISON).$(OM_SFX)
	-rm -rf $(OM_BISON)
	-mkdir -p $(OM_BISON)
	@tar -xf $(OM_SRC)/$(OM_BISON).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_BISON) && $(INACROSS) $(SPWD)/$(OM_BISON)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG) \
	PERL=missing \
	MAKEINFO=false \
	TEXINFO=false

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_BISON)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_BISON) install DESTDIR=$(OM_LIBROOT)


git_target: install_path

	@cd src && test -e $(OM_GIT).$(OM_SFX) || wget $(GNU_MRR)/git/old//$(OM_GIT).$(OM_SFX)
	-rm -rf $(OM_GIT)
	-mkdir -p $(OM_GIT)
	@tar -xf $(OM_SRC)/$(OM_GIT).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_GIT) && $(INACROSS) $(SPWD)/$(OM_GIT)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GIT)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GIT) install DESTDIR=$(OM_LIBROOT)


ocrad_target: install_path

	@cd src && test -e $(OM_OCRAD).$(OM_SFX) || wget $(GENERIC_MRR)/$(OM_OCRAD).$(OM_SFX)
	-rm -rf $(OM_OCRAD)
	-mkdir -p $(OM_OCRAD)
	@tar -xf $(OM_SRC)/$(OM_OCRAD).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_OCRAD) && $(INACROSS) $(SPWD)/$(OM_OCRAD)/configure $(INACROSS)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_OCRAD)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_OCRAD) install DESTDIR=$(OM_LIBROOT)


xorriso_target: install_path

	@cd src && test -e $(OM_XORRISO).$(OM_SFX) || wget $(GNU_MRR)/xorriso/$(OM_XORRISO).$(OM_SFX)
	-rm -rf $(OM_XORRISO)
	-mkdir -p $(OM_XORRISO)
	@tar -xf $(OM_SRC)/$(OM_XORRISO).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_XORRISO) && $(INACROSS) $(SPWD)/$(OM_XORRISO)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_XORRISO)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_XORRISO) install DESTDIR=$(OM_LIBROOT)


nano_target: install_path

	@cd src && test -e $(OM_NANO).$(OM_SFX) || wget $(GNU_MRR)/nano/$(OM_NANO).$(OM_SFX)
	-rm -rf $(OM_NANO)
	-mkdir -p $(OM_NANO)
	@tar -xf $(OM_SRC)/$(OM_NANO).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_NANO) && $(INACROSS) $(SPWD)/$(OM_NANO)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_NANO)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_NANO) install DESTDIR=$(OM_LIBROOT)


groff_target: install_path

	@cd src && test -e $(OM_GROFF).$(OM_SFX) || wget $(GNU_MRR)/groff/$(OM_GROFF).$(OM_SFX)
	# build at -j1
	-rm -rf $(OM_GROFF)
	-mkdir -p $(OM_GROFF)
	@tar -xf $(OM_SRC)/$(OM_GROFF).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_GROFF) && $(INACROSS) $(SPWD)/$(OM_GROFF)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GROFF)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GROFF) install DESTDIR=$(OM_LIBROOT)


bc_target: install_path

	@cd src && test -e $(OM_BC).$(OM_SFX) || wget $(GNU_MRR)/bc/$(OM_BC).$(OM_SFX)
	-rm -rf $(OM_BC)
	-mkdir -p $(OM_BC)
	@tar -xf $(OM_SRC)/$(OM_BC).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_BC) && $(INACROSS) $(SPWD)/$(OM_BC)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_BC)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_BC) install DESTDIR=$(OM_LIBROOT)


cpio_target: install_path

	@cd src && test -e $(OM_CPIO).$(OM_SFX) || wget $(GNU_MRR)/cpio/$(OM_CPIO).$(OM_SFX)
	-rm -rf $(OM_CPIO)
	-mkdir -p $(OM_CPIO)
	@tar -xf $(OM_SRC)/$(OM_CPIO).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_CPIO) && $(INACROSS) $(SPWD)/$(OM_CPIO)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_CPIO)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_CPIO) install DESTDIR=$(OM_LIBROOT)


rsync_target: install_path

	@cd src && test -e $(OM_RSYNC).$(OM_SFX) || wget $(OM_RSYNC_MRR)/$(OM_RSYNC).$(OM_SFX) 
	-rm -rf $(OM_RSYNC)
	-mkdir -p $(OM_RSYNC)
	@tar -xf $(OM_SRC)/$(OM_RSYNC).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_RSYNC) && $(INACROSS) $(SPWD)/$(OM_RSYNC)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_RSYNC)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_RSYNC) install DESTDIR=$(OM_LIBROOT)


libiconv_host: install_path

	@cd src && test -e $(OM_LIBICONV).$(OM_SFX) || wget $(GNU_MRR)/libiconv/$(OM_LIBICONV).$(OM_SFX)
	rm -rf $(OM_LIBICONV)
	@tar -xf $(OM_SRC)/$(OM_LIBICONV).$(OM_SFX) -C $(SPWD)
	cd $(SPWD)/$(OM_LIBICONV) && $(SPWD)/$(OM_LIBICONV)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_HOST) \
	$(INACROSS)

	$(MAKE) -C $(SPWD)/$(OM_LIBICONV)/
	$(MAKE) -C $(SPWD)/$(OM_LIBICONV)/ install 


libunistring_host: install_path

	@cd src && test -e $(OM_LIBUNISTRING).$(OM_SFX) || wget $(GNU_MRR)/libunistring/$(OM_LIBUNISTRING).$(OM_SFX)
	rm -rf $(OM_LIBUNISTRING)
	@tar -xf $(OM_SRC)/$(OM_LIBUNISTRING).$(OM_SFX) -C $(SPWD)
	cd $(SPWD)/$(OM_LIBUNISTRING) && $(SPWD)/$(OM_LIBUNISTRING)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_HOST) \
	$(INACROSS)

	$(MAKE) -C $(SPWD)/$(OM_LIBUNISTRING)/
	$(MAKE) -C $(SPWD)/$(OM_LIBUNISTRING)/ install 


libtool_host: install_path

	@cd src && test -e $(OM_LIBTOOL).$(OM_SFX) || wget $(GNU_MRR)/libtool/$(OM_LIBTOOL).$(OM_SFX)
	rm -rf $(OM_LIBTOOL)
	@tar -xf $(OM_SRC)/$(OM_LIBTOOL).$(OM_SFX) -C $(SPWD)
	cd $(SPWD)/$(OM_LIBTOOL) && $(SPWD)/$(OM_LIBTOOL)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_HOST) \
	$(INACROSS)

	$(MAKE) -C $(SPWD)/$(OM_LIBTOOL)/
	$(MAKE) -C $(SPWD)/$(OM_LIBTOOL)/ install


libgc_host: install_path

	@cd src && test -e $(OM_LIBGC).$(OM_SFX) || wget $(OM_LIBGC_MRR)/$(OM_LIBGC).$(OM_SFX)
	rm -rf $(OM_LIBGC)
	@tar -xf $(OM_SRC)/$(OM_LIBGC).$(OM_SFX) -C $(SPWD)
	@tar -xf $(OM_SRC)/$(OM_LIBATOMIC_OPS).$(OM_SFX) -C $(SPWD)/$(OM_LIBGC)
	cd $(SPWD)/$(OM_LIBGC) && mv $(OM_LIBATOMIC_OPS) libatomic_ops
	cd $(SPWD)/$(OM_LIBGC) && sed -i 's|uclibc|musl|g' configure
	cd $(SPWD)/$(OM_LIBGC) && $(SPWD)/$(OM_LIBGC)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_HOST) \
	--target=$(OM_SUBTARG) \
	--disable-threads \
	$(INACROSS)

	$(OM_GUILE_ENV) $(MAKE) -C $(SPWD)/$(OM_LIBGC)/
	$(OM_GUILE_ENV) $(MAKE) -C $(SPWD)/$(OM_LIBGC)/ install


libffi_host: install_path

	@cd src && test -e $(OM_LIBFFI).$(OM_SFX) || wget $(OM_LIBFFI_MRR)/$(OM_LIBFFI).$(OM_SFX)
	rm -rf $(OM_LIBFFI)
	@tar -xf $(OM_SRC)/$(OM_LIBFFI).$(OM_SFX) -C $(SPWD)
	cd $(SPWD)/$(OM_LIBFFI) && $(SPWD)/$(OM_LIBFFI)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_HOST) \
	$(INACROSS)

	$(MAKE) -C $(SPWD)/$(OM_LIBFFI)/
	$(MAKE) -C $(SPWD)/$(OM_LIBFFI)/ install
	cp $(OM_PREF)/$(OM_TARG)/lib/libffi-3.2/include/* $(OM_PREF)/$(OM_TARG)/include/


openssl_target: install_path

	@cd src && test -e $(OM_OPENSSL).$(OM_SFX) || wget $(OM_OPENSSL_MRR)/$(OM_OPENSSL).$(OM_SFX)
	-rm -rf $(OM_OPENSSL)
	-mkdir -p $(OM_OPENSSL)
	@tar -xf $(OM_SRC)/$(OM_OPENSSL).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_OPENSSL) && $(OM_OPENSSL_CROSS_ENV) $(SPWD)/$(OM_OPENSSL)/Configure \
	linux-elf --prefix=$(OM_LIBROOT) 
	
	# Satisfy makedepend, this is a hack
	-mkdir -p $(SPWD)/$(OM_OPENSSL)/include/openssl/
	-cp -r $(OM_PREF)/$(OM_TARG)/* $(SPWD)/$(OM_OPENSSL)/
	-cd $(SPWD)/$(OM_OPENSSL) && $(OM_OPENSSL_CROSS_ENV) make depend

	-$(OM_OPENSSL_CROSS_ENV) $(MAKE) -C $(SPWD)/$(OM_OPENSSL)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_OPENSSL) install DESTDIR=$(OM_LIBROOT)


gawk_target: install_path

	@cd src && test -e $(OM_GAWK).$(OM_SFX) || wget $(GNU_MRR)/gawk/$(OM_GAWK).$(OM_SFX)
	-rm -rf $(OM_GAWK)
	-mkdir -p $(OM_GAWK)
	@tar -xf $(OM_SRC)/$(OM_GAWK).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_GAWK) && $(INACROSS) $(SPWD)/$(OM_GAWK)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GAWK)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GAWK) install DESTDIR=$(OM_LIBROOT)


gettext_target: install_path

	@cd src && test -e $(OM_GETTEXT).$(OM_SFX) || wget $(GNU_MRR)/gettext/$(OM_GETTEXT).$(OM_SFX)
	-rm -rf $(OM_GETTEXT)
	-mkdir -p $(OM_GETTEXT)
	@tar -xf $(OM_SRC)/$(OM_GETTEXT).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_GETTEXT) && $(INACROSS) $(SPWD)/$(OM_GETTEXT)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GETTEXT)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GETTEXT) install DESTDIR=$(OM_LIBROOT)


octave_target: install_path

	@cd src && test -e $(OM_OCTAVE).$(OM_SFX) || wget $(GNU_MRR)/octave/$(OM_OCTAVE).$(OM_SFX)
	# requires fortran support
	-rm -rf $(OM_OCTAVE)
	-mkdir -p $(OM_OCTAVE)
	@tar -xf $(OM_SRC)/$(OM_OCTAVE).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_OCTAVE) && $(INACROSS) $(SPWD)/$(OM_OCTAVE)/configure \
	--host=$(OM_SUBTARG) \
	--disable-readline
	#$(INACROSS) FFLAGS=-ff2c --without-blas
	# --target=$(OM_SUBTARG) 
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_OCTAVE)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_OCTAVE) install DESTDIR=$(OM_LIBROOT)


pcre_host: install_path

	@cd src && test -e $(OM_PCRE).$(OM_SFX) || wget $(OM_PCRE_MRR)/$(OM_PCRE).$(OM_SFX)
	rm -rf $(OM_PCRE)
	@tar -xf $(OM_SRC)/$(OM_PCRE).$(OM_SFX) -C $(SPWD)
	cd $(SPWD)/$(OM_PCRE) && $(SPWD)/$(OM_PCRE)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_HOST) \
	$(INACROSS) \
	--disable-shared

	$(MAKE) -C $(SPWD)/$(OM_PCRE)
	$(MAKE) -C $(SPWD)/$(OM_PCRE)/ install 


openssh_target: install_path

	@cd src && test -e $(OM_OPENSSH).$(OM_SFX) || wget $(OM_OPENSSH_MRR)/$(OM_OPENSSH).$(OM_SFX)
	-rm -rf $(OM_OPENSSH)
	-mkdir -p $(OM_OPENSSH)
	@tar -xf $(OM_SRC)/$(OM_OPENSSH).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_OPENSSH) && $(INACROSS) $(SPWD)/$(OM_OPENSSH)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG) \
	--without-hardening \
	--disable-libutil \
	--disable-largefile \
	--disable-strip \
	--disable-etc-default-login \
	--disable-lastlog \
	--disable-utmp \
	--disable-utmpx \
	--disable-wtmp \
	--disable-wtmpx \
	--disable-libutil \
	--disable-pututline \
	--disable-pututxline \
	--without-stackprotect \
	--without-rpath

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_OPENSSH)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_OPENSSH) install DESTDIR=$(OM_LIBROOT) 


libressl_host: install_path

	@cd src && test -e $(OM_LIBRESSL).$(OM_SFX) || wget $(OM_LIBRESSL_MRR)/$(OM_LIBRESSL).$(OM_SFX)
	# just need headers, but cross compile anyway, this is a hack
	-rm -rf $(OM_LIBRESSL)
	-mkdir -p $(OM_LIBRESSL)
	@tar -xf $(OM_SRC)/$(OM_LIBRESSL).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_LIBRESSL) && $(INACROSS) $(SPWD)/$(OM_LIBRESSL)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_TARG) \
	--target=$(OM_TARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_LIBRESSL)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_LIBRESSL) install


patch_target: install_path

	@cd src && test -e $(OM_PATCH).$(OM_SFX) || wget $(GNU_MRR)/patch/$(OM_PATCH).$(OM_SFX)
	-rm -rf $(OM_PATCH)
	-mkdir -p $(OM_PATCH)
	@tar -xf $(OM_SRC)/$(OM_PATCH).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_PATCH) && $(INACROSS) $(SPWD)/$(OM_PATCH)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_PATCH)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_PATCH) install DESTDIR=$(OM_LIBROOT)


weston_target: install_path

	-rm -rf $(OM_WESTON)
	-mkdir -p $(OM_WESTON)
	@tar -xf $(OM_SRC)/$(OM_WESTON).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_WESTON) && $(INACROSS) $(SPWD)/$(OM_WESTON)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_WESTON)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_WESTON) install DESTDIR=$(OM_LIBROOT)


wayland_host: install_path

	rm -rf $(OM_WAYLAND)
	@tar -xf $(OM_SRC)/$(OM_WAYLAND).$(OM_SFX) -C $(SPWD)
	cd $(SPWD)/$(OM_WAYLAND) && $(SPWD)/$(OM_WAYLAND)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_HOST) \
	$(INACROSS) \
	--disable-documentation

	$(MAKE) -C $(SPWD)/$(OM_WAYLAND)/
	$(MAKE) -C $(SPWD)/$(OM_WAYLAND)/ install


expat_host: install_path

	@cd src && test -e $(OM_EXPAT).$(OM_SFX) || wget $(OM_EXPAT_MRR)/$(OM_EXPAT).$(OM_SFX)
	rm -rf $(OM_EXPAT)
	@tar -xf $(OM_SRC)/$(OM_EXPAT).$(OM_SFX) -C $(SPWD)
	cd $(SPWD)/$(OM_EXPAT) && $(SPWD)/$(OM_EXPAT)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_HOST) \
	$(INACROSS)

	$(MAKE) -C $(SPWD)/$(OM_EXPAT)/
	$(MAKE) -C $(SPWD)/$(OM_EXPAT)/ install


libxml2_target: install_path

	@cd src && test -e $(OM_LIBXML2).$(OM_SFX) || wget $(OM_LIBXML2_MRR)/$(OM_LIBXML2).$(OM_SFX)
	-rm -rf $(OM_LIBXML2)
	-mkdir -p $(OM_LIBXML2)
	@tar -xf $(OM_SRC)/$(OM_LIBXML2).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_LIBXML2) && $(INACROSS) $(SPWD)/$(OM_LIBXML2)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_LIBXML2)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_LIBXML2) install DESTDIR=$(OM_LIBROOT)


libxml2_host: install_path

	rm -rf $(OM_LIBXML2)
	@tar -xf $(OM_SRC)/$(OM_LIBXML2).$(OM_SFX) -C $(SPWD)
	cd $(SPWD)/$(OM_LIBXML2) && $(SPWD)/$(OM_LIBXML2)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_HOST) \
	$(INACROSS)

	$(MAKE) -C $(SPWD)/$(OM_LIBXML2)/
	$(MAKE) -C $(SPWD)/$(OM_LIBXML2)/ install


irssi_target: install_path

	@cd src && test -e $(OM_IRSSI).$(OM_SFX) || wget $(OM_IRSSI_MRR)/$(OM_IRSSI).$(OM_SFX)
	-$(MAKE) glib_host
	-rm -rf $(OM_IRSSI)
	-mkdir -p $(OM_IRSSI)
	@tar -xf $(OM_SRC)/$(OM_IRSSI).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_IRSSI) && $(INACROSS) $(SPWD)/$(OM_IRSSI)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_IRSSI)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_IRSSI) install DESTDIR=$(OM_LIBROOT)


glib_host: install_path

	@cd src && test -e $(OM_GLIB).$(OM_SFX) || wget $(GENERIC_MRR)/$(OM_GLIB).$(OM_SFX)
	-$(MAKE) libffi_host
	rm -rf $(OM_GLIB)
	@tar -xf $(OM_SRC)/$(OM_GLIB).$(OM_SFX) -C $(SPWD)
	-@cd $(SPWD)/$(OM_GLIB) && rm $(OM_SUBTARG).cache
	#-@cd $(SPWD)/$(OM_GLIB) && printf "%s\n" "glib_cv_long_long_format=I64" >> $(OM_SUBTARG).cache
	-@cd $(SPWD)/$(OM_GLIB) && printf "%s\n" "glib_cv_stack_grows=no" >> $(OM_SUBTARG).cache
	-@cd $(SPWD)/$(OM_GLIB) && printf "%s\n" "glib_cv_working_bcopy=no" >> $(OM_SUBTARG).cache
	-@cd $(SPWD)/$(OM_GLIB) && printf "%s\n" "glib_cv_sane_realloc=no" >> $(OM_SUBTARG).cache
	-@cd $(SPWD)/$(OM_GLIB) && printf "%s\n" "glib_cv_have_strlcpy=no" >> $(OM_SUBTARG).cache
	-@cd $(SPWD)/$(OM_GLIB) && printf "%s\n" "glib_cv_have_qsort_r=no" >> $(OM_SUBTARG).cache
	-@cd $(SPWD)/$(OM_GLIB) && printf "%s\n" "glib_cv_va_val_copy=no" >> $(OM_SUBTARG).cache
	-@cd $(SPWD)/$(OM_GLIB) && printf "%s\n" "glib_cv_rtldglobal_broken=no" >> $(OM_SUBTARG).cache
	-@cd $(SPWD)/$(OM_GLIB) && printf "%s\n" "glib_cv_uscore=yes" >> $(OM_SUBTARG).cache
	-@cd $(SPWD)/$(OM_GLIB) && printf "%s\n" "ac_cv_func_posix_getpwuid_r=no" >> $(OM_SUBTARG).cache
	-@cd $(SPWD)/$(OM_GLIB) && printf "%s\n" "ac_cv_func_nonposix_getpwuid_r=no" >> $(OM_SUBTARG).cache
	-@cd $(SPWD)/$(OM_GLIB) && printf "%s\n" "ac_cv_func_posix_getgrgid_r=no" >> $(OM_SUBTARG).cache
	-@cd $(SPWD)/$(OM_GLIB) && printf "%s\n" "glib_cv_use_pid_surrogate=no" >> $(OM_SUBTARG).cache
	-@cd $(SPWD)/$(OM_GLIB) && printf "%s\n" "ac_cv_func_printf_unix98=yes" >> $(OM_SUBTARG).cache
	-@cd $(SPWD)/$(OM_GLIB) && printf "%s\n" "ac_cv_func_vsnprintf_c99=yes" >> $(OM_SUBTARG).cache
	-@cd $(SPWD)/$(OM_GLIB) && $(INACROSS) $(SPWD)/$(OM_GLIB)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--target=$(OM_SUBTARG) \
	--host=$(OM_HOST) \
	--cache-file=$(OM_SUBTARG).cache

	$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GLIB)/
	$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GLIB)/ install


sudo_target: install_path

	@cd src && test -e $(OM_SUDO).$(OM_SFX) || wget $(OM_SUDO_MRR)/$(OM_SUDO).$(OM_SFX)
	-rm -rf $(OM_SUDO)
	-mkdir -p $(OM_SUDO)
	@tar -xf $(OM_SRC)/$(OM_SUDO).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_SUDO) && $(INACROSS) $(SPWD)/$(OM_SUDO)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG) \
	--disable-hardening \
	--disable-rpath \
	--disable-pie \
	--disable-nls \
	--disable-pam-session \
	--disable-static
	#--disable-shared-libutil \
	#--enable-static-sudoers \
	

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_SUDO)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_SUDO) install DESTDIR=$(OM_LIBROOT)


wirelesstools_target: install_path

	@cd patches && test -e wireless_tools-xcompile.patch || wget $(GENERIC_MRR)/wireless_tools-xcompile.patch
	@cd src && test -e $(OM_WIRELESSTOOLS).$(OM_SFX) || wget $(OM_WIRELESSTOOLS_MRR)/$(OM_WIRELESSTOOLS).$(OM_SFX)
	-rm -rf $(OM_WIRELESSTOOLS)
	-mkdir -p $(OM_WIRELESSTOOLS)
	@tar -xf $(OM_SRC)/$(OM_WIRELESSTOOLS).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_WIRELESSTOOLS) && patch -p0 < $(SPWD)/patches/wireless_tools-xcompile.patch
	# or ...
	#-cd $(SPWD)/$(OM_WIRELESSTOOLS) && sed -i "s|CC =|CC ?=|g" Makefile
	#-cd $(SPWD)/$(OM_WIRELESSTOOLS) && sed -i "s|AR =|AR ?=|g" Makefile
	#-cd $(SPWD)/$(OM_WIRELESSTOOLS) && sed -i "s|RANLIB =|RANLIB ?=|g" Makefile

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_WIRELESSTOOLS)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_WIRELESSTOOLS) install PREFIX=$(OM_LIBROOT)


libpam_host: install_path

	rm -rf $(OM_LIBPAM)
	@tar -xf $(OM_SRC)/$(OM_LIBPAM).$(OM_SFX) -C $(SPWD)
	@cd $(SPWD)/$(OM_LIBPAM) && patch -p1 < $(SPWD)/patches/libpam-musl-compat.patch
	cd $(SPWD)/$(OM_LIBPAM) && $(INACROSS) $(SPWD)/$(OM_LIBPAM)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_HOST) \
	$(INACROSS)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_LIBPAM)/
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_LIBPAM)/ install


libpam_target: install_path

	-rm -rf $(OM_LIBPAM)
	-mkdir -p $(OM_LIBPAM)
	@tar -xf $(OM_SRC)/$(OM_LIBPAM).$(OM_SFX) -C $(SPWD) 
	@cd $(SPWD)/$(OM_LIBPAM) && patch -p1 < $(SPWD)/patches/libpam-musl-compat.patch
	-cd $(SPWD)/$(OM_LIBPAM) && $(INACROSS) $(SPWD)/$(OM_LIBPAM)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG) \
	$(INACROSS)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_LIBPAM)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_LIBPAM) install DESTDIR=$(OM_LIBROOT)


weechat_target: install_path

	@cd src && test -e $(OM_WEECHAT).$(OM_SFX) || wget $(OM_WEECHAT_MRR)/$(OM_WEECHAT).$(OM_SFX)
	# this depends on gnucrypt / gcrypt which depends on java
	# untested
	-rm -rf $(OM_WEECHAT)
	-mkdir -p $(OM_WEECHAT)
	@tar -xf $(OM_SRC)/$(OM_WEECHAT).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_WEECHAT) && ./autogen.sh
	-cd $(SPWD)/$(OM_WEECHAT) && $(INACROSS) $(SPWD)/$(OM_WEECHAT)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_WEECHAT)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_WEECHAT) install DESTDIR=$(OM_LIBROOT)


gnucrypt_host: install_path

	@cd src && test -e $(OM_GNUCRYPT).$(OM_SFX) || wget $(OM_GNUCRYPT_MRR)/$(OM_GNUCRYPT).$(OM_SFX)
	# this depends on a java compiler, untested
	rm -rf $(OM_GNUCRYPT)
	@tar -xf $(OM_SRC)/$(OM_GNUCRYPT).$(OM_SFX) -C $(SPWD)
	-@cd $(SPWD)/$(OM_GNUCRYPT) && $(INACROSS) $(SPWD)/$(OM_GNUCRYPT)/configure \
	--prefix=$(OM_PREF)/$(OM_TARG) \
	--host=$(OM_HOST) \
	$(INACROSS)

	-@$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GNUCRYPT)/
	-@$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GNUCRYPT)/ install


ircii_target: install_path

	@cd patches && test -e ircii-20151120-xcompile.patch || wget $(GENERIC_MRR)/ircii-20151120-xcompile.patch
	@cd src && test -e $(OM_IRCII).$(OM_SFX) || wget $(OM_IRCII_MRR)/$(OM_IRCII).$(OM_SFX)
	-rm -rf $(OM_IRCII)
	-mkdir -p $(OM_IRCII)
	@tar -xf $(OM_SRC)/$(OM_IRCII).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_IRCII) && patch -p0 < $(SPWD)/patches/ircii-20151120-xcompile.patch
	-cd $(SPWD)/$(OM_IRCII) && $(INACROSS) $(SPWD)/$(OM_IRCII)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG) \
	--without-openssl
	#ax_prog_cc_for_build=gcc
	
	
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_IRCII)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_IRCII) install DESTDIR=$(OM_LIBROOT) 


mutt_target: install_path

	@cd src && test -e $(OM_MUTT).$(OM_SFX) || wget $(OM_MUTT_MRR)/$(OM_MUTT).$(OM_SFX)
	-rm -rf $(OM_MUTT)
	-mkdir -p $(OM_MUTT)
	@tar -xf $(OM_SRC)/$(OM_MUTT).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_MUTT) && $(INACROSS) $(SPWD)/$(OM_MUTT)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_MUTT)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_MUTT) install DESTDIR=$(OM_LIBROOT)


inetutils_target: install_path

	@cd patches && test -e inetutils-PATH_PROCNET_DEV.patch || wget $(GENERIC_MRR)/inetutils-PATH_PROCNET_DEV.patch
	@cd src && test -e $(OM_INETUTILS).$(OM_SFX) || wget $(OM_INETUTILS_MRR)/$(OM_INETUTILS).$(OM_SFX)
	-rm -rf $(OM_INETUTILS)
	-mkdir -p $(OM_INETUTILS)
	@tar -xf $(OM_SRC)/$(OM_INETUTILS).$(OM_SFX) -C $(SPWD)
	-@cd $(SPWD)/$(OM_INETUTILS) && patch -p0 < $(SPWD)/patches/inetutils-PATH_PROCNET_DEV.patch
	-cd $(SPWD)/$(OM_INETUTILS) && $(INACROSS) $(SPWD)/$(OM_INETUTILS)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG) \
	--disable-rlogin \
	--disable-toremote \
	--disable-tolocal \
	--disable-maintoremote \
	--disable-rsh \
	--disable-syslogd \
	--disable-inetd \
	--disable-rcp

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_INETUTILS)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_INETUTILS) install DESTDIR=$(OM_LIBROOT)


createtarg:

	printf "%s\n" '$(pack)_target: install_path' >> Makefile
	printf "%s\n" '' >> Makefile
	printf "%s\n" '\t-rm -rf $$(OM_$(OM_PACK))' >> Makefile
	printf "%s\n" '\tcd $$(OM_SRC) && test -e $$(OM_$(OM_PACK)).$$(OM_SFX) || wget $$(OM_$(OM_PACK)_MRR)/$$(OM_$(OM_PACK)).$$(OM_SFX)' >> Makefile
	printf "%s\n" '\t@tar -xf $$(OM_SRC)/$$(OM_$(OM_PACK)).$$(OM_SFX) -C $$(SPWD)' >> Makefile 
	printf "%s\n" '\t-cd $$(SPWD)/$$(OM_$(OM_PACK)) && $$(INACROSS) $$(SPWD)/$$(OM_$(OM_PACK))/configure \\' >> Makefile
	printf "%s\n" '\t--host=$$(OM_SUBTARG) \\' >> Makefile
	printf "%s\n" '\t--target=$$(OM_SUBTARG)' >> Makefile
	printf "%s\n" '' >> Makefile
	printf "%s\n" '\t-$$(INACROSS) $$(MAKE) -C $$(SPWD)/$$(OM_$(OM_PACK))' >> Makefile
	printf "%s\n" '\t-$$(INACROSS) $$(MAKE) -C $$(SPWD)/$$(OM_$(OM_PACK)) install DESTDIR=$$(OM_LIBROOT)' >> Makefile
	printf "%s\n" '' >> Makefile


createhost:

	printf "%s\n" '$(pack)_host: install_path' >> Makefile
	printf "%s\n" '' >> Makefile
	printf "%s\n" '\t-rm -rf $$(OM_$(OM_PACK))' >> Makefile
	printf "%s\n" '\tcd $$(OM_SRC) && test -e $$(OM_$(OM_PACK)).$$(OM_SFX) || wget $$(OM_$(OM_PACK)_MRR)/$$(OM_$(OM_PACK)).$$(OM_SFX)' >> Makefile
	printf "%s\n" '\t@tar -xf $$(OM_SRC)/$$(OM_$(OM_PACK)).$$(OM_SFX) -C $$(SPWD)' >> Makefile
	
	printf "%s\n" '\t-cd $$(SPWD)/$$(OM_$(OM_PACK)) && $$(INACROSS) $$(SPWD)/$$(OM_$(OM_PACK))/configure \\' >> Makefile
	printf "%s\n" '\t--prefix=$$(OM_PREF)/$$(OM_TARG) \\' >> Makefile
	printf "%s\n" '\t--host=$$(OM_HOST) \\' >> Makefile
	printf "%s\n" '\t$$(INACROSS)' >> Makefile
	printf "%s\n" '' >> Makefile
	printf "%s\n" '\t-$$(INACROSS) $$(MAKE) -C $$(SPWD)/$$(OM_$(OM_PACK))' >> Makefile
	printf "%s\n" '\t-$$(INACROSS) $$(MAKE) -C $$(SPWD)/$$(OM_$(OM_PACK)) install' >> Makefile
	printf "%s\n" '' >> Makefile


zsh_target: install_path

	@cd src && test -e $(OM_ZSH).$(OM_SFX) || wget $(OM_ZSH_MRR)/$(OM_ZSH).$(OM_SFX)
	-rm -rf $(OM_ZSH)
	@tar -xf $(OM_SRC)/$(OM_ZSH).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_ZSH) && $(INACROSS) $(SPWD)/$(OM_ZSH)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_ZSH)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_ZSH) install DESTDIR=$(OM_LIBROOT)


xz_target: install_path

	@cd src && test -e $(OM_XZ).$(OM_SFX) || wget $(OM_XZ_MRR)/$(OM_XZ).$(OM_SFX)
	-rm -rf $(OM_XZ)
	@tar -xf $(OM_SRC)/$(OM_XZ).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_XZ) && $(INACROSS) $(SPWD)/$(OM_XZ)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_XZ)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_XZ) install DESTDIR=$(OM_LIBROOT)

gptfdisk_target: install_path

	@cd src && test -e $(OM_GPTFDISK).$(OM_SFX) || wget $(OM_GPTFDISK_MRR)/$(OM_GPTFDISK).$(OM_SFX)
	#-$(MAKE) popt_host
	#-$(MAKE) e2fsprogs_host
	-rm -rf $(OM_GPTFDISK)
	@tar -xf $(OM_SRC)/$(OM_GPTFDISK).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_GPTFDISK) && patch -p0 < $(SPWD)/patches/gptfdisk-xcompile.patch
	-mkdir -p $(OM_LIBROOT)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GPTFDISK)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_GPTFDISK) install DESTDIR=$(OM_LIBROOT)


popt_host: install_path

	@cd src && test -e $(OM_POPT).$(OM_SFX) || wget $(OM_POPT_MRR)/$(OM_POPT).$(OM_SFX)
	-rm -rf $(OM_POPT)
	@tar -xf $(OM_SRC)/$(OM_POPT).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_POPT) && $(INACROSS) $(SPWD)/$(OM_POPT)/configure \
	--prefix=/ \
	--host=$(OM_HOST) \
	$(INACROSS)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_POPT)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_POPT) install DESTDIR=$(OM_PREF)/$(OM_TARG)

pdksh_target: install_path

	@cd src && test -e $(OM_PDKSH).$(OM_SFX) || wget $(OM_PDKSH_MRR)/$(OM_PDKSH).$(OM_SFX)
	-rm -rf $(OM_PDKSH)
	@tar -xf $(OM_SRC)/$(OM_PDKSH).$(OM_SFX) -C $(SPWD) 
	-cd $(SPWD)/$(OM_PDKSH) && $(INACROSS) $(SPWD)/$(OM_PDKSH)/configure \
	--prefix=$(OM_LIBROOT) \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_PDKSH)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_PDKSH) install

netbsdcurses_host: install_path

	-rm -rf $(OM_NETBSDCURSES)
	@tar -xf $(OM_SRC)/$(OM_NETBSDCURSES).$(OM_SFX) -C $(SPWD) 

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_NETBSDCURSES)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_NETBSDCURSES) install DESTDIR=$(OM_PREF)/$(OM_TARG)

tmux_target: install_path

	-rm -rf $(OM_TMUX)
	cd $(OM_SRC) && test -e $(OM_TMUX).$(OM_SFX) || wget $(OM_TMUX_MRR)/$(OM_TMUX).$(OM_SFX)
	@tar -xf $(OM_SRC)/$(OM_TMUX).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_TMUX) && $(INACROSS) $(SPWD)/$(OM_TMUX)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_TMUX)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_TMUX) install DESTDIR=$(OM_LIBROOT)

valgrind_target: install_path

	-rm -rf $(OM_VALGRIND)
	cd $(OM_SRC) && test -e $(OM_VALGRIND).$(OM_SFX) || wget $(OM_VALGRIND_MRR)/$(OM_VALGRIND).$(OM_SFX)
	@tar -xf $(OM_SRC)/$(OM_VALGRIND).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_VALGRIND) && $(INACROSS) $(SPWD)/$(OM_VALGRIND)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_VALGRIND)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_VALGRIND) install DESTDIR=$(OM_LIBROOT)

	
make_target: install_path

	-rm -rf $(OM_MAKE)
	cd $(OM_SRC) && test -e $(OM_MAKE).$(OM_SFX) || wget $(OM_MAKE_MRR)/$(OM_MAKE).$(OM_SFX)
	@tar -xf $(OM_SRC)/$(OM_MAKE).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_MAKE) && $(INACROSS) $(SPWD)/$(OM_MAKE)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_MAKE)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_MAKE) install DESTDIR=$(OM_LIBROOT)


genext2fs_host: install_path

	cd $(OM_SRC) && test -e $(OM_GENEXTFS).$(OM_SFX) || wget $(OM_GENEXTFS_MRR)/$(OM_GENEXTFS).$(OM_SFX)
	@tar -xf $(OM_SRC)/$(OM_GENEXTFS).$(OM_SFX) -C $(SPWD)
	-rm -rf $(OM_GENEXTFS)
	cd $(OM_SRC) && test -e $(OM_GENEXTFS).$(OM_SFX) || wget $(OM_GENEXTFS_MRR)/$(OM_GENEXTFS).$(OM_SFX)
	@tar -xf $(OM_SRC)/$(OM_GENEXTFS).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_GENEXTFS) && $(SPWD)/$(OM_GENEXTFS)/configure \
	--prefix=$(OM_PREF)/ \
	--host=$(OM_HOST)

	-$(MAKE) -C $(SPWD)/$(OM_GENEXTFS)
	$(MAKE) -C $(SPWD)/$(OM_GENEXTFS) install


generate_host_tools:

	-$(MAKE) busybox_host
	-$(MAKE) coreutils_host
	-$(MAKE) patch_host
	-$(MAKE) gawk_host
	-$(MAKE) make_host

busybox_host:

	@cd src && test -e omnicross-busybox-minimal.1 || wget $(GENERIC_MRR)/omnicross-busybox-minimal.1
	@cd src && test -e $(OM_BUSYBOX).$(OM_SFX) || wget $(GENERIC_MRR)/$(OM_BUSYBOX).$(OM_SFX)
	-rm -rf $(OM_BUSYBOX)
	cd $(OM_SRC) && test -e $(OM_BUSYBOX).$(OM_SFX) || wget $(OM_BUSYBOX_MRR)/$(OM_BUSYBOX).$(OM_SFX)
	@tar -xf $(OM_SRC)/$(OM_BUSYBOX).$(OM_SFX) -C $(SPWD)

	cp $(OM_SRC)/omnicross-busybox-minimal.1 $(OM_BUSYBOX)/.config
	-$(MAKE) -C $(SPWD)/$(OM_BUSYBOX) CONFIG_PREFIX=$(OM_SELFBLD)
	-$(MAKE) -C $(SPWD)/$(OM_BUSYBOX) CONFIG_PREFIX=$(OM_SELFBLD) install


coreutils_host:

	@cd src && test -e $(OM_COREUTILS).$(OM_SFX) || wget $(OM_COREUTILS_MRR)/$(OM_COREUTILS).$(OM_SFX)
	-rm -rf $(OM_COREUTILS)
	cd $(OM_SRC) && test -e $(OM_COREUTILS).$(OM_SFX) || wget $(OM_COREUTILS_MRR)/$(OM_COREUTILS).$(OM_SFX)
	@tar -xf $(OM_SRC)/$(OM_COREUTILS).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_COREUTILS) && $(SPWD)/$(OM_COREUTILS)/configure \
	--prefix=$(OM_SELFBLD) \
	--host=$(OM_HOST)

	-$(MAKE) -C $(SPWD)/$(OM_COREUTILS)
	-$(MAKE) -C $(SPWD)/$(OM_COREUTILS) install

patch_host:

	-rm -rf $(OM_PATCH)
	cd $(OM_SRC) && test -e $(OM_PATCH).$(OM_SFX) || wget $(OM_PATCH_MRR)/$(OM_PATCH).$(OM_SFX)
	@tar -xf $(OM_SRC)/$(OM_PATCH).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_PATCH) && $(SPWD)/$(OM_PATCH)/configure \
	--prefix=$(OM_SELFBLD) \
	--host=$(OM_HOST)

	-$(MAKE) -C $(SPWD)/$(OM_PATCH)
	-$(MAKE) -C $(SPWD)/$(OM_PATCH) install

gawk_host:

	-rm -rf $(OM_GAWK)
	cd $(OM_SRC) && test -e $(OM_GAWK).$(OM_SFX) || wget $(OM_GAWK_MRR)/$(OM_GAWK).$(OM_SFX)
	@tar -xf $(OM_SRC)/$(OM_GAWK).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_GAWK) && $(SPWD)/$(OM_GAWK)/configure \
	--prefix=$(OM_SELFBLD) \
	--host=$(OM_HOST)

	-$(MAKE) -C $(SPWD)/$(OM_GAWK)
	-$(MAKE) -C $(SPWD)/$(OM_GAWK) install

make_host:

	-rm -rf $(OM_MAKE)
	cd $(OM_SRC) && test -e $(OM_MAKE).$(OM_SFX) || wget $(OM_MAKE_MRR)/$(OM_MAKE).$(OM_SFX)
	@tar -xf $(OM_SRC)/$(OM_MAKE).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_MAKE) && $(SPWD)/$(OM_MAKE)/configure \
	--prefix=$(OM_SELFBLD) \
	--host=$(OM_HOST)
	
	-$(MAKE) -C $(SPWD)/$(OM_MAKE)
	-$(MAKE) -C $(SPWD)/$(OM_MAKE) install

host_path:

	$(eval PATH=$(OM_SELFBLD)/bin:$(OM_SELFBLD)/usr/bin:$(OM_SELFBLD)/usr/local/bin:$(PATH))


m4_target: install_path

	-rm -rf $(OM_M4)
	cd $(OM_SRC) && test -e $(OM_M4).$(OM_SFX) || wget $(OM_M4_MRR)/$(OM_M4).$(OM_SFX)
	@tar -xf $(OM_SRC)/$(OM_M4).$(OM_SFX) -C $(SPWD)
	-cd $(SPWD)/$(OM_M4) && $(INACROSS) $(SPWD)/$(OM_M4)/configure \
	--host=$(OM_SUBTARG) \
	--target=$(OM_SUBTARG)

	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_M4)
	-$(INACROSS) $(MAKE) -C $(SPWD)/$(OM_M4) install DESTDIR=$(OM_LIBROOT)

simple_dist:

	$(MAKE) install DESTDIR=$(PWD)/libroot
	$(MAKE) linux_target
	$(MAKE) busybox_target
	$(MAKE) genext2fs_host
	-cp -a ../tree/* libroot
	$(OM_PREF)/bin/genext2fs -b 1000000 -d $(OM_LIBROOT) binary.img
	qemu-system-$(OM_BACKWARDCOMPAT) -m 1000 -curses \
	-kernel $(OM_LIBROOT)/boot/vmlinuz \
	-append "root=/dev/sda" \
	-hda binary.img

simple_dist_test:

	$(OM_PREF)/bin/genext2fs -b 1000000 -d $(OM_LIBROOT) binary.img
	qemu-system-$(OM_BACKWARDCOMPAT) -m 1000 -curses \
	-kernel $(OM_LIBROOT)/boot/vmlinuz \
	-append "root=/dev/sda" \
	-hda binary.img


simple_dist_initramfs:

	$(MAKE) install DESTDIR=$(PWD)/libroot
	$(MAKE) busybox_target
	-cp -a ../tree/* libroot
	$(MAKE) linux_target
	qemu-system-$(OM_BACKWARDCOMPAT) -m 1000 -curses \
	-kernel $(OM_LIBROOT)/boot/vmlinuz \
	-append "root=/dev/sda rdinit=/bin/init"
	
postinstall:

	mkdir -p /newtoolchain/
	mount /dev/sdc /newtoolchain/
	mkdir -p /newtoolchain/src/
	mount /dev/sdb /newtoolchain/src
	cp -r /toolchain/* /newtoolchain
	chown -R liveuser /newtoolchain
	chmod 777 /tmp
	su - liveuser
	cd /newtoolchain/
	$(eval PATH=/usr/local/bin:/usr/bin:/bin/:sbin)
	$(eval CC=gcc)
	#make > logfile 2>&1 &

trivtest:

	# Some trivial compile time tests
	-mkdir -p $(LOGDIR)
	-/$(OM_PREF)/bin/$(OM_TARG)-gcc tests/pthreads-test.c -lpthread && \
	/$(OM_PREF)/bin/$(OM_TARG)-gcc -v 2>&1 | grep posix && \
	/$(OM_PREF)/bin/$(OM_TARG)-gcc tests/testing.c && \
		printf "%s\t--> %s  (pthreads, c" "$(LOGHOST)" "$(OM_TARG)" >> $(LOGDIR)/$(LOGHOST)-report || \
		printf "%s\t--> %s  (failed" "$(LOGHOST)" "$(OM_TARG)" >> $(LOGDIR)/$(LOGHOST)-report
	-/$(OM_PREF)/bin/$(OM_TARG)-g++ tests/testing.cpp && \
		printf ", c++" >> $(LOGDIR)/$(LOGHOST)-report 
	-/$(OM_PREF)/bin/$(OM_TARG)-gfortran tests/testing.f && \
		printf ", fortran" >> $(LOGDIR)/$(LOGHOST)-report
	-printf ")\n" >> $(LOGDIR)/$(LOGHOST)-report
	-printf "Extra OM_CONF=$(OM_CONF)\n" >> $(LOGDIR)/$(LOGHOST)-report
	-printf "\tOM_LINUXARCH: $(OM_LINUXARCH)\n" >> $(LOGDIR)/$(LOGHOST)-report
	-printf "\tOM_BACKWARDCOMPAT: $(OM_BACKWARDCOMPAT)\n" >> $(LOGDIR)/$(LOGHOST)-report
	-/$(OM_PREF)/bin/$(OM_TARG)-gcc -v >> $(LOGDIR)/$(LOGHOST)-report 2>&1
	-printf "\n" >> $(LOGDIR)/$(LOGHOST)-report

startreport:

	-mkdir -p $(LOGDIR)/
	-printf '\tOperating system: $(LOGHOSTUNAME)\n' >> $(LOGDIR)/$(LOGHOST)-report 
	-printf "\t$(OM_GCC)\n" >> $(LOGDIR)/$(LOGHOST)-report
	-printf "\t$(OM_BINUTILS)\n" >> $(LOGDIR)/$(LOGHOST)-report
	-printf "\t$(OM_MPC)\n" >> $(LOGDIR)/$(LOGHOST)-report
	-printf "\t$(OM_MPFR)\n" >> $(LOGDIR)/$(LOGHOST)-report
	-printf "\t$(OM_GMP)\n" >> $(LOGDIR)/$(LOGHOST)-report

ifeq ($(OM_LIBC),musl)
	-printf "\t$(OM_MUSL)\n"  >> $(LOGDIR)/$(LOGHOST)-report
endif
ifeq ($(OM_LIBC),uclibc)
	-printf "\t$(OM_UCLIBC)\n"  >> $(LOGDIR)/$(LOGHOST)-report
endif
ifeq ($(OM_LIBC),gnu)
	-printf "\t$(OM_GLIBC)\n"  >> $(LOGDIR)/$(LOGHOST)-report
endif
	
	
	-printf "%s\n" >> $(LOGDIR)/$(LOGHOST)-report
	-printf "%s\n" >> $(LOGDIR)/$(LOGHOST)-report


logreport:

	cp Makefile $(LOGDIR)/makefile_used.txt
	cd $(LOGDIR) && cp $(LOGHOST)-report README
	cd $(LOGDIR) && curl -u '$(LOGUSER)' https://api.github.com/user/repos -d '{"name":"$(LOGNAME)"}'
	cd $(LOGDIR) && git init
	cd $(LOGDIR) && git add *
	cd $(LOGDIR) && git commit -m "$(LOGNAME)"
	cd $(LOGDIR) && git remote add origin https://github.com/$(LOGUSER)/$(LOGNAME).git
	cd $(LOGDIR) && git push origin master


