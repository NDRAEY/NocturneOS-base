set(RUST_COMPILER_TOOLCHAIN "nightly-2025-07-01")
set(RUST_FLAGS -Zbuild-std=core,compiler_builtins,alloc -Znext-lockfile-bump)
set(RUST_TOOLCHAIN_FULLNAME "${RUST_COMPILER_TOOLCHAIN}-x86_64-unknown-linux-gnu")  
set(RUST_CARGO_PATH "~/.rustup/toolchains/${RUST_TOOLCHAIN_FULLNAME}/bin/src/cargo")

function(add_rust_integration target)
	message("test: ${target} ${RUST_TARGET_TRIPLE}")
endfunction()
