############################################################################
# Prelude 

# Initialize AC: ACINIT( package_name, version, bug_report_address)
AC_INIT([zrfcxlsreader], [0.2])

# install-sh decomp missing will be in build-aux dir
AC_CONFIG_AUX_DIR([build-aux])


# Initialize Automake
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Safety: File must exist
AC_CONFIG_SRCDIR([src/zrfcxlsreader.cpp])

AC_CONFIG_MACRO_DIR([m4])


############################################################################
# Check for programs 

# Let's set a C++ compiler
AC_PROG_CXX
AC_PROG_CC


############################################################################
# Checks for libraries 

############################################################################
# Checks for header files 

############################################################################
# Checks for typedefs, structures, and compiler characteristics

############################################################################
# Checks for library functions 

############################################################################
# Output fles

LT_INIT

# Make config header
AC_CONFIG_HEADERS([config.h])


# Automake creates FILE.in for each FILE that has a FILE.am.
AC_CONFIG_FILES([Makefile src/Makefile])

# Finally produce "configure" script
AC_OUTPUT
