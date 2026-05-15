# cast
Build tool for C. Easy to get started like Rust's Cargo
```sh
cast init myapp
cd myapp
cast build
```
It reads a `cast.toml` in your project root, which it generates, and handles compilation, profiles and installation.

```
cast - a build tool for C

usage:
  cast init [name]     scaffold a new project
  cast build           build (debug)
  cast build --release build (release)
  cast clean           remove build directory
  cast run [args...]   build and run
  cast install         install binary to prefix
  cast --help          show this message
```

## building cast
Cast builds itself. Bootstrap it with the build.sh:
```sh
sh build.sh && cast build
```