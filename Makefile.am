AUTOMAKE_OPTIONS = foreign nostdinc

SUBDIRS = src docs
if ENABLE_TESTS
SUBDIRS += tests
endif
if BUILD_EXAMPLES
SUBDIRS += example
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libssh2.pc

include_HEADERS = \
  include/libssh2.h \
  include/libssh2_publickey.h \
  include/libssh2_sftp.h

NETWAREFILES =  nw/keepscreen.c \
  nw/nwlib.c \
  nw/GNUmakefile \
  nw/test/GNUmakefile

DISTCLEANFILES =

VMSFILES = vms/libssh2_make_example.dcl vms/libssh2_make_help.dcl \
  vms/libssh2_make_kit.dcl vms/libssh2_make_lib.dcl vms/man2help.c \
  vms/readme.vms vms/libssh2_config.h

WIN32FILES = win32/GNUmakefile \
  win32/libssh2_config.h win32/config.mk win32/rules.mk \
  win32/libssh2.rc

OS400FILES = os400/README400 os400/initscript.sh os400/make.sh \
  os400/make-src.sh os400/make-rpg.sh os400/make-include.sh \
  os400/os400sys.c os400/ccsid.c \
  os400/libssh2_config.h os400/macros.h os400/libssh2_ccsid.h \
  os400/include/alloca.h os400/include/sys/socket.h os400/include/stdio.h \
  os400/libssh2rpg/libssh2.rpgle.in \
  os400/libssh2rpg/libssh2_ccsid.rpgle.in \
  os400/libssh2rpg/libssh2_publickey.rpgle \
  os400/libssh2rpg/libssh2_sftp.rpgle \
  Makefile.os400qc3.inc

EXTRA_DIST = $(WIN32FILES) $(NETWAREFILES) get_ver.awk \
  maketgz NMakefile RELEASE-NOTES libssh2.pc.in $(VMSFILES) config.rpath \
  CMakeLists.txt cmake git2news.pl libssh2-style.el README.md $(OS400FILES) \
  buildconf

ACLOCAL_AMFLAGS = -I m4

.PHONY: ChangeLog
ChangeLog:
	echo "see NEWS" > ./ChangeLog
DISTCLEANFILES += ChangeLog

dist-hook:
	rm -rf $(top_builddir)/tests/log
	find $(distdir) -name "*.dist" -exec rm {} \;
	(distit=`find $(srcdir) -name "*.dist"`; \
	for file in $$distit; do \
	  strip=`echo $$file | sed -e s/^$(srcdir)// -e s/\.dist//`; \
	  cp $$file $(distdir)$$strip; \
	done)

# Code Coverage

init-coverage:
	make clean
	lcov --directory . --zerocounters

COVERAGE_CCOPTS ?= "-g --coverage"
COVERAGE_OUT ?= docs/coverage

build-coverage:
	make CFLAGS=$(COVERAGE_CCOPTS) check
	mkdir -p $(COVERAGE_OUT)
	lcov --directory . --output-file $(COVERAGE_OUT)/$(PACKAGE).info \
		--capture

gen-coverage:
	genhtml --output-directory $(COVERAGE_OUT) \
		$(COVERAGE_OUT)/$(PACKAGE).info \
		--highlight --frames --legend \
		--title "$(PACKAGE_NAME)"

coverage: init-coverage build-coverage gen-coverage

checksrc:
	perl ci/checksrc.pl -i4 -m79 \
	  -ASIZEOFNOPAREN \
	  -ASNPRINTF \
	  -ACOPYRIGHT \
	  -AFOPENMODE \
	  -AEQUALSNULL \
	  -ANOTEQUALSZERO \
	  -ATYPEDEFSTRUCT \
	  -Wsrc/libssh2_config.h src/*.[ch] include/*.h example/*.c \
	  tests/*.[ch]
