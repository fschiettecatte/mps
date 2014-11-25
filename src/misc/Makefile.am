
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
AM_CPPFLAGS = -I../search -I../report -I../protocols/lwps -I../spi -I../language -I../utils


# Applications
bin_PROGRAMS = defazio parser verify machenv


# MPS libraries
mps_base_libs = ../search/libsearch.a ../report/libreport.a ../protocols/lwps/liblwps.a ../spi/libspi.a ../language/liblanguage.a ../utils/libutils.a

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


# Defazio
defazio_SOURCES = defazio.c
defazio_LDADD = $(mps_libs)


# Parser
parser_SOURCES = parser.c
parser_LDADD = $(mps_libs)


# Verify
verify_SOURCES = verify.c
verify_LDADD = $(mps_libs)


# Machenv
machenv_SOURCES = machenv.c



# Extras to distribute
EXTRA_DIST=Contents



# Clean
clean-local:
    rm -rf $(CLEAN_LOCAL)

