#*****************************************************************************
#       Copyright (C) 1993-2010, FS Consulting LLC. All rights reserved      *
#                                                                            *
#  This notice is intended as a precaution against inadvertent publication   *
#  and does not constitute an admission or acknowledgement that publication  *
#  has occurred or constitute a waiver of confidentiality.                   *
#                                                                            *
#  This software is the proprietary and confidential property                *
#  of FS Consulting LLC.                                                     *
#*****************************************************************************



#
# TDB
#
# / header check for icu
# / header check for basistech
# / clean 
# / enable ICU if BT is enabled?? no
# / test install
# - ./conf files install ??
# - ./scripts files install ??
# - ./test-scripts files ??
# - ./src/php build
#


AC_PREREQ(2.59)
AC_INIT(MPS, 11.0, fschiettecatte@gmail.com)
AC_CONFIG_SRCDIR([src/utils/version.h])
AC_CONFIG_AUX_DIR(config)

AC_CANONICAL_TARGET
# AC_CANONICAL_HOST

AM_INIT_AUTOMAKE

# AC_CONFIG_HEADER([config.h])
AM_CONFIG_HEADER(config.h)


# Define GNU source
AC_GNU_SOURCE



# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_MAKE_SET


# Checks for libraries.


# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h limits.h locale.h \
netdb.h netinet/in.h stddef.h stdlib.h string.h sys/file.h \
sys/ioctl.h sys/socket.h sys/time.h termios.h unistd.h utime.h \
wchar.h wctype.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM


# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_FSEEKO
# AC_FUNC_GETLOADAVG
AC_PROG_GCC_TRADITIONAL
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_FUNC_STRCOLL
AC_FUNC_STRERROR_R
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_UTIME_NULL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([dup2 ftruncate getcwd gethostbyaddr gethostbyname \
gethostname getpagesize gettimeofday inet_ntoa localtime_r memmove \
memset mkdir munmap pow putenv regcomp rmdir setlocale socket sqrt \
strcasecmp strchr strdup strerror strncasecmp strpbrk strrchr strstr \
strtol utime])



# Debug flags
#
# -g to include basic debugging information
# -g3 to include all debugging information
# -ggdb to include GDB specific debugging information
# -ggdb3 to include all GDB specific debugging information
#
#DEBUG_CPPFLAGS="-DDEBUG"
DEBUG_CPPFLAGS=""
DEBUG_CFLAGS="-DDEBUG -ggdb3"


# Defines, these are there so that the code
# can label the build type
DEBUG_DEFINE="-DDEBUG"
PROFILE_DEFINE="-DPROFILE"
COVERAGE_DEFINE="-DCOVERAGE"
OPTIMIZED_DEFINE="-DOPTIMIZED"


# Check for build type
AC_MSG_CHECKING(for build type)
AC_ARG_WITH(build-type,
    AC_HELP_STRING([--with-build-type=TYPE],[specify build type (debug|(prof)ile|(cov)erage|(opt)imized - default debug)]),
    [WITH_BUILD_TYPE=$withval],[WITH_BUILD_TYPE=debug])

# Profile build
if test "$WITH_BUILD_TYPE" == "profile" -o "$WITH_BUILD_TYPE" == "prof"; then

    # Profiler flags
    #
    # -pg to include profile information
    #
    CPPFLAGS="${CPPFLAGS} ${DEBUG_CPPFLAGS}"
    CFLAGS="${DEBUG_CFLAGS} ${PROFILE_DEFINE} -pg"
    CXXFLAGS="${DEBUG_CFLAGS} -pg"

# Coverage build
elif test "$WITH_BUILD_TYPE" == "coverage" -o "$WITH_BUILD_TYPE" == "cov"; then

    # Coverage flags
    #
    # -fprofile-arcs -ftest-coverage to include coverage information
    #
    CPPFLAGS="${CPPFLAGS} ${DEBUG_CPPFLAGS}"
    CFLAGS="${DEBUG_CFLAGS} ${COVERAGE_DEFINE} -fprofile-arcs -ftest-coverage"
    CXXFLAGS="${DEBUG_CFLAGS} -fprofile-arcs -ftest-coverage"

