import os
import argparse
import logging
from docker_runner import DockerRunner
import yaml

default_config = "/etc/slurm/spank-docker.yml"
logging.basicConfig()
log = logging.getLogger("docker-runnner")


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="""
        Wrapper to run user executables in docker containers
        """
    )

    parser.add_argument(
        "--docker", type=str,
        help="Name of the container",
        required=True,
    )

    parser.add_argument(
        "--cmd", type=str,
        help="Command to run",
        required=True,
    )

    parser.add_argument(
        "--user", type=str, metavar="UID:GID",
        help="User who is running command",
        required=True,
    )

    parser.add_argument(
        "--opts", type=str,
        help="Additional arguments for docker",
    )

    parser.add_argument(
        "--config", type=str,
        default=default_config,
        help="Config file",
    )

    parser.add_argument(
        "--envfile", type=str,
        help="File with environmntal variables",
    )

    parser.add_argument(
        "--debug", action="store_const",
        dest="loglevel", const=logging.DEBUG, default=logging.INFO,
        help="Debug output"
    )

    args = parser.parse_args()
    return args


def parse_config(conf_file=default_config):
    yaml_config = {}

    if not os.path.isfile(conf_file):
        log.debug("No config file {}".format(conf_file))
        return yaml_config

    log.debug("Found config file {}".format(conf_file))

    with open(conf_file) as f:
        try:
            yaml_config = yaml.load(f)
        except yaml.scanner.ScannerError, yaml.YAMLError:
            log.error("Error parsing config file")
            return {}

    log.debug("Configuration params: {}".format(yaml_config))

    return yaml_config


def run():
    args = parse_arguments()
    log.setLevel(args.loglevel)
    log.debug("Arguments: {}".format(args))
    CONFIG = parse_config(args.config)
    dr = DockerRunner(args=args, CONFIG=CONFIG)
    return dr.Run()
