"""
ESP32 App Foundation (c) 2026 CB Labs

SPDX-License-Identifier: Apache-2.0

PlatformIO pre-build script: scripts/u8g2gl_generate.py
Description: Two jobs for the U8G2GL UI of the 256x64 SSD1322 boards.
             1. Regenerates it before a build, when a U8G2GL CLI checkout is
                configured, and otherwise leaves the committed generated/ directory
                as it is ([D-11]: a build never needs the generator).
             2. Applies its generated capacities to the U8G2GL runtime's own sources
                (build middleware, below).

    The project is uidesign/ssd1322-256x64/u8g2gl; its generated/ output is
    committed and consumed through lib_extra_dirs as the library U8G2GLUI.

    Configured:     the CLI path is the environment variable U8G2GL_CLI (a path to
                    cli.js), or else the env option custom_u8g2gl_cli in
                    platformio.ini. The environment variable wins, so a developer
                    can override a project setting without editing it. Then
                        node <cli> generate uidesign/ssd1322-256x64/u8g2gl --target ssd1322
                    runs from the project root. A non-zero exit fails the build,
                    and the CLI's output is printed.
    Not configured: one line is printed saying the committed generated/ is used,
                    and the build continues.

    A configured path that does not exist is an error, not a silent skip: whoever
    set it expects regeneration, and building from a stale generated/ would hide
    that it did not happen.

    Capacities: generated/u8g2gl_generated_config.h holds the capacities counted from the
    XML (here 4 bindings, where the runtime's default is 256). Only the runtime's own
    src/u8g2gl_obj.cpp reads them, to size its binding pool, so the header has to reach
    that file. The header's own advice, `build_flags = -include <header>`, forces it into
    EVERY translation unit of the build, including the Arduino core, the ESP-IDF
    components and unrelated libraries, none of which has u8g2gl on its include path:
    measured, it fails the first of them (`u8g2gl/u8g2gl_version.h: No such file`). The
    scoped form used here is PlatformIO's build middleware: a callback that adds
    `-include <header>` to the compile of the u8g2gl library's src/*.cpp only. The header
    path is derived from the project directory, never written down.

    The eventual shape, once the CLI is published, is `npx u8g2gl generate ...`.
    This script never calls npx: until then it would fetch an unpinned package
    from the network during a build.

    Testability: the decision and the invocation are the plain functions below,
    which run under python3 outside SCons. The SCons wrapper at the bottom is
    thin. To run by hand:
        U8G2GL_CLI=/path/to/cli.js python3 scripts/u8g2gl_generate.py [project_root]
"""

import os
import subprocess
import sys

# SCons injects Import() into the script's globals; plain python3 does not.
try:
    Import("env")  # noqa: F821
except NameError:
    env = None

UI_PROJECT = "uidesign/ssd1322-256x64/u8g2gl"   # The U8G2GL project this hook regenerates
UI_TARGET = "ssd1322"                           # The <target name> in its u8g2gl.xml
CLI_ENV_VAR = "U8G2GL_CLI"                      # Environment variable holding the path to cli.js
CLI_OPTION = "custom_u8g2gl_cli"                # Env option in platformio.ini, used when the variable is unset
UI_LIB_DIR = "uidesign/ssd1322-256x64"          # lib_extra_dirs entry that makes an env use this UI
CAPACITY_HEADER = UI_PROJECT + "/generated/u8g2gl_generated_config.h"  # Capacities, relative to the project root
RUNTIME_SOURCES = "*/u8g2gl/src/*.cpp"          # Glob (on the node path) matching only the runtime's own sources
PREFIX = "[u8g2gl_generate]"                    # Prefix of every line this script prints


def resolve_cli(environ, option_value):
    """Return the configured path to cli.js, or None when nothing is configured.

    The environment variable takes precedence over the option. A leading ~ is
    expanded; the result is otherwise returned as written.

    Args:
        environ: Mapping of environment variables (os.environ).
        option_value: The env's custom_u8g2gl_cli option, or None/"" if unset.

    Returns:
        The expanded path to cli.js, or None when neither source names one.
    """
    chosen = (environ.get(CLI_ENV_VAR) or "").strip() or (option_value or "").strip()
    if not chosen:
        return None
    return os.path.expanduser(chosen)


def generate_ui(project_root, cli, ui_project=UI_PROJECT, target=UI_TARGET, runner=subprocess.run):
    """Run the U8G2GL CLI's generate command for the UI project.

    Args:
        project_root: Directory the command runs from and ui_project is relative to.
        cli: Path to the CLI's cli.js.
        ui_project: Project directory, relative to project_root.
        target: The <target name> to generate for.
        runner: subprocess.run, replaceable in tests.

    Returns:
        (exit_code, output): the CLI's exit code and its combined stdout and stderr.
        A missing cli.js or a missing node is reported as exit code 127 with an
        explanation in the output, and the CLI is never run.
    """
    # --- Step 1: the configured CLI must exist ---
    if not os.path.isfile(cli):
        return 127, "%s configured CLI not found: %s\n" % (PREFIX, cli)

    # --- Step 2: run it ---
    command = ["node", cli, "generate", ui_project, "--target", target]
    try:
        result = runner(command, cwd=project_root, stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT, universal_newlines=True)
    except FileNotFoundError:
        return 127, "%s node is not on PATH; it is needed to run %s\n" % (PREFIX, cli)
    return result.returncode, result.stdout


