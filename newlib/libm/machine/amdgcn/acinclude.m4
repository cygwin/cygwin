if test "${libm_machine_dir}" = "amdgcn"; then
  dnl The vector math library cannot be built in SIMT mode because there are
  dnl no vector types available.
  AC_CACHE_CHECK([for amdgcn SIMT mode], newlib_cv_amdgcn_simt, [dnl
    AC_PREPROC_IFELSE([AC_LANG_PROGRAM(
[[#ifndef __SIMT__
# error "Not SIMT mode"
#endif
]])], [newlib_cv_amdgcn_simt="yes"], [newlib_cv_amdgcn_simt="no"])])
fi

AM_CONDITIONAL(AMDGCN_SIMT, test "$newlib_cv_amdgcn_simt" = "yes")
