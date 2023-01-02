Import("env")

print("Replacing")
env.Replace(AR="emar")
env.Replace(CC="emcc")
env.Replace(CXX="em++")