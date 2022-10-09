import sys
import filecmp
from os import path
from shutil import copyfile
import os

Import("env")


def apply(env, path_to_directory_with_custom_library_json, library_name):
    """
    Copies the library.json from the path:
    <project_root_directory>/<path_to_directory_with_custom_library_json>/
    <library_name>/library.json
    to the place in the PIO build directory where the library is downloaded.
    Takes the map with environmental variables from PIO as the first argument
    (obtained with 'Import("env")' and the accessible with 'env' variable).
    """

    root_dir = env['PROJECT_DIR']
    pio_env = env['PIOENV']
    libdeps_dir = env['PROJECT_LIBDEPS_DIR']

    library_json_src_path = path.join(
        root_dir,
        path_to_directory_with_custom_library_json,
        library_name + ".json")

    library_json_dst_path = path.join(
        libdeps_dir, pio_env, library_name, 'library.json')

    local_library_json_dst_path = path.join(
        root_dir, "lib", library_name, 'library.json')

    if path.exists(path.dirname(local_library_json_dst_path)):
          if not path.exists(local_library_json_dst_path) or \
            not filecmp.cmp(library_json_src_path, local_library_json_dst_path):
              print("PRE: Overwriting library.json in lib/{}".format(library_name))
              copyfile(library_json_src_path, local_library_json_dst_path)

    else:
      if not path.exists(path.dirname(library_json_dst_path)):
        print("PRE: The library {} is not found in the building directory".format(
            library_name), file=sys.stderr)
      else:
          if not path.exists(library_json_dst_path) or \
            not filecmp.cmp(library_json_src_path, library_json_dst_path):
              print("PRE: Overwriting library.json in {}".format(library_name))
              copyfile(library_json_src_path, library_json_dst_path)


def glob(env, path_to_directory_with_custom_library_json):
    json_files = [pos_json for pos_json in os.listdir(path_to_directory_with_custom_library_json) if pos_json.endswith('.json')]
    [apply(env, path_to_directory_with_custom_library_json, os.path.splitext(json_file)[0]) for json_file in json_files]


glob(env, env['PROJECT_DIR'] + "/scripts/overlays")
print("PRE!!!!")
