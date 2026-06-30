#!/usr/bin/env python3

import tarfile
import os
import xsetup

cwd = os.getcwd()
unzip_dir = f"{cwd}/_3rd_build"

libname = "mmdb"
src_file = f"{cwd}/_3rd_source/libmaxminddb-1.9.1.tar.gz"
unzipped_src_dir = f"{unzip_dir}/libmaxminddb-1.9.1"
install_dir = f"{cwd}/_3rd_installed"


def build():
    try:
        file = tarfile.open(src_file)
        file.extractall(unzip_dir)
    finally:
        file.close()

    try:
        os.chdir(unzipped_src_dir)
        os.system(
            "cmake "
            f"{xsetup.cmake_build_type} "
            "-Wno-dev "
            "-DCMAKE_CXX_STANDARD=20 "
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=1 "
            "-DBUILD_SHARED_LIBS=OFF "
            "-DBUILD_TESTING=OFF "
            f"-DCMAKE_INSTALL_PREFIX={install_dir!r} -B build ."
        )
        os.system(f"cmake --build build {xsetup.cmake_build_config} -- all")
        os.system(f"cmake --install build {xsetup.cmake_build_config}")
    except Exception as e:
        print(f"{libname} error: %s" % e)
        return False
    finally:
        os.chdir(cwd)
    print(f"{libname} installed to {install_dir!r}")
    return True


if __name__ == "__main__":
    build()
