
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
AM_CPPFLAGS = -I../../report -I../../protocols/lwps -I../../spi -I../../language -I../../utils 


# Applications
bin_PROGRAMS = lscript lsearch ltrec


# MPS libraries
mps_libs = ../../report/libreport.a ../../protocols/lwps/liblwps.a ../../spi/libspi.a ../../language/liblanguage.a ../../utils/libutils.a


# Lscript
lscript_SOURCES = lscript.c
lscript_LDADD = $(mps_libs)


# Lsearch
lsearch_SOURCES = lsearch.c
lsearch_LDADD = $(mps_libs)


# Ltrec
ltrec_SOURCES = ltrec.c
ltrec_LDADD = $(mps_libs)



# Extras to distribute
EXTRA_DIST=Contents



# Clean
clean-local:
    rm -rf $(CLEAN_LOCAL)

