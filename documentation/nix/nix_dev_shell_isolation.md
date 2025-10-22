# How Nix Development Shells Isolate Shared Libraries on Linux

When using **Nix development shells** for GCC or Clang development (including libraries like `libclang` and Boost) on a traditional Linux distribution such as **Ubuntu**, Nix provides isolation through environment control and linker configuration rather than full OS-level sandboxing.

---

## 1. Toolchain Selection (Build Time)

Inside a Nix development shell, the **compiler wrappers** provided by Nix take precedence in your `PATH`. These wrappers ensure that headers and libraries are sourced from the Nix store, not the host system.

Key mechanisms:

- **PATH** prioritizes Nix’s wrapped compilers (`gcc`, `clang`)
- Environment variables point to Nix store paths:
  - `PKG_CONFIG_PATH` → `.pc` files in `/nix/store/...`
  - `CMAKE_PREFIX_PATH`, `LIBRARY_PATH`, `CPATH` → `/nix/store/...`
- LLVM/Clang from Nix uses a sysroot inside `/nix/store`, meaning headers and system libraries are isolated from `/usr/include` and `/usr/lib`

---

## 2. Linker Inputs & Rpaths (Link Time)

The compiler wrappers automatically inject linker flags that direct linking toward Nix store paths.

- Adds `-L/nix/store/.../lib`
- Embeds rpaths with `-Wl,-rpath,/nix/store/.../lib`
- Sets the **ELF interpreter** to the Nix glibc dynamic loader (e.g., `/nix/store/.../lib/ld-linux-x86-64.so.2`)

You can verify this with:

```bash
readelf -l ./prog | grep interpreter
```

This ensures binaries always use the Nix-provided glibc loader rather than Ubuntu’s.

---

## 3. Runtime Resolution (Run Time)

Because the interpreter and rpaths both reference the Nix store, runtime linking naturally resolves to Nix-managed shared libraries.

Verification commands:

```bash
ldd ./prog           # should show only /nix/store paths
patchelf --print-rpath ./prog
```

Even if your Ubuntu environment sets `LD_LIBRARY_PATH`, the embedded rpaths will typically override it for directly linked libraries.

---

## 4. Environment Hygiene

Running `nix develop --pure` or `nix-shell --pure` clears inherited environment variables from the host, such as:

- `LD_LIBRARY_PATH`
- `CPATH`
- `LIBRARY_PATH`

This eliminates leakage from `/usr/lib` or `/usr/include`, resulting in a more hermetic environment (though not a full chroot).

---

## 5. When Leaks Can Still Happen

While Nix provides strong isolation, some scenarios can bypass it:

- Manually invoking `/usr/bin/gcc` or `/usr/bin/ld`
- Adding `-L/usr/lib` explicitly
- Software using `dlopen()` to load libraries dynamically by name

**Fixes:**

- Use Nix’s compiler paths (`which gcc` → `/nix/store/...`)
- Use `wrapProgram` or `makeWrapper` to set a controlled `LD_LIBRARY_PATH`
- Ensure build systems embed correct `RUNPATH` values

For truly isolated builds, use `nix build` with sandboxing enabled instead of an interactive shell.

---

## 6. Verifying Isolation

Quick checks inside the shell:

```bash
which gcc            # /nix/store/... path
which clang          # /nix/store/... path
gcc -v               # shows include and lib search dirs from /nix/store
readelf -l a.out | grep interpreter
patchelf --print-rpath a.out
ldd a.out
```

All paths should point exclusively into `/nix/store/...`.

---

## Summary

Nix development shells achieve isolation via:

| Phase | Mechanism | Effect |
|--------|------------|--------|
| **Build time** | Compiler wrappers, env vars | Ensures store-based includes/libs |
| **Link time** | Rpaths, Nix glibc loader | Embeds store paths into ELF |
| **Run time** | Nix loader + rpaths | Resolves shared libs from store |
| **Environment** | `--pure` shell | Clears host contamination |

While not a sandbox in the chroot sense, the combination of Nix’s compiler wrappers, sysroot configuration, and embedded rpaths effectively isolates builds and runtime linking from the host’s shared libraries.

