# encoding: utf-8
# enforce_cxx11.py -- enforcing cxx11 if requested
# Copyright (C) 2018 a1batross
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

from waflib.Configure import conf

@conf
def check_cxx11(conf, enable):
	if enable:
		if conf.env.COMPILER_CC != 'msvc':
			conf.env.append_unique('CXXFLAGS', '-std=c++11')
