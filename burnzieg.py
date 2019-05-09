#! /usr/bin/python3 -B
#
# Copyright (c) 2018-2019 Gavin D. Howard and contributors.
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

import os
import sys
import subprocess
import time

from mathconfig import *

def usage():
	print("usage: {} [test_num exe]".format(script))
	print("\n    test_num is the last Burnikel/Ziegler number to run through tests")
	sys.exit(1)

if __name__ != "__main__":
	usage()

name = "Burnikel/Ziegler"
option = "z"

print_warning(name)
parse_args()

mx = 520
mn = 2

num = "9" * mx * mx
num2 = "123456789" * mx

set_limits(mx, mn)

indata = "for (i = 0; i < 100; ++i) {} / i\n {} / {}\n1.23456789^10000 / 123456789^10000\nhalt".format(num, num, num2)

optimal = run(name, indata, [ "divide", "modulus", "power", "sqrt" ], option)

print_result(name, option, optimal)
