import datetime
import os
import subprocess

Import("env")


def run_git(args, fallback):
    try:
        return subprocess.check_output(
            ["git"] + args,
            cwd=env.subst("$PROJECT_DIR"),
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip()
    except Exception:
        return fallback


project_dir = env.subst("$PROJECT_DIR")
generated_dir = os.path.join(project_dir, ".pio", "generated")
os.makedirs(generated_dir, exist_ok=True)

git_hash = run_git(["rev-parse", "--short", "HEAD"], "nogit")
git_branch = run_git(["rev-parse", "--abbrev-ref", "HEAD"], "unknown")
dirty = run_git(["status", "--porcelain"], "")
dirty_suffix = "-dirty" if dirty else ""
build_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M")
build_env = env.subst("$PIOENV")
version = f"{build_time} {git_hash}{dirty_suffix}"

header_path = os.path.join(generated_dir, "build_version.h")
with open(header_path, "w", newline="\n") as f:
    f.write("#pragma once\n")
    f.write(f'#define GPS_SOFTWARE_VERSION "{version}"\n')
    f.write(f'#define GPS_BUILD_TIME "{build_time}"\n')
    f.write(f'#define GPS_BUILD_GIT_HASH "{git_hash}"\n')
    f.write(f'#define GPS_BUILD_GIT_BRANCH "{git_branch}"\n')
    f.write(f'#define GPS_BUILD_ENV "{build_env}"\n')
    f.write(f"#define GPS_BUILD_DIRTY {1 if dirty else 0}\n")

env.Append(CPPPATH=[generated_dir])
