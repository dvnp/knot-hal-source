lib_LIBRARIES = libphy_driver.a
lib_ARDUINO = phy_driver.c phy_driver_nrf24.c

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)

libphy_driver_a_SOURCES = phy_driver.c phy_driver_nrf24.c
libphy_driver_a_CPPFLAGS = $(AM_CFLAGS) -I$(top_srcdir)/src/hal/comm \
								-I$(top_srcdir)/src/nrf24l01

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(lib_LIBRARIES) $(top_srcdir)/libs
	$(MKDIR_P) $(top_srcdir)/hal/arduino && cp $(lib_ARDUINO) $(top_srcdir)/hal/arduino
clean-local:
	$(RM) -r libphy_driver.a
	$(RM) -r $(top_srcdir)/libs
