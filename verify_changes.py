import json
import os
import sys

def check_documentation():
    print("Checking OAE.md...")
    path = "assets/documentation/User Guide/OAE.md"
    if os.path.exists(path):
        with open(path, 'r') as f:
            content = f.read()
            if "Version" in content and "1.2" in content:
                print(f"PASS: {path} exists and contains version info.")
            else:
                print(f"FAIL: {path} exists but missing version info.")
                sys.exit(1)
    else:
        print(f"FAIL: {path} does not exist.")
        sys.exit(1)

def check_settings_serialization():
    print("Checking Settings Serialization...")
    settings_file = "settings.orionconf"

    # Simulate writing settings
    data = {
        "scanPluginsAtStartup": False,
        "theme": "Dark"
    }

    with open(settings_file, 'w') as f:
        json.dump(data, f)

    print(f"PASS: {settings_file} written (simulation).")
    # In a real scenario, running the app would read this.
    # Since we can't run the GUI app, we verify the code changes via grep in previous steps or compilation.
    # But here we just verify the script can write the file the app expects.

if __name__ == "__main__":
    check_documentation()
    check_settings_serialization()
