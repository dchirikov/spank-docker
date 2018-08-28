/* Written by Dmitry Chirikov <dmitry@chirikov.ru>
 * This file is part of spank-docker,
 * SLURM module for running docker containers
 * https://github.com/dchirikov/spank-docker
 * Spank-docker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Spank-docker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with spank-docker.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <cstddef>
#include <unistd.h>
#include <limits.h>
extern "C" {
  #include <slurm/spank.h>
}
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#define OPT_DOCKER_NAME 0
#define OPT_DOCKER_OPTS 1

extern "C" const char plugin_name[]= "spank-docker";
extern "C" const char plugin_type[] = "spank";
extern "C" const unsigned int plugin_version = 1;

extern "C" int opt_cb_docker(int val, const char *optarg, int remote);

struct spank_option spank_docker_cmd_options[] =
{
  { (char *)"docker", (char *)("name"),
    (char *)"Name of the container", 1, OPT_DOCKER_NAME,
    (spank_opt_cb_f) opt_cb_docker
  },
  { (char *)"docker-opts", (char *)"options",
    (char *)"Options to run for the container", 1, OPT_DOCKER_OPTS,
    (spank_opt_cb_f) opt_cb_docker
  },
};

struct RetInt {
  int rc;
  int data;
};

struct RetStr {
  int rc;
  std::string data;
};

class DockerRunner {
public:
  DockerRunner(
    spank_t sp, std::string runner, std::string name, std::string opts);
  RetStr Run();
private:
  spank_t _sp;
  std::string _runner, _name, _opts;
  std::vector<std::string> _environ;
  RetStr _GetCmd();
  RetInt _GetUID();
  RetInt _GetGID();
  int _GetEnv();
};

RetStr spank_exec(const char* cmd);
