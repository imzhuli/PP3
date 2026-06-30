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
cmake_prefix_path    = f"-DCMAKE_PREFIX_PATH={install_path}"    if (install_path := os.getenv("CMAKE_INSTALL_PREFIX")) else ""
cmake_install_prefix = f"-DCMAKE_INSTALL_PREFIX={install_path}" if (install_path := os.getenv("CMAKE_INSTALL_PREFIX")) else ""

print("#########################################")
print(cmake_install_prefix)
print("#########################################")


src_file = f"{cwd}/src_3rd/librdkafka-v2.10.0.tar.gz"
unzipped_src_dir = f"{build_path}/librdkafka-2.10.0"

###############################################

def build():
    fu.auto_extract(src_file, build_path)

    cmake_file = f"{unzipped_src_dir}/CMakeLists.txt"
    if not cu.fix_cmake(cmake_file):
        return False

    try:
        os.chdir(unzipped_src_dir)
        os.system(
            "cmake "
            f'{xsetup.cmake_build_standard} '
            f'{xsetup.cmake_build_type} '
            f'{cmake_prefix_path} '
            f'{cmake_install_prefix} '
            "-Wno-dev "
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=1 "
            "-DRDKAFKA_BUILD_STATIC=ON "
            "-DBUILD_SHARED_LIBS=OFF "
            "-DWITH_SSL=ON "
            "-DWITH_SASL=ON "
            "-DWITH_LZ4_EXT=OFF "
            "-DWITH_ZLIB=OFF "
            "-DWITH_ZSTD=OFF "
            "-DRDKAFKA_BUILD_TESTS=OFF -DRDKAFKA_BUILD_EXAMPLES=OFF "
            "-DCMAKE_CXX_STANDARD=20 "
            f"-B .build . "
        )
        os.system(f"cmake --build   .build {xsetup.cmake_build_config} -- {xsetup.cmake_threads()}")
        os.system(f"cmake --install .build {xsetup.cmake_build_config}")
    except Exception as e:
        return False
    finally:
        os.chdir(cwd)
    return True

if __name__ == "__main__":
    build()
