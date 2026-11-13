import sys
import json
import subprocess
import argparse

def fetch_page(per_page, page):
    # Fetch from GH API. We unset GITHUB_TOKEN to avoid 'Bad credentials' if token is invalid,
    # ensuring we hit the public endpoint.
    cmd = f'unset GITHUB_TOKEN && gh api "/repos/cblabs-org/esp32-app-foundation/releases?per_page={per_page}&page={page}"'
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    if result.returncode != 0:
        # Check if it's just an empty array or an error
        if result.stdout.strip() == "[]":
            return []
        print(f"GH API Error: {result.stderr}", file=sys.stderr)
        return None
    return json.loads(result.stdout)

def parse_version(tag):
    """
    Simulates SemVer::parseVersionTag().
    Returns a tuple suitable for comparison: (major, minor, patch, channel, build).
    Channels: dev=0, alpha=1, beta=2, rc=3, stable=255.
    """
    tag = tag.lstrip('v')
    parts = tag.split('-')
    
    main_parts = parts[0].split('.')
    major = int(main_parts[0]) if len(main_parts) > 0 else 0
    minor = int(main_parts[1]) if len(main_parts) > 1 else 0
    patch = int(main_parts[2]) if len(main_parts) > 2 else 0
    
    channel = 255 # stable
    build = 0
    
    if len(parts) > 1:
        pre_parts = parts[1].split('.')
        chan_str = pre_parts[0]
        if chan_str == 'dev': channel = 0
        elif chan_str == 'alpha': channel = 1
        elif chan_str == 'beta': channel = 2
        elif chan_str == 'rc': channel = 3
        
        if len(pre_parts) > 1:
            try:
                build = int(pre_parts[1])
            except ValueError:
                pass
                
    return (major, minor, patch, channel, build)

def get_channel_floor(chan_str):
    if chan_str == 'dev': return 0
    if chan_str == 'alpha': return 1
    if chan_str == 'beta': return 2
    if chan_str == 'rc': return 3
    return 255

def simulate_ota(local_tag, target_channel='dev', per_page=1, max_pages=20):
    current_version = parse_version(local_tag)
    floor = get_channel_floor(target_channel)
    
    best_ver = None
    best_tag = None
    stop_scanning = False
    
    print(f"=== OTA Simulation ===")
    print(f"Local version : {local_tag}")
    print(f"Target Channel: {target_channel} (floor value {floor})")
    print(f"Pagination    : per_page={per_page}, max_pages={max_pages}\n")
    
    for page in range(1, max_pages + 1):
        if stop_scanning:
            break
            
        print(f"--> Requesting page {page}...")
        releases = fetch_page(per_page, page)
        
        if releases is None or len(releases) == 0:
            print(f"    Empty page {page} — stopping scan")
            break
            
        for r in releases:
            tag_name = r.get('tag_name', '')
            if not tag_name: continue
            
            is_pre = r.get('prerelease', False)
            
            # EXIT 1: Hit stable release
            if not is_pre:
                print(f"    Hit stable {tag_name} in scan — stopping (page {page})")
                stop_scanning = True
                break
                
            candidate = parse_version(tag_name)
            
            # Floor check (e.g. if we want beta, we ignore dev)
            if candidate[3] < floor:
                print(f"    Skipping {tag_name} — does not meet floor '{target_channel}'")
                continue
                
            # EXIT 2: Found an acceptable prerelease that beats best candidate
            # Notice we do NOT stop scanning here anymore!
            if best_tag is None or candidate > best_ver:
                print(f"    Found better prerelease {tag_name} (page {page})")
                best_ver = candidate
                best_tag = tag_name
                
    print(f"\n=== Scan Result ===")
    if best_tag is None:
        print("No acceptable releases found.")
    else:
        is_newer = (best_ver > current_version)
        print(f"Best remote release : {best_tag}")
        if is_newer:
            print(f"Action: UPDATE AVAILABLE! ({local_tag} -> {best_tag})")
        else:
            print(f"Action: No update needed. Local {local_tag} is >= remote {best_tag}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Simulate ESP32 OTA pagination logic")
    parser.add_argument("--local", default="v0.0.11-dev.11", help="Current local version tag")
    parser.add_argument("--channel", default="dev", help="Target channel (dev, beta, stable)")
    parser.add_argument("--per-page", type=int, default=1, help="Results per page")
    parser.add_argument("--pages", type=int, default=20, help="Max pages to scan")
    
    args = parser.parse_args()
    simulate_ota(args.local, args.channel, args.per_page, args.pages)
