#!/usr/bin/env python3
import _file_util  as fu
import _check_env  as ce
import _cmake_util as cu
import getopt
import os
import sys
import xsetup
import platform

j_threads = None
if __name__ == "__main__":
    try:
        argv = sys.argv[1:]
        opts, args = getopt.getopt(argv, "rj:")
    except getopt.GetoptError:
        sys.exit(2)
    for opt, arg in opts:
        if opt == "-r":
            xsetup.Release()
        if opt == "-j":
            j_threads = " -j%s " % arg
        pass
xsetup.Output()

###############################################

cwd = ce.root_dir
src_path = cwd + "/src_common"
build_path = cwd + "/__build/x_common"
cmake_install_prefix = f"-DCMAKE_INSTALL_PREFIX={install_path}" if (install_path := os.getenv("CMAKE_INSTALL_PREFIX")) else ""
libxel_install_dir  = ce.xel_release_dir if (xsetup.build_release) else ce.xel_debug_dir
if libxel_install_dir is None:
    raise "libxel not found"
cmake_libxel_prefix = f"-DXEL_PREFIX={libxel_install_dir}"

#######################################

print(cmake_libxel_prefix)

os.system(
    'cmake '
    f'{xsetup.cmake_build_standard} '
    f'{xsetup.cmake_build_type} '
    f'{cmake_install_prefix} '
    f'{cmake_libxel_prefix} '
    f'-Wno-dev -DCMAKE_EXPORT_COMPILE_COMMANDS=1 '
    f'-B "{build_path}" -S "{src_path}"')
os.system(f'cmake --build   "{build_path}" {xsetup.cmake_build_config} -- {xsetup.cmake_threads()}')
os.system(f'cmake --install "{build_path}" {xsetup.cmake_build_config}')
