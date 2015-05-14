#!/usr/bin/env python

import os
from subprocess import call
from distutils.core import setup
from distutils.command.build_py import build_py

# an extension to the build step that compiles and copies our shared object
# file
class cmake_lib(build_py):

    def run(self):

        module_name = "pysopnet"
        lib_name    = "pysopnet/lib" + module_name + ".so"
        source_dir  = os.path.abspath(".")
        build_dir   = os.path.join(source_dir, "build-pysopnet")
        lib_file    = os.path.join(build_dir, lib_name)

        if not self.dry_run:

            # create our shared object file
            call(["./compile_wrapper.sh", source_dir, build_dir, module_name])

            target_dir = self.build_lib

            # make sure the module dir exists
            self.mkpath(os.path.join(target_dir, module_name))

            # copy our library to the module dir
            self.copy_file(lib_file, os.path.join(target_dir, lib_name))

        # run parent implementation
        build_py.run(self)

setup(
    name='PySopnet',
    version='0.1.0',
    author='Jan Funke',
    author_email='funke@ini.ch',
    license='LICENSE',
    description='Python wrappers for the SOPNET neuron reconstruction pipeline.',
    packages=['pysopnet'],
    cmdclass={'build_py' : cmake_lib}
)
