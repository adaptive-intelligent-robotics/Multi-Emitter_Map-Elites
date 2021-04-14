#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys
import time
from typing import Union

SINGULARITY_DEFINITION_FILE_NAME = "singularity.def"


class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


def error_print(message: str):
    print(f"{bcolors.FAIL}{message}{bcolors.ENDC}", file=sys.stderr)


def bold(message: str):
    return f"{bcolors.BOLD}{message}{bcolors.ENDC}"


def load_singularity_file(path_to_singularity_definition_file: str):
    try:
        # read input file
        fin = open(path_to_singularity_definition_file, "rt")

    except IOError:
        error_print(f"ERROR, {path_to_singularity_definition_file} file not found!")
    finally:

        data = fin.read()
        # close the input file
        fin.close()
        return data


def get_repo_address():
    # Search projects
    command = os.popen("git config --local remote.origin.url")
    url = command.read()[:-1]

    # if it is using the ssh protocal,
    # we need to convert it into an https address as the key is not available inside the container
    if url.startswith("git@"):
        url = url.replace(":", "/")
        url = url.replace("git@", "")

    return url


def get_commit_sha_and_branch_name(project_commit_sha_to_consider: str):
    # Search projects
    command = os.popen(f"git rev-parse --short {project_commit_sha_to_consider}")
    sha = command.read()[:-1]
    command = os.popen(f"git rev-parse --abbrev-ref {project_commit_sha_to_consider}")
    branch = command.read()[:-1]

    return sha, branch


def check_local_changes():
    command = os.popen("git status --porcelain --untracked-files=no")
    output = command.read()[:-1]
    if output:
        error_print("WARNING: There are currently unpushed changes:")
        error_print(output)


def check_local_commit_is_pushed(project_commit_ref_to_consider: str):
    command = os.popen(f"git branch -r --contains {project_commit_ref_to_consider}")
    remote_branches_containing_commit = command.read()[:-1]

    if not remote_branches_containing_commit:
        error_print(f"WARNING: local commit {project_commit_ref_to_consider} not pushed, build is likely to fail!")


def get_project_folder_name():
    return os.path.basename(os.path.dirname(os.getcwd()))


def clone_commands(project_commit_ref_to_consider: str,
                   ci_job_token: str,
                   personal_token: str,
                   project_name: str,
                   ) -> str:
    repo_address = get_repo_address()
    sha, branch = get_commit_sha_and_branch_name(project_commit_ref_to_consider)

    if ci_job_token:  # we are in a CI environment
        repo_address = f"http://gitlab-ci-token:{ci_job_token}@{repo_address}"
    elif personal_token:  # if a personal token is available
        repo_address = f"https://oauth:{personal_token}@{repo_address}"
    else:
        repo_address = f"https://{repo_address}"

    print(f"Building final image using branch: {bold(branch)} with sha: {bold(sha)} \n URL: {bold(repo_address)}")
    code_block = f"git clone --recurse-submodules --shallow-submodules {repo_address} {project_name} " \
                 f"&& cd {project_name} " \
                 f"&& git checkout {sha} " \
                 f"&& cd .."

    return code_block


def apply_changes(original_file: str,
                  project_commit_ref_to_consider: str,
                  ci_job_token: str,
                  personal_token: str,
                  project_name: str,
                  ):
    fout = open("./tmp.def", "w")
    for line in original_file.splitlines():
        if "#NOTFORFINAL" in line:
            continue
        if "#CLONEHERE" in line:
            line = clone_commands(project_commit_ref_to_consider, ci_job_token, personal_token, project_name)
        fout.write(line + "\n")
    fout.close()


def compile_container(project_name: str,
                      image_name: Union[str, None],
                      debug: bool):
    if not image_name:
        image_name = f"final_{project_name}_{time.strftime('%Y-%m-%d_%H_%M_%S')}.sif"
    subprocess.run(['singularity', 'build', '--force', '--fakeroot', image_name, './tmp.def'])
    if not debug:
        os.remove('./tmp.def')


