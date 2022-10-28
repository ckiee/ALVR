#[cfg(target_os = "linux")]
fn main() {
    let argv0 = std::env::args().next().unwrap();
    // location of the ALVR vulkan layer manifest
    let layer_path = match std::fs::read_link(&argv0) {
        Ok(path) => path
            .parent()
            .unwrap()
            .join("../../share/vulkan/explicit_layer.d"),
        Err(err) => panic!("Failed to read vrcompositor symlink: {err}"),
    };
    // TODO: make this toggleable and a PR (?)
    // std::env::set_var("VK_LAYER_PATH", format!("/nix/store/sz2zfg71694y26bhcvhsa2dypklinzpl-vulkan-tools-lunarg-1.3.224.0/etc/vulkan/explicit_layer.d:/nix/store/yn86szvdpgx5w9aylbdw4ial5mqdq524-vulkan-validation-layers-1.3.224.0:{}",layer_path.to_str().unwrap()));
    // std::env::set_var("VK_APIDUMP_LOG_FILENAME","/tmp/vrcompositor_apidump.ckie");
    // std::env::set_var("VK_INSTANCE_LAYERS", "VK_LAYER_ALVR_capture:VK_LAYER_LUNARG_api_dump:VK_LAYER_KHRONOS_validation");
    std::env::set_var("VK_LAYER_PATH", layer_path);
    std::env::set_var("VK_INSTANCE_LAYERS", "VK_LAYER_ALVR_capture");
    // fossilize breaks the ALVR vulkan layer
    std::env::set_var("DISABLE_VK_LAYER_VALVE_steam_fossilize_1", "1");

    let err = exec::execvp(argv0 + ".real", std::env::args());
    println!("Failed to run vrcompositor {err}");
}

#[cfg(not(target_os = "linux"))]
fn main() {
    unimplemented!();
}
