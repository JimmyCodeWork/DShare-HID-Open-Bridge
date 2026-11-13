# SPDX-FileCopyrightText: 2026 DShare-HID Open Bridge contributors
# SPDX-License-Identifier: MIT

# Windows runtime DLL helpers for builds that use prebuilt Qt and OpenSSL instead of vcpkg.
#
# With the vcpkg toolchain, vcpkg's app-local deployment copies dependent DLLs next to each
# executable. Prebuilt Qt and OpenSSL packages do not do that, so executables in the build tree
# (the app and the unit tests) would fail to start with a missing-DLL error. These helpers fill
# that gap and are no-ops when vcpkg is in use or on other platforms.

include_guard(GLOBAL)

# Directory holding the OpenSSL DLLs, or empty if it cannot be determined.
function(dsh_openssl_bin_dir out_var)
  set(_dir "")
  if(DEFINED OPENSSL_ROOT_DIR AND EXISTS "${OPENSSL_ROOT_DIR}/bin")
    set(_dir "${OPENSSL_ROOT_DIR}/bin")
  elseif(OPENSSL_INCLUDE_DIR)
    get_filename_component(_root "${OPENSSL_INCLUDE_DIR}" DIRECTORY)
    if(EXISTS "${_root}/bin")
      set(_dir "${_root}/bin")
    endif()
  endif()
  set(${out_var} "${_dir}" PARENT_SCOPE)
endfunction()

# Directory holding the Qt DLLs, or empty if it cannot be determined.
function(dsh_qt_bin_dir out_var)
  set(_dir "")
  if(QT6_INSTALL_PREFIX AND QT6_INSTALL_BINS AND EXISTS "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}")
    set(_dir "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}")
  endif()
  set(${out_var} "${_dir}" PARENT_SCOPE)
endfunction()

# Copy the Qt and OpenSSL DLLs that <target> needs into its output directory after each build.
function(dsh_deploy_runtime_dlls target)
  if(NOT WIN32 OR VCPKG_TOOLCHAIN)
    return()
  endif()

  find_package(OpenSSL ${REQUIRED_OPENSSL_VERSION} REQUIRED COMPONENTS SSL Crypto)
  dsh_openssl_bin_dir(_openssl_bin)
  set(_openssl_dlls)
  if(_openssl_bin)
    file(GLOB _openssl_dlls "${_openssl_bin}/libcrypto-3*.dll" "${_openssl_bin}/libssl-3*.dll")
  endif()
  if(NOT _openssl_dlls)
    message(FATAL_ERROR
      "OpenSSL DLLs not found. Set OPENSSL_ROOT_DIR to an OpenSSL 3 installation that has a bin "
      "folder, for example <Qt>/Tools/OpenSSLv3/Win_x64.")
  endif()

  # $<TARGET_RUNTIME_DLLS> covers the Qt libraries (imported SHARED targets). FindOpenSSL
  # creates UNKNOWN library targets, so the OpenSSL DLLs are listed explicitly.
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_RUNTIME_DLLS:${target}> ${_openssl_dlls} $<TARGET_FILE_DIR:${target}>
    COMMAND_EXPAND_LISTS
    VERBATIM
  )
endfunction()
