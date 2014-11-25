
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
AM_CPPFLAGS = -I../../language -I../../utils $(MPS_ICU_INCLUDES)


# Libraries
lib_LIBRARIES = libicu.a


# ICU library
libicu_a_SOURCES = IcuErr.h \
    IcuUnicodeNormalizer.cpp IcuUnicodeNormalizer.h \
    IcuUtils.cpp IcuUtils.h \
    IcuWrappers.cpp IcuWrappers.h 



# Extras to distribute
EXTRA_DIST=Contents



# Clean
clean-local:
    rm -rf $(CLEAN_LOCAL)

