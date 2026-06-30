#!/usr/bin/env python3

import tarfile
import zipfile

def auto_extract(src_file: str, dst_dir: str):
    ext = _get_longest_extensions(src_file)
    if src_file.endswith(".zip"):
        return _unzip_source(src_file, dst_dir)
    if src_file.endswith(".tar.gz"):
        return _extract_tarfile(src_file, dst_dir)
    if src_file.endswith(".tar.xz"):
        return _extract_tarfile(src_file, dst_dir)
    if src_file.endswith(".tar.bz2"):
        return _extract_tarfile(src_file, dst_dir)

###################### private ######################

def _get_longest_extensions(filename):
    parts = filename.split('.')
    part_num = len(parts)
    if (part_num <= 1) :
        return ""
    extensions = '.'.join(parts[-(part_num-1):])
    return extensions


def _extract_tarfile(src_file: str, dst_dir: str):
    print(src_file)
    with tarfile.open(src_file) as file:
        if hasattr(tarfile, 'data_filter'):
            file.extractall(dst_dir, filter='data')
        else:
            file.extractall(dst_dir)


def _unzip_source(src_file: str, dst_dir: str):
    with zipfile.ZipFile(src_file, 'r') as zip_ref:
        zip_ref.extractall(dst_dir)


