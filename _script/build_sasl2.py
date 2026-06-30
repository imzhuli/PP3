#!/usr/bin/env python3
import _check_env  as ce
import _file_util  as fu
import os
import xsetup

###############################################

cwd = ce.root_dir
build_path = f"{cwd}/__build"
install_dir = f"{install_path}" if (install_path := os.getenv("CMAKE_INSTALL_PREFIX")) else ""

src_file = f"{cwd}/src_3rd/cyrus-sasl-2.1.28.tar.gz"
unzipped_src_dir = f"{build_path}/cyrus-sasl-2.1.28"

###############################################

def build():
    fu.auto_extract(src_file, build_path)

    try:
        print(f"start building..., src_dir={unzipped_src_dir}")
        os.chdir(unzipped_src_dir)
        os.system(f'./configure --prefix={install_dir!r} --enable-static --disable-shared '
            f"--with-openssl={install_dir!r} "
            "--enable-scram "
            "--disable-sample "
            "--disable-obsolete_cram_attr "
            "--disable-obsolete_digest_attr "
            "--disable-checkapop "
            "--disable-cram "
            "--disable-digest "
            "--disable-otp "
            "--disable-gssapi "
            "--with-saslauthd=no "
            "--with-dblib=none "
            "--with-pic "
        )
        os.system(f"make -j 16")
        os.system(f"make install")
    except Exception as e:
        print(e)
        return False
    finally:
        os.chdir(cwd)
    return True


if __name__ == "__main__":
    build()