def run_hook(project_root, environ, option_value, out=print, runner=subprocess.run):
    """The hook's whole decision: skip, or regenerate and report.

    Args:
        project_root: The PlatformIO project directory.
        environ: Mapping of environment variables (os.environ).
        option_value: The env's custom_u8g2gl_cli option, or None.
        out: Where lines are printed.
        runner: subprocess.run, replaceable in tests.

    Returns:
        0 to continue the build, non-zero (the CLI's exit code) to fail it.
    """
    cli = resolve_cli(environ, option_value)

    # --- Not configured: build from the committed generated/ ---
    if cli is None:
        out("%s %s is not set and %s is empty: using the committed %s/generated" %
            (PREFIX, CLI_ENV_VAR, CLI_OPTION, UI_PROJECT))
        return 0

    # --- Configured: regenerate; any failure stops the build ---
    out("%s regenerating %s with %s" % (PREFIX, UI_PROJECT, cli))
    code, output = generate_ui(project_root, cli, runner=runner)
    if output:
        out(output.rstrip("\n"))
    if code != 0:
        out("%s FAILED (exit %d): the build stops rather than use a stale generated/" % (PREFIX, code))
    return code


def capacity_header_path(project_root):
    """Absolute path of the generated capacities header, derived from the project root.

    Raises:
        FileNotFoundError: if the header does not exist (nothing was generated or committed).
    """
    path = os.path.join(project_root, CAPACITY_HEADER)
    if not os.path.isfile(path):
        raise FileNotFoundError("%s generated capacities header not found: %s" % (PREFIX, path))
    return path


def capacity_cc_flags(base_flags, header):
    """CCFLAGS for compiling one runtime source with the capacities header forced in.

    Args:
        base_flags: The env's CCFLAGS (a list).
        header: Absolute path from capacity_header_path().

    Returns:
        A new list; the env's own list is left alone, so no other source is affected.
    """
    return list(base_flags) + ["-include", header]


def env_uses_ui(lib_extra_dirs):
    """True if an env's lib_extra_dirs makes it see this UI project.

    crowpanel-128-rotary overrides lib_extra_dirs and never lists the SSD1322
    directory, so the hook has nothing to do for it. native does not extend this
    script's base env at all.
    """
    # PlatformIO may hand the option back as one string or as a list of entries.
    if isinstance(lib_extra_dirs, (list, tuple)):
        lib_extra_dirs = ",".join(lib_extra_dirs)
    return UI_LIB_DIR in (lib_extra_dirs or "")


# =============================================================================
# SCons wrapper (thin) and manual entry point
# =============================================================================

if env is not None:
    _libs = env.GetProjectOption("lib_extra_dirs", "")
    if env_uses_ui(_libs):
        _root = env.subst("$PROJECT_DIR")
        _code = run_hook(_root, os.environ, env.GetProjectOption(CLI_OPTION, None))
        if _code != 0:
            env.Exit(1)

        # Scope the generated capacities to the runtime's own sources. The middleware runs for
        # every source node but only acts on the ones matching RUNTIME_SOURCES.
        _header = capacity_header_path(_root)

        def _apply_capacities(lib_env, node):
            """Rebuild one runtime source node with the generated capacity header forced in."""
            # The two-argument form hands over the environment of the library being built,
            # which carries that library's own include paths: using the project env here
            # drops them (measured: u8g2gl/u8g2gl_version.h not found). env.Object()
            # returns a list, and the espressif32 platform's own middleware
            # (frameworks/espidf.py) calls node.srcnode() on whatever the previous one
            # returned, so hand on the single object node, not the list.
            return lib_env.Object(node, CCFLAGS=capacity_cc_flags(lib_env["CCFLAGS"], _header))[0]

        env.AddBuildMiddleware(_apply_capacities, RUNTIME_SOURCES)
        print("%s capacities %s forced into the u8g2gl runtime sources only" % (PREFIX, _header))
    else:
        print("%s %s does not use %s: nothing to do" % (PREFIX, env.get("PIOENV"), UI_LIB_DIR))
elif __name__ == "__main__":
    # Manual run: an optional argument names the project root (a tmp/ copy, say);
    # the default is this repository.
    _root = os.path.abspath(sys.argv[1] if len(sys.argv) > 1
                            else os.path.join(os.path.dirname(__file__), ".."))
    sys.exit(run_hook(_root, os.environ, None))
