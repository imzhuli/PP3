#!/usr/bin/env python3

import sys
import os
import shutil
import tarfile

root_dir = None
xel_debug_dir   = None
xel_release_dir = None

MIN_PY_VERSION_MAJOR = 3
MIN_PY_VERSION_MINOR = 7

def remove_dir(dir: str):
    if root_dir is None:
        raise "root working dir not properly set"
    if dir.startswith("/"):
        raise "ce has own root dir setup, use relative path instead"
    shutil.rmtree(dir, onerror=__remove_readonly_callback)


def remake_dir(dir: str):
    remove_dir(dir)
    os.makedirs(dir)


#####################################################
#  private functions
#####################################################

_cwd = os. getcwd()

def _check_env():
    valid = _check_version() and _check_tag_file()
    if not valid:
        print("Failed to pass all env requirements")
        return False

    global root_dir
    global xel_debug_dir
    global xel_release_dir
    root_dir = _cwd
    xel_debug_dir   = root_dir + "/../xel/__installed_debug"
    xel_release_dir = root_dir + "/../xel/__installed_release"
    if not _check_xel_tag_file():
        print("xel dependency dir not found")

    print(f"CheckEnv: Rootdir={root_dir}")
    print(f"xel_debug_dir:   {xel_debug_dir}")
    print(f"xel_release_dir: {xel_release_dir}")
    return True

def _check_version():
    py_version_major = sys.version_info[0]
    py_version_minor = sys.version_info[1]
    if py_version_major < MIN_PY_VERSION_MAJOR:
        print("invalid python major version: at least %u" %
              MIN_PY_VERSION_MAJOR)
        return False
    if py_version_major == MIN_PY_VERSION_MAJOR and py_version_minor < MIN_PY_VERSION_MINOR:
        print("invalid python ninor version: at least %u" %
              MIN_PY_VERSION_MINOR)
        return False
    return True

def _check_tag_file():
    tag_filename = f"{_cwd}/CoreXApp.tag"
    if not os.path.isfile(tag_filename):
        print("tag file not found, please check the working directory")
        return False
    return True

def _check_xel_tag_file():
    if root_dir is None:
        print("root_dir is not ready")
        return False
    xel_tag = root_dir + "/../xel/CoreXX.tag"
    if not os.path.isfile(xel_tag):
        print("xel tag file not found, please check the working directory")
        return False
    if not os.path.isdir(xel_debug_dir):
        print("xel debug installation not found")
    if not os.path.isdir(xel_release_dir):
        print("xel release installation not found")
    return True

def __remove_readonly_callback(func, path, excinfo):
    error = excinfo[0]
    if error is PermissionError:
        os.chmod(path, stat.S_IWRITE | stat.S_IREAD)
        func(path)
    elif error is FileNotFoundError:
        return
    else:
        raise error

##############################################
# init and run ched
##############################################

if not _check_env():
    raise "failed to check env"
