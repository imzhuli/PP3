#!/usr/bin/env python3

import os
import re
import shutil
import stat
import tempfile

__ignore_cmake_params = """\
if (DEFINED CMAKE_POLICY_VERSION_MINIMUM)
    set(CMAKE_POLICY_VERSION_MINIMUM ${CMAKE_POLICY_VERSION_MINIMUM})
endif()
"""

__fix_compiler_flags = """
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format-truncation")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-implicit-function-declaration")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-nested-externs")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-int-conversion")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-sign-conversion")
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format-truncation")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-implicit-function-declaration")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-nested-externs")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-int-conversion")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-sign-conversion")
elseif (CMAKE_C_COMPILER_ID MATCHES "MSVC")
	add_compile_options("-wd4267")
	add_compile_options("-wd4244")
	add_compile_options("-wd4819")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GNU only begin
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-class-memaccess")
    # GNU only end

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-implicit-fallthrough")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-shadow")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-suggest-override")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-suggest-destructor-override")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-zero-as-null-pointer-constant")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-#pragma-messages")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-implicit-int-conversion")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-utf8")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-implicit-fallthrough")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-shadow")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-suggest-override")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-suggest-destructor-override")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-zero-as-null-pointer-constant")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-#pragma-messages")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-implicit-int-conversion")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-utf8")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
endif()
"""


def __compare_cmake_versions(version1, version2):
    def normalize(v):
        return [int(x) for x in v.split('.')]
    v1 = normalize(version1)
    v2 = normalize(version2)
    for i in range(max(len(v1), len(v2))):
        n1 = v1[i] if i < len(v1) else 0
        n2 = v2[i] if i < len(v2) else 0
        if n1 < n2:
            return -1
        elif n1 > n2:
            return 1
    return 0


def ensure_cmake_minimum_required(cmake_file, min_version = "3.5", default_version="3.10"):
    try:
        # 读取文件内容
        with open(cmake_file, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        # 检查是否已存在cmake_minimum_required命令
        cmake_min_req_index = -1
        pattern = re.compile(r'cmake_minimum_required\s*\(\s*VERSION\s+["\']?(\d+\.\d+(.\d+)*)["\']?\s*\)', flags=re.IGNORECASE)
        for index,line in enumerate(lines):
            match = pattern.search(line)
            if match:
                if cmake_min_req_index == -1:
                    cmake_min_req_index = index
                current_version = match.group(1)
                if __compare_cmake_versions(current_version, min_version) <= 0:
                    lines[index] = pattern.sub(f'cmake_minimum_required(VERSION "{default_version}")', line)

        if cmake_min_req_index == -1:
            new_line = f"cmake_minimum_required(VERSION {default_version})\n"
            lines.insert(0, new_line)
            cmake_min_req_index = 1

        lines.insert(cmake_min_req_index, __ignore_cmake_params)
        with open(cmake_file, 'w', encoding='utf-8') as f:
            f.writelines(lines)
        return True
    except Exception as e:
        print(f"error processing {cmake_file}: {e}", flush=True)
        return False


def fix_cmake(cmakefile):
    ensure_cmake_minimum_required(cmakefile)
    try:
        find_target = r"^PROJECT\(.+\)"
        with open(cmakefile, "r") as sources:
            lines = sources.readlines()
        with open(cmakefile, "w") as sources:
            for line in lines:
                sources.write(line)
                if re.search(find_target, line, re.IGNORECASE):
                    sources.write(__fix_compiler_flags)
    except Exception as e:
        return False
    return True


def remove_lines(file_path, start_line, num_lines):
    end_line = start_line + num_lines
    try:
        temp_file = tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', delete=False)
        temp_path = temp_file.name

        with open(file_path, 'r', encoding='utf-8') as source_file, \
             open(temp_path, 'w', encoding='utf-8') as temp:

            for current_line_num, line in enumerate(source_file, 1):
                # 如果当前行在删除范围内，则跳过
                if start_line <= current_line_num < end_line:
                    continue
                temp.write(line)

        os.replace(temp_path, file_path)
        return True
    except FileNotFoundError:
        print(f"错误: 文件 '{file_path}' 不存在")
        return False
    except Exception as e:
        print(f"操作失败: {str(e)}")
        # 清理临时文件
        if os.path.exists(temp_path):
            os.remove(temp_path)
        return False

if __name__ == "__main__":
    cr = __compare_cmake_versions("3.0", "3.5")
    print(cr)

