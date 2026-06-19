"""
Pre-build script: Generate .S assembly wrappers for managed component
embedded certificate files.

PlatformIO's SCons/cmake bridge in dual-framework (Arduino+ESP-IDF) mode
does not properly handle cmake's target_add_binary_data() for managed
components. The EXCLUDE_COMPONENTS variable in CMakeLists.txt is also
ignored. This script generates the missing .S files unconditionally at
script load time so they exist before SCons evaluates its targets.
"""

Import("env")
import os

# Cert files that managed components try to embed via target_add_binary_data()
EMBEDDED_CERTS = [
    ("espressif__esp_insights", "server_certs/https_server.crt"),
    ("espressif__esp_insights", "server_certs/mqtt_server.crt"),
    ("espressif__esp_rainmaker", "server_certs/rmaker_mqtt_server.crt"),
    ("espressif__esp_rainmaker", "server_certs/rmaker_claim_service_server.crt"),
    ("espressif__esp_rainmaker", "server_certs/rmaker_ota_server.crt"),
]

build_dir = env.subst("$BUILD_DIR")
project_dir = env.subst("$PROJECT_DIR")
managed_dir = os.path.join(project_dir, "managed_components")

os.makedirs(build_dir, exist_ok=True)

for component, cert_rel in EMBEDDED_CERTS:
    cert_abs = os.path.join(managed_dir, component, cert_rel)
    if not os.path.isfile(cert_abs):
        continue

    cert_basename = os.path.basename(cert_rel)
    asm_path = os.path.join(build_dir, cert_basename + ".S")

    if os.path.isfile(asm_path):
        continue

    # Generate the .S wrapper that .incbin's the cert
    sym = cert_basename.replace(".", "_").replace("-", "_")
    with open(asm_path, "w") as f:
        f.write(".data\n")
        f.write(".section .rodata.embedded\n")
        f.write('.global {0}_start\n{0}_start:\n'.format(sym))
        f.write('.incbin "{}"\n'.format(cert_abs.replace("\\", "/")))
        f.write('.global {0}_end\n{0}_end:\n'.format(sym))

    print("[gen_cert_asm] Generated %s" % asm_path)
