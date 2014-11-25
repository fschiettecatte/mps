
#*****************************************************************************
#       Copyright (C) 1993-2011, FS Consulting LLC. All rights reserved      *
#                                                                            *
#  This notice is intended as a precaution against inadvertent publication   *
#  and does not constitute an admission or acknowledgement that publication  *
#  has occurred or constitute a waiver of confidentiality.                   *
#                                                                            *
#  This software is the proprietary and confidential property                *
#  of FS Consulting LLC.                                                     *
#*****************************************************************************


# Includes
AM_CPPFLAGS = -I../report -I../protocols/lwps -I../spi -I../language -I../utils


# Server sources
server_sources = srvr_http.c  srvr_http.h \
          srvr_lwps.c  srvr_lwps.h \
          server.c  server.h


# Applications
bin_PROGRAMS = mpsserver mpsgateway


# MPS libraries
mps_base_libs = ../report/libreport.a ../protocols/lwps/liblwps.a ../spi/libspi.a ../language/liblanguage.a ../utils/libutils.a

if MPS_ENABLE_MECAB
  mps_mecab_libs = $(MPS_MECAB_LIBS)
else
  mps_mecab_libs =
endif

if MPS_ENABLE_ICU
  mps_icu_libs = ../language/icu/libicu.a $(MPS_ICU_LIBS)
else
  mps_icu_libs =
endif

mps_libs = $(mps_base_libs) $(mps_mecab_libs) $(mps_icu_libs)



# MPS Gateway
mpsgateway_SOURCES = $(server_sources)
mpsgateway_LDADD = ../gateway/libgateway.a $(mps_libs)


# MPS Server
mpsserver_SOURCES = $(server_sources)
mpsserver_LDADD = ../search/libsearch.a $(mps_libs)



# Extras to distribute
EXTRA_DIST=Contents



# Clean
clean-local:
    rm -rf $(CLEAN_LOCAL)

