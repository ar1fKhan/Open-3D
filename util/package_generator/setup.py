# Copyright (c) 2018, Open3D community
# Redistribution and use in full or partial is allowed according to the terms of the MIT license.

import os
import re
import sys
import glob
import shutil
import fnmatch
import pypandoc
from setuptools import setup, Distribution

libs_extensions = ['dll', 'dylib', 'pyd', 'so', ]

class BinaryDistribution(Distribution):
    def has_ext_modules(foo):
        return True

def read(fname, ext=None):
    if ext:
        try:
            return pypandoc.convert(fname, ext)
        except OSError:
            pypandoc.download_pandoc()
            return pypandoc.convert(fname, ext)
    return open(fname, 'r').read()

def get_version():
    data = read('../../CMakeLists.txt')
    version_info = dict(re.findall('OPEN3D_VERSION_([^ \t]+)[ \t]+"([0-9]+)"', data))
    version = '%s.%s.%s' % (version_info['MAJOR'], version_info['MINOR'], version_info['PATCH'])
    return version

def prepare_package():
    try:
        idx = sys.argv.index('--base-dir')
    except ValueError:
        exit('You must specifiy the base directory using --base-dir argument')
    BASE_DIR, sys.argv = sys.argv[idx + 1], sys.argv[:idx] + sys.argv[idx+2:]

    shutil.copy("../../LICENSE", 'open3d/LICENSE.txt')
    with open("open3d/README.rst", "w") as fp:
        fp.write(long_description)

    #Copy found files to the package path
    package_data = []
    for root, dirnames, filenames in os.walk(BASE_DIR):
        for libs_extension in libs_extensions:
            filterednames = fnmatch.filter(filenames, '*open3d*.' + libs_extension)
            package_data += filterednames
            for filename in filterednames:
                shutil.copy(root + '/' + filename, 'open3d/')
    assert(len(package_data) > 0), 'No package data found!'

def clean_package():
    for libs_extension in libs_extensions:
        for file in glob.glob('open3d/*open3d*.' + libs_extension):
            print("removing '%s'" % file)
            os.remove(file)
    if os.path.isdir('build'):
        print("removing '%s' folder" % 'build')
        shutil.rmtree('build')
    if os.path.isdir('open3d.egg-info'):
        print("removing '%s' folder" % 'open3d.egg-info')
        shutil.rmtree('open3d.egg-info')
    if os.path.exists('open3d/LICENSE.txt'):
        print("removing '%s'" % 'open3d/LICENSE.txt')
        os.remove('open3d/LICENSE.txt')
    if os.path.exists('open3d/README.rst'):
        print("removing '%s'" % 'open3d/README.rst')
        os.remove('open3d/README.rst')

long_description = read('../../README.md', 'rst').strip().replace("\r\n", "\n")
try:
    sys.argv.index('bdist_wheel')
    prepare_package()
except ValueError:
    pass

setup(
    name = "open3d",
    version = get_version(),
    author = 'Open3D community',
    author_email = 'info@open-3d.org',
    description = ("Open3D: A Modern Library for 3D Data Processing"),
    license = "MIT",
    keywords = "computer-vision 3d-reconstruction 3d-graphics python-bindings rgbd pointcloud",
    url = "http://www.open-3d.org",
    packages=['open3d', ],
    install_requires=['numpy', ],
    include_package_data=True,
    distclass=BinaryDistribution,
    #package_data=dict(open3d=package_data, ),
    long_description=long_description,
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Operating System :: Microsoft :: Windows",
        "Operating System :: MacOS",
        "Operating System :: POSIX :: Linux",
        "Programming Language :: C++",
        "Programming Language :: Python",
        "Topic :: Utilities",
        "Topic :: Scientific/Engineering",
        "Topic :: Software Development :: Libraries",
        "Topic :: Multimedia :: Graphics :: 3D Modeling",
        "Topic :: Multimedia :: Graphics :: Viewers",
        "Environment :: Win32 (MS Windows)",
        "Environment :: MacOS X",
        "Environment :: X11 Applications",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "Natural Language :: English",
        "License :: OSI Approved :: MIT License",
    ],
)

#Cleanup
try:
    sys.argv.index('clean')
    clean_package()
except ValueError:
    pass