# Optimized build
elif test "$WITH_BUILD_TYPE" == "optimized" -o "$WITH_BUILD_TYPE" == "opt"; then

    # Optimisation flags
    #
    # -O3 for full optimization
    #
    OPTIMIZATION_CPPFLAGS="-DNDEBUG"
    OPTIMIZATION_CFLAGS="-O3"


    CPPFLAGS="${CPPFLAGS} ${OPTIMIZATION_CPPFLAGS}"
    CFLAGS="${OPTIMIZATION_CFLAGS} ${OPTIMIZED_DEFINE}"
    CXXFLAGS="${OPTIMIZATION_CFLAGS}"

    # Platform specific optimization flags
    #
    # -march=pentium4
    # -march=x86-64     - generic x86-64
    # -march=opteron    - opteron
    # -march=nocona     - 64bit Xeon
    # 
    case $host in
        i386-apple-* ) CFLAGS="${CFLAGS} -march=pentium4";;
        i686-apple-* ) CFLAGS="${CFLAGS} -march=pentium4";;
        i686-* ) CFLAGS="${CFLAGS} -march=pentium4";;
        x86_64-* ) CFLAGS="${CFLAGS} -march=x86-64";;
#       x86_64-* ) CFLAGS="${CFLAGS} -march=opteron";;
#       x86_64-* ) CFLAGS="${CFLAGS} -march=nocona";;
        * ) ;;
    esac

# Default build, also debug build
else

    CPPFLAGS="${CPPFLAGS} ${DEBUG_CPPFLAGS}"
    CFLAGS="${DEBUG_CFLAGS}"
    CXXFLAGS="${DEBUG_CFLAGS}"

fi
AC_MSG_RESULT([$WITH_BUILD_TYPE])




# Generic preprocessor defines
#
# -D_GNU_SOURCE for gnu source
# -D_REENTRANT for re-entrant code
#
# See: http://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
#
CPPFLAGS="${CPPFLAGS} -D_GNU_SOURCE -D_REENTRANT"




# Generic compiler flags
#
# -pipe to pipe results between files
# -D_STL=std ??
# -fexceptions to enable exception handling
# -ffor-scope to limit the scope of variables
# -fno-gnu-keywords to disable 'typeof'
#
# -m32 to force 32 bit s/w
# -m64 to force 64 bit s/w
#
CFLAGS="${CFLAGS} -pipe"
CXXFLAGS="${CXXFLAGS} -D_STL=std -fexceptions -ffor-scope -fno-gnu-keywords"




# Generic compiler warning flags
#
# -Wall for all warnings
# -Wcast-align for cast alignment warnings
# -Wconversion for implicit parameter conversion
# -Werror to convert warnings into errors
# -Wmissing-prototypes for missing prototypes warnings
# -Wno-pointer-sign to turn off warning for different signedness for pointer argument passing (gcc 4.x only)
# -Wpadded for padding inclusion
# -Wpointer-arith for pointer arithmetic warnings
# -Wshadow for shadow parameters
# -Wsign-compare for signed/unsigned comparisons
#
CFLAGS="${CFLAGS} -Wall -Wcast-align -Wmissing-prototypes -Wno-pointer-sign -Wpointer-arith -Wshadow -Wsign-compare"




# Platform specific compiler flags
#
# -pthread for pthreads (except MacOS X)
#
case $host in
    i386-apple-* ) ;;
    i686-apple-* ) ;;
    * ) CFLAGS="${CFLAGS} -pthread";;
esac




# Generic linker flags
#
# -fexceptions to enable exception handling
#
LDFLAGS="${LDFLAGS} -fexceptions"




# Platform specific linker flags
#
# -Wl,-Bsymbolic to limit interposition (Linux/Intel/AMD only)
#
case $host in
    i686-pc-* ) LDFLAGS="${LDFLAGS} -Wl,-Bsymbolic";;
    x86_64-* ) LDFLAGS="${LDFLAGS} -Wl,-Bsymbolic";;
    * ) ;;
esac




# Check for MeCab
#
# -DMPS_ENABLE_MECAB
#
# -I/usr/local/mecab/include
#
# -L/usr/local/mecab/lib -lmecab
#
AC_MSG_CHECKING(for MeCab)
AC_ARG_WITH(mecab,
    AC_HELP_STRING([--with-mecab@<:@=DIR@:>@],[include MeCab support (Katakana-to-Hiragana normalization functionality requires ICU) (default no, directory defaults to: '/usr/local/mecab')]),
    [WITH_MECAB=$withval],[WITH_MECAB=no])

