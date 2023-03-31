#! /usr/bin/env python
# encoding: utf-8
# a1batross, mittorn, 2018

from __future__ import print_function
from waflib import Logs, Context, Configure
import sys
import os

VERSION = '0.1'
APPNAME = 'CSGO'
top = '.'

Context.Context.line_just = 55 # should fit for everything on 80x26

class Subproject:
	def __init__(self, name, fnFilter = None):
		self.name = name
		self.fnFilter = fnFilter

	def is_exists(self, ctx):
		return ctx.path.find_node(self.name + '/wscript')

	def is_enabled(self, ctx):
		if not self.is_exists(ctx):
			return False

		if self.fnFilter:
			return self.fnFilter(ctx)

		return True

SUBDIRS = [
	Subproject('tier0')
]

def options(opt):
	grp = opt.add_option_group('Common options')

	grp.add_option('-d', '--dedicated', action = 'store_true', dest = 'DEDICATED', default = False,
		help = 'build dedicated server [default: %default]')

	grp.add_option('-8', '--64bits', action = 'store_true', dest = 'ALLOW64', default = False,
		help = 'allow targetting 64-bit engine [default: %default]')

	grp = opt.add_option_group('Renderers options')

	grp.add_option('--use-sdl', action = 'store', dest = 'SDL', default = sys.platform in ['linux','android'],
		help = 'build with SDL [default: %default]')

	grp.add_option('--enable-togl', action='store_true', dest='TOGL', default = sys.platform != 'win32',
		help = 'Direct3D -> OpenGL translation layer [default: %default]')

	opt.load('compiler_optimizations subproject')

	for i in SUBDIRS:
		if not i.is_exists(opt):
			continue

		opt.add_subproject(i.name)

	opt.load('xcompile compiler_cxx compiler_c clang_compilation_database strip_on_install waf_unit_test msdev msvs')
	if sys.platform == 'win32':
		opt.load('msvc')
	opt.load('reconfigure')

