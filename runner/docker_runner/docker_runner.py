import logging
import socket
import subprocess as sp
from jinja2 import Environment
import docker
from threading import Timer
import sys


default_host_timeout = 30
logging.basicConfig()

def run_cmd(cmd, timeout=120):
    """
    Returns 'rc', 'stdout', 'stderr', 'exception'
    Where 'exception' is a content of Python exception if any
    """
    rc = 255
    stdout, stderr, exception = "", "", ""
    try:
        proc = sp.Popen(
            cmd, shell=True,
            stdout=sp.PIPE, stderr=sp.PIPE
        )
        timer = Timer(
            timeout, lambda p: p.kill(), [proc]
        )
        try:
            timer.start()
            stdout, stderr = proc.communicate()
        except:
            log.debug("Timeout executing '{}'".format(cmd))
        finally:
            timer.cancel()

        proc.wait()
        rc = proc.returncode
    except Exception as e:
        exception = e
    return rc, stdout, stderr, exception


class DockerRunner(object):

    def __init__(self, args, CONFIG):
        module_name = self.__module__ + "." + type(self).__name__
        self.log = logging.getLogger(module_name)
        self.log.setLevel(args.loglevel)
        self.args = args
        self.CONFIG = CONFIG
        self.options = {
            'docker': '',
            'scripts': '',
        }
        self.environ = self.read_envfile(args.envfile)

    def read_envfile(self, envfile):
        with open(envfile, 'ro') as f:
            env = [e.strip() for e in f.readlines()]
        return env

    def alter_options(self):
        # first get global parameters
        conf = self.CONFIG['container_options']
        if '*' in conf:
            for k, v in conf['*'].items():
                self.options[k] = v
        # override if some tunable specified for particular container
        if self.args.docker in conf.items():
            for k, v in conf[self.args.docker].items():
                self.options[k] = v
        # can we user override docker options
        if ('allow_user_override' in self.options
                and self.options['allow_user_override']):
            self.options['docker'] = self.args.opts

    def Run(self):
        if ("alowed_containers" in self.CONFIG
                and self.args.docker not in self.CONFIG["alowed_containers"]):

            msg = (
                "Docker {} is not allowed to run by admin"
            ).format(self.args.docker)

            self.log.error(msg)
            return False

        self.alter_options()
        msg = (
            "Docker {} will be running with opts: {}"
        ).format(self.args.docker, self.options)
        self.log.debug(msg)

        stdout_lines = []
        uid, gid = self.args.user.split(':')
        tmp_cmd = self.args.cmd.split('\\0')
        cmd = []
        for e in tmp_cmd:
            if e.find(" ") > 0:
                cmd.append('"' + e + '"')
            else:
                cmd.append(e)

        j2_vars = {
            "UID": uid,
            "GID": gid,
            "ENVFILE": self.args.envfile,
            "IMAGE": self.args.docker,
            "CMD": " ".join(cmd),
        }

        for elem in self.options['scripts']:

            if ('type' not in elem
                    or elem['type'] not in ['container', 'host']):
                msg = "Unknow scripts type"
                self.log.error(msg)
                return False

            if 'script' not in elem:
                msg = "No script specified"
                self.log.error(msg)
                return False

            if 'timeout' not in elem:
                elem['timeout'] = default_host_timeout

            j2_vars["stdout_lines"] = stdout_lines
            script_body = (
                Environment()
                .from_string(elem['script'])
                .render(**j2_vars)
            )

            msg = (
                "Script type: {}; body: {}"
            ).format(
                elem['type'],
                script_body.replace("\n", "\\n")
            )
            self.log.debug(msg)

            # run host script
            if elem['type'] == 'host':
                rc, stdout, stderr, exception = run_cmd(
                    script_body,
                    elem['timeout']
                )
                if rc:
                    self.log.debug("EXCEPTION: {}".format(exception))
                    self.log.error("STDERR: {}".format(stderr))
                    self.log.error("STDOUT: {}".format(stdout))
                    return False

                stdout_lines = stdout.split('\n')

                continue

            # start docker container
            client = docker.from_env()

            docker_opts = self.options['docker'].copy()
            docker_opts['image'] = self.args.docker
            docker_opts['command'] = (
                "bash -c \"" + script_body.replace('"', '\\"') + "\""
            )
            #docker_opts['auto_remove'] = True
            docker_opts['remove'] = True
            docker_opts['tty'] = True
            docker_opts['stderr'] = True
            docker_opts['environment'] = self.environ

            try:
                d = client.containers.run(**docker_opts)
            except docker.errors.ContainerError:
                msg = "Host: {}; Error occured for: {}".format(
                    socket.gethostname(),
                    j2_vars['CMD']
                )
                self.log.error(msg)
            else:
                sys.stdout.write(d)

        return True