if test "$WITH_MECAB" != "no"; then

    AC_MSG_RESULT([])

    if test "$WITH_MECAB" == "yes"; then
        MPS_MECAB_ROOT="/usr/local/mecab"
    else
        MPS_MECAB_ROOT=$WITH_MECAB
    fi

    AC_CHECK_FILE([$MPS_MECAB_ROOT],, AC_MSG_ERROR([MeCab was not found in: '$MPS_MECAB_ROOT']))
#   AC_CHECK_FILE([$MPS_MECAB_ROOT/include/mecab.h],, AC_MSG_ERROR([MeCab was not found in: '$MPS_MECAB_ROOT']))

    CPPFLAGS="${CPPFLAGS} -DMPS_ENABLE_MECAB"

    MPS_MECAB_INCLUDES=" -I$MPS_MECAB_ROOT/include"
    AC_SUBST(MPS_MECAB_INCLUDES)

    MPS_MECAB_LIBS=" -L$MPS_MECAB_ROOT/lib -lmecab"
    AC_SUBST(MPS_MECAB_LIBS)

    with_mecab=yes
    AC_MSG_RESULT([MeCab was enabled])
else
    with_mecab=no
    AC_MSG_RESULT([$with_mecab])
fi
AM_CONDITIONAL([MPS_ENABLE_MECAB], [test "$with_mecab" = yes])




# Check for ICU
#
# -DMPS_ENABLE_ICU
#
# -I/usr/local/icu/include
#
# -L/usr/local/icu/lib -licui18n -licuuc -licudata
#
AC_MSG_CHECKING(for ICU)
AC_ARG_WITH(icu,
    AC_HELP_STRING([--with-icu@<:@=DIR@:>@],[include ICU support (default no, directory defaults to: '/usr/local/icu')]),
    [WITH_ICU=$withval],[WITH_ICU=no])

if test "$WITH_ICU" != "no"; then

    AC_MSG_RESULT([])

    if test "$WITH_ICU" == "yes"; then
        MPS_ICU_ROOT="/usr/local/icu"
    else
        MPS_ICU_ROOT=$WITH_ICU
    fi

    AC_CHECK_FILE([$MPS_ICU_ROOT],, AC_MSG_ERROR([ICU was not found in: '$MPS_ICU_ROOT']))
#   AC_CHECK_FILE([$MPS_ICU_ROOT/include/unicode/uversion.h],, AC_MSG_ERROR([ICU was not found in: '$MPS_ICU_ROOT']))

    CPPFLAGS="${CPPFLAGS} -DMPS_ENABLE_ICU -DU_CHARSET_IS_UTF8=1"

    MPS_ICU_INCLUDES=" -I$MPS_ICU_ROOT/include"
    AC_SUBST(MPS_ICU_INCLUDES)

    MPS_ICU_LIBS="-L$MPS_ICU_ROOT/lib -licui18n -licuuc -licudata"
    AC_SUBST(MPS_ICU_LIBS)

    with_icu=yes
    AC_MSG_RESULT([ICU was enabled])
else
    with_icu=no
    AC_MSG_RESULT([$with_icu])
fi
AM_CONDITIONAL([MPS_ENABLE_ICU], [test "$with_icu" = yes])




# Check for Google malloc library
#
# -L/usr/local/google/lib -ltcmalloc
#
AC_MSG_CHECKING(for Google malloc library)
AC_ARG_WITH(google-malloc,
    AC_HELP_STRING([--with-google-malloc@<:@=DIR@:>@],[include Google malloc library (default no, directory defaults to: '/usr/local/google')]),
    [WITH_GOOGLE_MALLOC=$withval],[WITH_GOOGLE_MALLOC=no])

