import os
import platform

git_show_output = os.popen("git show -s --format=\"%h %ci\" HEAD").read()[:-1]
git_branch_output = os.popen("git rev-parse --abbrev-ref HEAD").read()[:-1]
hostname = platform.node()

print(git_show_output)

print(git_branch_output)

print(hostname)