def get_args():
    parser = argparse.ArgumentParser(description='Build a read-only final container '
                                                 'in which the entire project repository is cloned',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('--path-def', required=False, type=str, default=SINGULARITY_DEFINITION_FILE_NAME,
                        help='path to singularity definition file.')

    parser.add_argument('--commit-ref', '-c', required=False, type=str, default="HEAD",
                        help='commit/branch/tag to consider in the project repository '
                             '(useful only when using #CLONEHERE).')

    parser.add_argument('--ci-job-token', required=False, type=str, default=get_ci_job_token(),
                        help='Gitlab CI job token (useful only when using #CLONEHERE). '
                             'If not specified, it takes the value of the environment variable CI_JOB_TOKEN, '
                             'if it exists. '
                             'If the environment variable SINGULARITYENV_CI_JOB_TOKEN is not set yet, '
                             'then it is set the value provided.')
    parser.add_argument('--personal-token', required=False, type=str, default=get_personal_token(),
                        help='Gitlab Personal token (useful only when using #CLONEHERE). '
                             'If not specified, it takes the value of the environment variable PERSONAL_TOKEN, '
                             'if it exists. '
                             'If the environment variable SINGULARITYENV_PERSONAL_TOKEN is not set yet, '
                             'then it is set the value provided.')

    parser.add_argument('--project', required=False, type=str, default=get_project_folder_name(),
                        help='Specify the name of the project. This corresponds to: '
                             '(1) Name of the folder in which the current repository will be cloned '
                             '(useful only when using #CLONEHERE); '
                             '(2) the name in the final singularity image "final_<project>_YYYY_mm_DD_HH_MM_SS.sif". '
                             'By default, it uses the name of the parent folder, as it is considered that '
                             'the script is executed in the \'singularity/\' folder of the project.')

    parser.add_argument('--image', '-i', required=False, type=str, default=None,
                        help='Name of the image to create. By default: "final_<project>_YYYY_mm_DD_HH_MM_SS.sif"')

    parser.add_argument('--debug', '-d', action="store_true",
                        help='Shows debugging information. Temporary files are not removed.')

    args = parser.parse_args()
    return args


def get_ci_job_token() -> Union[str, None]:
    if "CI_JOB_TOKEN" in os.environ:
        return os.getenv('CI_JOB_TOKEN')
    else:
        return None


def get_personal_token() -> Union[str, None]:
    if "PERSONAL_TOKEN" in os.environ:
        return os.getenv('PERSONAL_TOKEN')
    else:
        return None


def generate_singularity_environment_variables(ci_job_token: str,
                                               personal_token: str,
                                               ):
    key_singularityenv_ci_job_token = "SINGULARITYENV_CI_JOB_TOKEN"
    if ci_job_token \
            and key_singularityenv_ci_job_token not in os.environ:
        os.environ[key_singularityenv_ci_job_token] = ci_job_token

    key_singularityenv_personal_token = "SINGULARITYENV_PERSONAL_TOKEN"
    if personal_token \
            and key_singularityenv_personal_token not in os.environ:
        os.environ[key_singularityenv_personal_token] = personal_token


def main():
    args = get_args()

    path_to_singularity_definition_file = args.path_def
    project_commit_ref_to_consider = args.commit_ref
    ci_job_token = args.ci_job_token
    personal_token = args.personal_token
    project_name = args.project
    debug = args.debug
    image_name = args.image

    # doing some checks and print warnings
    check_local_changes()
    check_local_commit_is_pushed(project_commit_ref_to_consider)

    # getting the orignal singularity file
    data = load_singularity_file(path_to_singularity_definition_file)

    # appling the changes and writing this in ./tmp.def
    apply_changes(data, project_commit_ref_to_consider, ci_job_token, personal_token, project_name)

    # Create environment variables for singularity
    generate_singularity_environment_variables(ci_job_token, personal_token)

    # compiling and deleting ./tmp.def
    compile_container(project_name, image_name, debug)


if __name__ == "__main__":
    main()