def configure(conf):
	conf.load('fwgslib reconfigure compiler_optimizations')
	conf.env.MSVC_TARGETS = ['x86' if not conf.options.ALLOW64 else 'x64']

	# Load compilers early
	conf.load('xcompile compiler_c compiler_cxx')

	# HACKHACK: override msvc DEST_CPU value by something that we understand
	if conf.env.DEST_CPU == 'amd64':
		conf.env.DEST_CPU = 'x86_64'

	if conf.env.COMPILER_CC == 'msvc':
		conf.load('msvc_pdb')

	conf.load('msvs msdev subproject gitversion clang_compilation_database strip_on_install waf_unit_test enforce_pic')

	# Force XP compatibility, all build targets should add subsystem=bld.env.MSVC_SUBSYSTEM
	if conf.env.MSVC_TARGETS[0] == 'x86':
		conf.env.MSVC_SUBSYSTEM = 'WINDOWS,5.01'
		conf.env.CONSOLE_SUBSYSTEM = 'CONSOLE,5.01'
	else:
		conf.env.MSVC_SUBSYSTEM = 'WINDOWS'
		conf.env.CONSOLE_SUBSYSTEM = 'CONSOLE'

	enforce_pic = True # modern defaults

	conf.check_pic(enforce_pic)

	# We restrict 64-bit builds ONLY for Win/Linux/OSX running on Intel architecture
	# Because compatibility with original GoldSrc
	if conf.env.DEST_OS in ['win32', 'linux', 'darwin'] and conf.env.DEST_CPU == 'x86_64':
		conf.env.BIT32_MANDATORY = not conf.options.ALLOW64
		if conf.env.BIT32_MANDATORY:
			Logs.info('WARNING: will build engine for 32-bit target')
	else:
		conf.env.BIT32_MANDATORY = False

	conf.load('force_32bit')

	compiler_optional_flags = [
		'-fdiagnostics-color=always'
	]

	cxx_compiler_optional_flags = [
		'-Wno-narrowing'
	]

	c_compiler_optional_flags = [
		'-fnonconst-initializers' # owcc
	]

	includes = [
		os.path.abspath('public'),
		os.path.abspath('public/tier0'),
		os.path.abspath('public/tier1'),
		os.path.abspath('thirdparty/SDL2/include'),
		os.path.abspath('common')
	]

	cflags, linkflags = conf.get_optimization_flags()

	# And here C++ flags starts to be treated separately
	cxxflags = list(cflags)
	if conf.env.COMPILER_CC != 'msvc':
		conf.check_cc(cflags=cflags, linkflags=linkflags, msg='Checking for required C flags')
		conf.check_cxx(cxxflags=cflags, linkflags=linkflags, msg='Checking for required C++ flags')

		conf.env.append_unique('CFLAGS', cflags)
		conf.env.append_unique('CXXFLAGS', cxxflags)
		conf.env.append_unique('LINKFLAGS', linkflags)

		cxxflags += conf.filter_cxxflags(compiler_optional_flags + cxx_compiler_optional_flags, cflags)
		cflags += conf.filter_cflags(compiler_optional_flags + c_compiler_optional_flags, cflags)

	conf.env.append_unique('CFLAGS', cflags)
	conf.env.append_unique('CXXFLAGS', cxxflags)
	conf.env.append_unique('LINKFLAGS', linkflags)
	conf.env.append_unique('INCLUDES', includes)

	if conf.env.DEST_OS != 'win32':
		conf.check_cc(lib='dl', mandatory=False)

		if not conf.env.LIB_M: # HACK: already added in xcompile!
			conf.check_cc(lib='m')
	else:
		# Common Win32 libraries
		# Don't check them more than once, to save time
		# Usually, they are always available
		# but we need them in uselib
		a = [
			'user32',
			'shell32',
			'gdi32',
			'advapi32',
			'dbghelp',
			'psapi',
			'ws2_32'
		]

		if conf.env.COMPILER_CC == 'msvc':
			for i in a:
				conf.start_msg('Checking for MSVC library')
				conf.check_lib_msvc(i)
				conf.end_msg(i)
		else:
			for i in a:
				conf.check_cc(lib = i)

	# CSGO specific defines
	conf.env.append_unique('DEFINES', [
		'CSTRIKE_REL_BUILD',
		'RAD_TELEMETRY_DISABLED', # removed for partner depot
		'CSTRIKE15'
	])

	conf.define_cond('COMPILER_GCC', conf.env.COMPILER_CC == 'gcc')

	if conf.options.DEDICATED:
		conf.options.SDL = False
		conf.define('DEDICATED', 1)

	if conf.options.TOGL:
		conf.env.append_unique('DEFINES', [
			'DX_TO_GL_ABSTRACTION',
			'GL_GLEXT_PROTOTYPES'
		])

	if conf.options.SDL:
		conf.env.SDL = True
		conf.define('USE_SDL', 1)

	if conf.options.ALLOW64:
		conf.define('PLATFORM_64BITS', 1)

	#TODO: other platforms support
	if conf.env.DEST_OS != 'win32': # POSIX
		conf.env.append_unique('DEFINES', [
			'POSIX=1', '_POSIX=1',
			'GNUC'
		])

	if conf.env.DEST_OS == 'linux':
		conf.define('_GLIBCXX_USE_CXX11_ABI',0) # TODO:remove this till we remove protobuf
		conf.env.append_unique('DEFINES', [
			'LINUX=1', '_LINUX=1',
			'_DLL_EXT=.so'
		])

	if conf.env.DEST_OS == 'darwin':
		conf.env.append_unique('DEFINES', [
			'OSX=1', '_OSX=1',
			'_DLL_EXT=.dylib'
		])

	if conf.options.BUILD_TYPE == 'debug':
		conf.env.append_unique('DEFINES', [
			'DEBUG', '_DEBUG'
		])
	else:
		conf.env.append_unique('DEFINES', [
			'NDEBUG'
		])

	# set _FILE_OFFSET_BITS=64 for filesystems with 64-bit inodes
	if conf.env.DEST_OS != 'win32' and conf.env.DEST_SIZEOF_VOID_P == 4:
		# check was borrowed from libarchive source code
		file_offset_bits_usable = conf.check_cc(fragment='''
#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#define KB ((off_t)1024)
#define MB ((off_t)1024 * KB)
#define GB ((off_t)1024 * MB)
#define TB ((off_t)1024 * GB)
int t2[(((64 * GB -1) % 671088649) == 268434537)
       && (((TB - (64 * GB -1) + 255) % 1792151290) == 305159546)? 1: -1];
int main(void) { return 0; }''',
		msg='Checking if _FILE_OFFSET_BITS can be defined to 64', mandatory=False)
		if file_offset_bits_usable:
			conf.define('_FILE_OFFSET_BITS', 64)
		else: conf.undefine('_FILE_OFFSET_BITS')

	conf.env.LIBDIR = conf.env.PREFIX + '/bin'
	conf.env.BINDIR = conf.env.PREFIX

	for i in SUBDIRS:
		if not i.is_enabled(conf):
			continue

		conf.add_subproject(i.name)

def build(bld):
	for i in SUBDIRS:
		if not i.is_enabled(bld):
			continue

		bld.add_subproject(i.name)
