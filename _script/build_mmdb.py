#!/usr/bin/env python3
import _file_util  as fu
import _check_env  as ce
import _cmake_util as cu
import getopt
import os
import sys
import xsetup

if __name__ == "__main__":
    try:
        argv = sys.argv[1:]
        opts, args = getopt.getopt(argv, "r")
    except getopt.GetoptError:
        sys.exit(2)
    for opt, arg in opts:
        if opt == "-r":
            xsetup.Release()
        pass
xsetup.Output()

###############################################

cwd = ce.root_dir
build_path = f"{cwd}/__build"
cmake_install_prefix = f"-DCMAKE_INSTALL_PREFIX={install_path}" if (install_path := os.getenv("CMAKE_INSTALL_PREFIX")) else ""

src_file = f"{cwd}/src_3rd/libmaxminddb-1.9.1.tar.gz"
unzipped_src_dir = f"{build_path}/libmaxminddb-1.9.1"

###############################################

def build():
    fu.auto_extract(src_file, build_path)

    try:
        os.chdir(unzipped_src_dir)
        cu.ensure_cmake_minimum_required("CMakeLists.txt")
        os.system(
            'cmake '
            f'{xsetup.cmake_build_standard} '
            f'{xsetup.cmake_build_type} '
            f'{cmake_install_prefix} '
            '-Wno-dev '
            '-DBUILD_SHARED_LIBS=OFF '
            '-DBUILD_TESTING=OFF '
            '-B .build . ')
        os.system(f"cmake --build   .build {xsetup.cmake_build_config} -- {xsetup.cmake_threads()}")
        os.system(f"cmake --install .build {xsetup.cmake_build_config}")
    except Exception as e:
        print(f"error: {e}")
        return False
    finally:
        os.chdir(cwd)
    return True


if __name__ == "__main__":
    build()
