Import('env')
from os.path import join, realpath

# private library flags
for item in env.get("CPPDEFINES", []):
    if isinstance(item, str) and item.startswith("FREERTOS_PORT"):
        env.Append(CPPPATH=[realpath(join("ports", item[14:]))])
        env.Replace(SRC_FILTER=["+<*>", "-<../ports*>", "+<../ports/%s>" % item[14:]])
        break

