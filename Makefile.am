AM_MAKEFLAGS = --no-print-directory
SUBDIRS = src/spi src/nrf24l01 src/hal/storage src/hal/time src/hal/log src/hal/comm
AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)
AM_LDFLAGS = $(BUILD_LDFLAGS)

bin_PROGRAMS = proxy/spiproxyd src/nrfd/nrfd src/phyemud/phyemud \
							examples/uthingd tools/rpirx tools/rpitx tools/frag1

proxy_spiproxyd_SOURCES = proxy/main.c
proxy_spiproxyd_LDADD = libs/libspi.a libs/libnrf24l01.a @GLIB_LIBS@
proxy_spiproxyd_LDFLAGS = $(AM_LDFLAGS)
proxy_spiproxyd_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@

src_nrfd_nrfd_SOURCES = src/nrfd/main.c \
				src/nrfd/manager.h src/nrfd/manager.c
src_nrfd_nrfd_LDADD = libs/libhallog.a \
				libs/libhalcomm.a \
				libs/libhaltime.a \
				libs/libhalstorage.a \
				libs/libnrf24l01.a \
				libs/libspi.a \
				@GLIB_LIBS@
src_nrfd_nrfd_LDFLAGS = $(AM_LDFLAGS)
src_nrfd_nrfd_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src \
				-I$(top_srcdir)/src/hal/comm

src_phyemud_phyemud_SOURCES = src/phyemud/main.c \
			src/hal/comm/comm_private.h \
			src/phyemud/manager.h src/phyemud/manager.c \
			src/phyemud/phyunix.c src/phyemud/physerial.c
src_phyemud_phyemud_LDADD = @GLIB_LIBS@
src_phyemud_phyemud_LDFLAGS = $(AM_LDFLAGS)
src_phyemud_phyemud_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src \
				-I$(top_srcdir)/src/hal/comm

examples_uthingd_SOURCES = examples/uthing.c \
			src/phyemud/phyunix.c
examples_uthingd_LDFLAGS = $(AM_LDFLAGS)
examples_uthingd_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src \
				-I$(top_srcdir)/src/hal/comm

tools_rpirx_SOURCES = tools/rpirx.c
tools_rpirx_LDADD = libs/libnrf24l01.a libs/libspi.a \
				 @GLIB_LIBS@
tools_rpirx_LDFLAGS = $(AM_LDFLAGS)
tools_rpirx_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src \
				-I$(top_srcdir)/src/spi \
				-I$(top_srcdir)/src/nrf24l01

tools_rpitx_SOURCES = tools/rpitx.c
tools_rpitx_LDADD = libs/libnrf24l01.a libs/libspi.a \
				 @GLIB_LIBS@
tools_rpitx_LDFLAGS = $(AM_LDFLAGS)
tools_rpitx_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src \
				-I$(top_srcdir)/src/spi \
				-I$(top_srcdir)/src/nrf24l01

tools_frag1_SOURCES = tools/frag1.c
tools_frag1_LDADD = libs/libnrf24l01.a libs/libspi.a \
				 @GLIB_LIBS@
tools_frag1_LDFLAGS = $(AM_LDFLAGS)
tools_frag1_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src \
				-I$(top_srcdir)/src/spi \
				-I$(top_srcdir)/src/nrf24l01

DISTCLEANFILES =

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp compile missing install-sh

clean-local:
	$(RM) -r proxy/spiproxyd nrf/nrfd
