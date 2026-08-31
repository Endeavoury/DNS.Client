use std::env;

fn main() {
    println!("cargo:rerun-if-env-changed=RATATOSKR_LIB_DIR");
    if let Ok(directory) = env::var("RATATOSKR_LIB_DIR") {
        println!("cargo:rustc-link-search=native={directory}");
    }
    println!("cargo:rustc-link-lib=dylib=ratatoskr");
}
