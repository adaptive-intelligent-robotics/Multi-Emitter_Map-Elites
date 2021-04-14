#!/usr/bin/env python3

import argparse
import os
import subprocess

import build_final_image

EXP_PATH = "/git/sferes2/exp/"


def get_default_image_name():
    return f"{build_final_image.get_project_folder_name()}.sif"


def build_sandbox(path_singularity_def: str,
                  image_name: str):
    # check if the sandbox has already been created
    if os.path.exists(image_name):
        return

    print(f"{image_name} does not exist, building it now from {path_singularity_def}")
    assert os.path.exists(path_singularity_def)  # exit if path_singularity_definition_file is not found

    # run commands
    command = f"singularity build --force --fakeroot --sandbox {image_name} {path_singularity_def}"
    subprocess.run(command.split())


def run_container(nvidia: bool,
                  use_no_home: bool,
                  image_name: str):
    additional_args = ""

    if nvidia:
        print("Nvidia runtime ON")
        additional_args += " " + "--nv"

    if use_no_home:
        print("Using --no-home")
        additional_args += " " + "--no-home"

    command = f"singularity shell -w {additional_args} " \
              f"--bind {os.path.dirname(os.getcwd())}:{EXP_PATH}/{image_name[:-4]} " \
              f"{image_name}"
    subprocess.run(command.split())


def get_args():
    parser = argparse.ArgumentParser(description='Build a sandbox container and shell into it.',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-n', '--nv', action='store_true', help='enable experimental Nvidia support')
    parser.add_argument('--no-home', action='store_true', help='apply --no-home to "singularity shell"')

    parser.add_argument('--path-def', required=False, type=str,
                        default=build_final_image.SINGULARITY_DEFINITION_FILE_NAME,
                        help='path to singularity definition file')

    parser.add_argument('-i', '--image', required=False, type=str,
                        default=get_default_image_name(),
                        help='name of the sandbox image to start')

    args = parser.parse_args()

    return args


def main():
    args = get_args()

    enable_nvidia_support = args.nv
    use_no_home = args.no_home
    path_singularity_definition_file = args.path_def
    image_name = args.image

    build_sandbox(path_singularity_definition_file, image_name)
    run_container(enable_nvidia_support, use_no_home, image_name)


if __name__ == "__main__":
    main()
