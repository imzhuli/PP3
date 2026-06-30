#!/usr/bin/env python3

import _check_env as ce
import os

build_dir   = "__build"
install_dir = "__installed_debug"

############## prepare

ce.remake_dir(build_dir)
#ce.remake_dir(install_dir)
os.environ["CMAKE_INSTALL_PREFIX"] = f"{ce.root_dir}/{install_dir}"

############## build and install

#os.system("python3 ./_script/build_openssl.py")
#os.system("python3 ./_script/build_sasl2.py")
#os.system("python3 ./_script/build_mmdb.py")
#os.system("python3 ./_script/build_rdkfk.py")
os.system("python3 ./_script/build_x_common.py")
os.system("python3 ./_script/build_x_app.py")

