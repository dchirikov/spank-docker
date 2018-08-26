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

#include "spank-docker.h"

std::string opt_docker_name = "";
std::string opt_docker_opts = "";
std::string opt_docker_runner = "";

// callback function to handle cmdline for srun and #SBATCH tags
extern "C" int opt_cb_docker(int val, const char *optarg, int remote) {
  // Fill docker name
  if (val == OPT_DOCKER_NAME) {
    opt_docker_name = optarg;
  }
  // Fill additional options
  if (val == OPT_DOCKER_OPTS) {
    opt_docker_opts = optarg;
  }
  return 0;
}

extern "C" int slurm_spank_init(spank_t spank, int ac, char *argv[]) {
  // Make --docker and --docker-options avaliable in *.job files too
  auto ptr = spank_docker_cmd_options;
  // How many options we need to register
  auto num_opts = sizeof(spank_docker_cmd_options)/sizeof(spank_option);
  // Register options in a loop
  for (int i=0; i<num_opts; i++) {
      auto err = spank_option_register(spank, ptr);
      if (err != ESPANK_SUCCESS) {
          slurm_error("Unable to register spank docker options");
          return 1;
      }
      ptr++;
  }
  // Extra option from plugstack.conf is our runner
  // Put it to opt_docker_additional_opts
  for (int i=0; i<ac; i++) {
    opt_docker_runner += argv[i];
    opt_docker_runner += " ";
  }
  return 0;
}

extern "C" int slurm_spank_task_init_privileged(spank_t spank, int ac, char *argv[]) {
  // No --docker was specified
  // Nothing to do
  if (opt_docker_name == "") {
    return 0;
  }

  // Figuring out what user is about to run and put it into the container
  auto dr = DockerRunner(
      spank, opt_docker_runner, opt_docker_name, opt_docker_opts);

  // Main call
  auto res = dr.Run();

  // check exit code and STDOUT
  auto rc = res.rc;
  auto out_err = res.data;

  if (out_err != "") {
    slurm_error(out_err.c_str());
  }

  // Force exit. We are not going to allow SLURM proceed with user unput.
  // Job was running already
  exit(rc);

  return 0;
}

// Constructor
DockerRunner::DockerRunner(
  spank_t sp, std::string runner, std::string name, std::string opts)
  : _sp(sp), _name(name), _opts(opts)  {}

// Returns command user whants to run
RetStr DockerRunner::_GetCmd() {
  std::string cmd = "";
  int jac;
  char** jav;
  auto err = spank_get_item(_sp, S_JOB_ARGV, &jac, &jav);
  if (err != ESPANK_SUCCESS) {
    slurm_error("Unable to get S_JOB_ARGV");
    return {-1, ""};
  }
  for (int i=0; i<jac; i++) {
    cmd += jav[i];
    if (i != jac-1) {
      cmd += "\\0";
    }
  }
  return {0, cmd};
}

// Returns UID
RetInt DockerRunner::_GetUID() {
  int uid;
  auto err = spank_get_item(_sp, S_JOB_UID, &uid);
  if (err != ESPANK_SUCCESS) {
    slurm_error("Unable to get S_JOB_UID");
    return {-1, -1};
  }
  return {0, uid};
}

// Returns GID
RetInt DockerRunner::_GetGID() {
  int gid;
  auto err = spank_get_item(_sp, S_JOB_GID, &gid);
  if (err != ESPANK_SUCCESS) {
    slurm_error("Unable to get S_JOB_GID");
    return {-1, -1};
  }
  return {0, gid};
}

// Main runner
RetStr DockerRunner::Run() {
  std::string cmd;
  int rc, uid, gid;
  // get UID of the user
  {
    auto res = _GetUID();
    rc = res.rc;
    uid = res.data;
  }
  if (rc != 0) {
    return {-1, ""};
  }
  // get GID of the user
  {
    auto res = _GetGID();
    rc = res.rc;
    gid = res.data;
  }
  if (rc != 0) {
    return {-1, ""};
  }
  // get command user is trying to run
  {
    auto res = _GetCmd();
    rc = res.rc;
    cmd = res.data;
  }
  if (rc != 0) {
    return {-1, ""};
  }
  //slurm_error("%s|%d|%d", cmd.c_str(), uid, gid);
  // TODO
  // Now run docker wrapper
  return {0, ""};
}
