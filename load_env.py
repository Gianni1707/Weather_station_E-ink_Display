#
#  PlatformIO pre-build script.
#
#  Reads the (git-ignored) .env file in the project root and injects each
#  KEY=VALUE pair as a -D compiler macro, so secrets/personal config never
#  live in a tracked source file. Keys in STRING_KEYS are emitted as quoted
#  C string literals; everything else is passed through verbatim (numbers).
#
#  If .env is missing the build still works using the fallbacks in config.h.
#
import os

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

STRING_KEYS = {"WIFI_SSID", "WIFI_PASSWORD", "CITY_NAME"}

env_path = os.path.join(env["PROJECT_DIR"], ".env")

if not os.path.isfile(env_path):
    print("[load_env] .env not found -> using config.h fallbacks")
else:
    injected = []
    with open(env_path, "r", encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            key = key.strip()
            value = value.strip()
            # Strip a single matching pair of surrounding quotes, if present.
            if len(value) >= 2 and value[0] == value[-1] and value[0] in ("'", '"'):
                value = value[1:-1]
            if key in STRING_KEYS:
                env.Append(CPPDEFINES=[(key, env.StringifyMacro(value))])
            else:
                env.Append(CPPDEFINES=[(key, value)])
            injected.append(key)
    print("[load_env] injected from .env: " + ", ".join(injected))
