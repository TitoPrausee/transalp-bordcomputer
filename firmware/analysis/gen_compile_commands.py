import json, glob, os
inc = [
 "-Icomponents/gps/include","-Icomponents/utils/include","-Icomponents/diagnostics/include",
 "-Icomponents/sensors/include","-Icomponents/power/include","-Icomponents/storage/include",
 "-Icomponents/ble/include","-Icomponents/ble","-Icomponents/display/include",
 "-Icomponents/ota/include","-Icomponents/hid_controller/include",
 "-Icomponents/carplay_bridge/include","-Icomponents/carplay_bridge",
 "-Imain","-Itest","-Itest/mocks",
]
flags = ["-DTEST_HOST","-std=c11","-Wno-unused-parameter"] + inc
db = []
for f in sorted(glob.glob("components/*/*.c") + glob.glob("main/*.c")):
    db.append({"directory": os.getcwd(), "command": "clang " + " ".join(flags) + " -c " + f + " -o /dev/null", "file": f})
with open("compile_commands.json","w") as fh:
    json.dump(db, fh, indent=2)
print(f"Generated {len(db)} entries")