if test "$WITH_GOOGLE_MALLOC" != "no"; then

    AC_MSG_RESULT([])

    if test "$WITH_GOOGLE_MALLOC" == "yes"; then
        GOOGLE_TOOLS_ROOT="/usr/local/google"
    else
        GOOGLE_TOOLS_ROOT=$WITH_GOOGLE_MALLOC
    fi

    AC_CHECK_FILE([$GOOGLE_TOOLS_ROOT],, AC_MSG_ERROR([Google malloc library was not found in: '$GOOGLE_TOOLS_ROOT']))
#   AC_CHECK_FILE([$GOOGLE_TOOLS_ROOT/lib/libtcmalloc.a],, AC_MSG_ERROR([Google malloc library was not found in: '$GOOGLE_TOOLS_ROOT']))

    LIBS="${LIBS} -L${GOOGLE_TOOLS_ROOT}/lib -ltcmalloc"

    AC_MSG_RESULT([yes])
else
    AC_MSG_RESULT([no])
fi




# Check for profile
AC_MSG_CHECKING(for profile)
AC_ARG_WITH(profile,
    AC_HELP_STRING([--with-profile=TYPE],[specify profile (generate|use|(no)ne - default none)]),
    [WITH_PROFILE=$withval],[WITH_PROFILE=none])

# Profile generate
if test "$WITH_PROFILE" == "generate"; then

    # Profile flags
    #
    # -fprofile-generate to generate profile
    #
    CFLAGS="-fprofile-generate"
    CXXFLAGS="-fprofile-generate"

# Profile use
elif test "$WITH_PROFILE" == "use"; then

    # Profile flags
    #
    # -fprofile-use to use profile
    #
    CFLAGS="${CFLAGS} -fprofile-use"
    CXXFLAGS="${CXXFLAGS} -fprofile-use"

fi
AC_MSG_RESULT([$WITH_PROFILE])




# Check for combine
AC_MSG_CHECKING(for combine)
AC_ARG_WITH(combine,
    AC_HELP_STRING([--with-combine=TYPE],[specify combine (yes|no - default no)]),
    [WITH_COMBINE=$withval],[WITH_COMBINE=no])

# Combine
if test "$WITH_COMBINE" != "no"; then

    # Combine flags
    #
    # -combine to combine
    #
    CFLAGS="${CFLAGS} -combine"
    CXXFLAGS="${CXXFLAGS} -combine"

fi
AC_MSG_RESULT([$WITH_COMBINE])




# Default libraries
#
# -lm for maths library
# -lstdc++ for c++ library (added because we link with gcc rather than g++)
#
LIBS="${LIBS} -lm"

if test "$with_mecab" == "yes" -o "$with_icu" == "yes"; then
    LIBS="${LIBS} -lstdc++"
fi




# Platform specific libraries
#
# -liconv for character set conversion (on MacOS X)
# -lpthread for posix threads library (on Linux/Intel/AMD)
# -lrt for posix real time library (on Linux/Intel/AMD)
#
case $host in
    i386-apple-* ) LIBS="${LIBS} -liconv";;
    i686-apple-* ) LIBS="${LIBS} -liconv";;
    i686-* ) LIBS="${LIBS} -lpthread -lrt";;
    x86_64-* ) LIBS="${LIBS} -lpthread -lrt";;
    * ) ;;
esac




# Additional things to delete when we clean
case $host in
    i386-apple-* ) CLEAN_LOCAL="*~ .DS_Store :2eDS_Store";;
    i686-apple-* ) CLEAN_LOCAL="*~ .DS_Store :2eDS_Store";;
    i686-* ) CLEAN_LOCAL="*~ .DS_Store :2eDS_Store .AppleDouble .svn/.AppleDouble";;
    x86_64-* ) CLEAN_LOCAL="*~ .DS_Store :2eDS_Store .AppleDouble .svn/.AppleDouble";;
    * ) ;;
esac
AC_SUBST(CLEAN_LOCAL)




# Generate the makefiles
AC_CONFIG_FILES([Makefile src/Makefile src/utils/Makefile \
        src/language/Makefile src/language/icu/Makefile \
        src/protocols/Makefile src/protocols/lwps/Makefile \
        src/spi/Makefile src/report/Makefile src/parsers/Makefile \
        src/gateway/Makefile src/search/Makefile src/server/Makefile \
        src/clients/Makefile src/clients/spi/Makefile src/clients/lwps/Makefile \
        src/misc/Makefile])
AC_OUTPUT














