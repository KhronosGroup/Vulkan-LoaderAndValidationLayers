#!/usr/bin/env python
#
# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import argparse
import multiprocessing
import os
import subprocess
import sys

from subprocess import PIPE, STDOUT

THIS_DIR = os.path.realpath(os.path.dirname(__file__))

ALL_ARCHITECTURES = (
  'arm',
  'arm64',
  'mips',
  'mips64',
  'x86',
  'x86_64',
)

# According to vk_platform.h, armeabi is not supported for Vulkan
# so remove it from the abis list.
ALL_ABIS = (
  'armeabi-v7a',
  'arm64-v8a',
  'mips',
  'mips64',
  'x86',
  'x86_64',
)

def jobs_arg():
  return '-j{}'.format(multiprocessing.cpu_count() * 2)

def arch_to_abis(arch):
  return {
    'arm': ['armeabi-v7a'],
    'arm64': ['arm64-v8a'],
    'mips': ['mips'],
    'mips64': ['mips64'],
    'x86': ['x86'],
    'x86_64': ['x86_64'],
  }[arch]

class ArgParser(argparse.ArgumentParser):
  def __init__(self):
    super(ArgParser, self).__init__()

    self.add_argument(
      '--out-dir', help='Directory to place temporary build files.',
      type=os.path.realpath, default=os.path.join(THIS_DIR, 'out'))

    self.add_argument(
      '--arch', choices=ALL_ARCHITECTURES,
      help='Architectures to build. Builds all if not present.')

    self.add_argument('--installdir', dest='installdir', required=True,
      help='Installation directory. Required.')

    # The default for --dist-dir has to be handled after parsing all
    # arguments because the default is derived from --out-dir. This is
    # handled in run().
    self.add_argument(
      '--dist-dir', help='Directory to place the packaged artifact.',
      type=os.path.realpath)


def main():
  print('Constructing Vulkan validation layer source...')

  print('THIS_DIR: %s' % THIS_DIR)
  parser = ArgParser()
  args = parser.parse_args()

  arches = ALL_ARCHITECTURES
  if args.arch is not None:
    arches = [args.arch]

  # ensure directory exists.
  if not os.path.isdir(args.installdir):
    os.makedirs(args.installdir)

  # Make paths absolute, and ensure directories exist.
  installdir = os.path.abspath(args.installdir)

  abis = []
  for arch in arches:
    abis.extend(arch_to_abis(arch))

  shaderc_dir = THIS_DIR + '/../../shaderc/shaderc/android_test'
  print('shaderc_dir = %s' % shaderc_dir)

  if os.path.isdir('/buildbot/android-ndk'):
    ndk_dir = '/buildbot/android-ndk'
  elif os.path.isdir(os.environ['NDK_PATH']):
    ndk_dir = os.environ['NDK_PATH'];
  else:
    print('Error: No NDK environment found')
    return

  ndk_build = os.path.join(ndk_dir, 'ndk-build')
  platforms_root = os.path.join(ndk_dir, 'platforms')
  toolchains_root = os.path.join(ndk_dir, 'toolchains')
  build_dir = THIS_DIR

  print('installdir: %s' % installdir)
  print('ndk_dir: %s' % ndk_dir)
  print('ndk_build: %s' % ndk_build)
  print('platforms_root: %s' % platforms_root)

  compiler = 'clang'
  stl = 'gnustl_static'
  obj_out = os.path.join(THIS_DIR, stl, 'obj')
  lib_out = os.path.join(THIS_DIR, 'jniLibs')

  print('obj_out: %s' % obj_out)
  print('lib_out: %s' % lib_out)

  print('Building shader toolchain...')
  build_cmd = [
    'bash', ndk_build, '-C', shaderc_dir, jobs_arg(),
    'APP_ABI=' + ' '.join(abis),
    # Use the prebuilt platforms and toolchains.
    'NDK_PLATFORMS_ROOT=' + platforms_root,
    'NDK_TOOLCHAINS_ROOT=' + toolchains_root,
    'GNUSTL_PREFIX=',

    # Tell ndk-build where all of our makefiles are and where outputs
    # should go. The defaults in ndk-build are only valid if we have a
    # typical ndk-build layout with a jni/{Android,Application}.mk.
    'NDK_PROJECT_PATH=null',
    'NDK_TOOLCHAIN_VERSION=' + compiler,
    'APP_BUILD_SCRIPT=' + os.path.join(shaderc_dir, 'jni', 'Android.mk'),
    'APP_STL=' + stl,
    'NDK_APPLICATION_MK=' + os.path.join(shaderc_dir, 'jni', 'Application.mk'),
    'NDK_OUT=' + os.path.join(shaderc_dir, 'obj'),
    'NDK_LIBS_OUT=' + os.path.join(shaderc_dir, 'jniLibs'),
    'THIRD_PARTY_PATH=../..',

    # Put armeabi-v7a-hard in its own directory.
    '_NDK_TESTING_ALL_=yes'
  ]

  subprocess.check_call(build_cmd)
  print('Finished shader toolchain build')

  build_cmd = [
    'bash', THIS_DIR + '/android-generate.sh'
  ]
  print('Generating generated layers...')
  subprocess.check_call(build_cmd)
  print('Generation finished')


  build_cmd = [
    'bash', ndk_build, '-C', build_dir, jobs_arg(),
    'APP_ABI=' + ' '.join(abis),
    # Use the prebuilt platforms and toolchains.
    'NDK_PLATFORMS_ROOT=' + platforms_root,
    'NDK_TOOLCHAINS_ROOT=' + toolchains_root,
    'GNUSTL_PREFIX=',

    # Tell ndk-build where all of our makefiles are and where outputs
    # should go. The defaults in ndk-build are only valid if we have a
    # typical ndk-build layout with a jni/{Android,Application}.mk.
    'NDK_PROJECT_PATH=null',
    'NDK_TOOLCHAIN_VERSION=' + compiler,
    'APP_BUILD_SCRIPT=' + os.path.join(build_dir, 'jni', 'Android.mk'),
    'APP_STL=' + stl,
    'NDK_APPLICATION_MK=' + os.path.join(build_dir, 'jni', 'Application.mk'),
    'NDK_OUT=' + obj_out,
    'NDK_LIBS_OUT=' + lib_out,
    'THIRD_PARTY_PATH=',

    # Put armeabi-v7a-hard in its own directory.
    '_NDK_TESTING_ALL_=yes'
  ]

  print('Building Vulkan validation layers for ABIs:' +
    ' {}'.format(', '.join(abis)))
  subprocess.check_call(build_cmd)

  print('Finished building Vulkan validation layers')
  out_package = os.path.join(installdir, 'vulkan_validation_layers.zip')
  os.chdir(lib_out)
  build_cmd = [
      'zip', '-9qr', out_package, "."
  ]

  print('Packaging Vulkan validation layers')
  subprocess.check_call(build_cmd)
  print('Finished Packaging Vulkan validation layers')


if __name__ == '__main__':
  main()
