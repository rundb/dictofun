import os
import platform
import sys

# First argument passed into this script is the binary path, retrieved from CMake

git_show_output = os.popen("git show -s --format=\"%h %ci\" HEAD").read()[:-1]
git_branch_output = os.popen("git rev-parse --abbrev-ref HEAD").read()[:-1]
hostname = platform.node()

build_info_data = "\"Dictofun FW. " + git_show_output + ". " + "branch: " + git_branch_output + ". Built at: " + hostname + "\""

current_build_path = sys.argv[1]

with open(current_build_path + "/version.cpp", "w+") as f:
    f.write("#include \"version.h\"\n")
    f.write("namespace version\n{\n")
    f.write("const char BUILD_SUMMARY_STRING[] = \n    " + build_info_data + ";\n")
    f.write("const int BUILD_SUMMARY_STRING_LENGTH = " + str(len(build_info_data)) + ";\n")
    f.write("}\n")
