#!/usr/bin/python

import sys
from docker_runner import cmd

if __name__ == '__main__':
    res = cmd.run()
    sys.exit(not bool(res))

