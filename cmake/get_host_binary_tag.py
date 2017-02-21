#!/usr/bin/env python
'''
Inspect the system to return the BINARY_TAG string.

Note that the 4th element will be always "-opt".

Inspired by
* https://github.com/HEP-SF/documents/tree/master/HSF-TN/draft-2015-NAM
* https://github.com/HEP-SF/tools
'''

import os
import re
import platform
from subprocess import check_output, STDOUT

def _Linux_os():
    dist = platform.linux_distribution(full_distribution_name=False)
    dist_name = dist[0].lower()
    dist_version = dist[1]
    if dist_name in ('redhat', 'centos'):
        if 'CERN' in open('/etc/%s-release' % dist_name).read():
            dist_name = 'slc'
        dist_version = dist_version.split('.', 1)[0]
    elif dist_name == 'ubuntu':
        dist_version = dist_version.replace('.', '')
    return dist_name + dist_version

def _Darwin_os():
    version = platform.mac_ver()[0].split('.')
    return 'macos' + ''.join(version[:2])

def _Windows_os():
    return 'win' + ''.join(platform.win32_ver()[1][:2])

def _unknown_os():
    return 'unknown'

os_id = globals().get('_%s_os' % platform.system(), _unknown_os)

def _compiler_version(cmd=os.environ.get('CC', 'cc')):
    m = re.search(r'(gcc|clang|icc|LLVM) version (\d+)\.(\d+)',
                  check_output([cmd, '-v'], stderr=STDOUT))
    comp = 'clang' if m.group(1) == 'LLVM' else m.group(1)
    vers = m.group(2)
    if (comp == 'gcc' and int(vers) < 7) or comp == 'clang':
        vers += m.group(3)
    return comp + vers

def compiler_id():
    return _compiler_version()

print('-'.join([platform.machine(), os_id(), compiler_id(), 'opt']))
