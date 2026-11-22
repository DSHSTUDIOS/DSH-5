from pathlib import Path

Import("env")

# Keep the Teensy DSP library visible when the ARM compiler lives outside its package.
package = env.PioPlatform().get_package_dir("toolchain-gccarmnoneeabi-teensy")
if package:
    env.AppendUnique(LIBPATH=[str(Path(package) / "arm-none-eabi" / "lib")])
