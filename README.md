# Greenmatter

## Development Requirements

1. The `connectedhomeip` subdirectory must contain the Matter SDK.
2. The Matter SDK in the `connectedhomeip` directory must be configured.
3. The Matter SDK environment must be activated before each development session.

## Getting the Matter SDK

(... based on the instructions [here](https://project-chip.github.io/connectedhomeip-doc/guides/BUILDING.html) ...)

```console
$ git clone --depth=1 git@github.com:project-chip/connectedhomeip.git
$ python3 ./scripts/checkout_submodules.py --shallow --platform  linux
```

> Note: The instructions above are just one way to meet the requirement that the `connectedhomeip` directory
contains a copy of the Matter SDK.

## Configuring Matter SDK

Follow build instructions [here](https://project-chip.github.io/connectedhomeip-doc/guides/BUILDING.html).

### Every Time

There are certain steps that must be taken before _every_ development
session:

1. Activate the matter SDK environment:

```console
$ cd connectedhomeip && source ./scripts/activate.sh
```

### Niceties

ninja will generate a compilation database suitable for clangd:

```console
$ ninja -C out/host -t compdb > compile_commands.json
```

### Fixes For Redhat:

Must change

```diff
diff --git a/scripts/setup/constraints.txt b/scripts/setup/constraints.txt
index 874d46f6..4534c71a 100644
--- a/scripts/setup/constraints.txt
+++ b/scripts/setup/constraints.txt
@@ -141,7 +141,7 @@ packaging==23.0
     #   ghapi
     #   idf-component-manager
     #   west
-pandas==2.1.4 ; platform_machine != "aarch64" and platform_machine != "arm64"
+pandas==2.2.3 ; platform_machine != "aarch64" and platform_machine != "arm64"
     # via -r requirements.memory.txt
 parso==0.8.3
     # via jedi
```
## References:

Carlos Pignataro and Jainam Parikh and Ron Bonica and Michael Welzl, _ICMP Extensions for Environmental Information_, [https://datatracker.ietf.org/doc/draft-pignataro-green-enviro-icmp/](https://datatracker.ietf.org/doc/draft-pignataro-green-enviro-icmp/).