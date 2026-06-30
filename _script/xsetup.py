import platform

def cmake_threads(number=None):
    if platform.system() == 'Windows':
        return ""
    if number is None:
        number = 16
    number = int(number)
    return f"-j {number}"

build_debug   = True
build_release = False

cmake_build_type = f"-DCMAKE_BUILD_TYPE=Debug"
cmake_build_config = f"--config Debug"
cmake_build_standard = f"-DCMAKE_CXX_STANDARD=23 -DCMAKE_POSITION_INDEPENDENT_CODE=ON"

def Debug():
    global cmake_build_type
    global cmake_build_config
    global build_debug
    global build_release
    build_type = "Debug"
    cmake_build_type = f"-DCMAKE_BUILD_TYPE={build_type}"
    cmake_build_config = f"--config {build_type}"
    build_debug   = True
    build_release = False
    PostCheck()
    pass


def Release():
    global cmake_build_type
    global cmake_build_config
    global build_debug
    global build_release
    build_type = "Release"
    cmake_build_type = f"-DCMAKE_BUILD_TYPE={build_type}"
    cmake_build_config = f"--config {build_type}"
    build_debug   = False
    build_release = True
    PostCheck()
    pass

def PostCheck():
    global cmake_build_type
    global cmake_build_config
    cmake_use_single_build=(platform.system() != 'Windows')
    if not cmake_use_single_build: #windows
        cmake_build_type = ""
    pass

def Output():
    print(f"cmake_build_type: {cmake_build_type}")
    print(f"cmake_build_config: {cmake_build_config}")
    print(f"cmake_build_standard: {cmake_build_standard}")
    print(f"build_debug={build_debug}, build_release={build_release}")

PostCheck()
