# cast
build tool for C. easy to get started like rust's cargo
```sh
cast init myapp
cd myapp
cast run
doas cast install
```
it reads a `cast.toml` in your project root, which it generates, and handles compilation, profiles and installation.

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
cast builds itself. bootstrap it with the `build.sh` script
