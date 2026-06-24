Import("env")

import os
import re


_TRUE_VALUES = {"1", "true", "yes", "on"}
_FALSE_VALUES = {"", "0", "false", "no", "off"}
_COMMIT_PATTERN = re.compile(r"^[0-9a-fA-F]{7,40}$")


def _read_boolean_env(name):
    value = os.getenv(name, "").strip().lower()
    if value in _TRUE_VALUES:
        return True
    if value in _FALSE_VALUES:
        return False
    raise ValueError(f"{name} must be a boolean value, got {value!r}")


defines = []
testing_channel = _read_boolean_env("ALTRUIST_CHANNEL_TESTING")
env["ALTRUIST_ARTIFACT_CHANNEL"] = "testing" if testing_channel else "stable"

if testing_channel:
    defines.append("ALTRUIST_CHANNEL_TESTING")

if _read_boolean_env("ALTRUIST_HEALTH_TELEMETRY"):
    defines.append("ALTRUIST_HEALTH_TELEMETRY")

build_commit = os.getenv("ALTRUIST_BUILD_COMMIT", "").strip()
if build_commit:
    if not _COMMIT_PATTERN.fullmatch(build_commit):
        raise ValueError(
            "ALTRUIST_BUILD_COMMIT must contain a 7-40 character hexadecimal SHA"
        )
    defines.append(("ALTRUIST_BUILD_COMMIT", env.StringifyMacro(build_commit)))

if defines:
    env.Append(CPPDEFINES=defines)
    define_names = [
        item[0] if isinstance(item, tuple) else item
        for item in defines
    ]
    print("Applied CI build defines:", ", ".join(define_names))
