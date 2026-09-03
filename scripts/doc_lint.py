#!/usr/bin/env python3
import os
import sys
import re
import time
import argparse
from pathlib import Path

BANNED_IDS = {
    'reapplyConfig': 'applyConfig',
    'iConfigurable': 'IConfigurable',
    'iDataSource': 'IDataSource',
    'IMcpContributor': 'IMCPContributor',
    'githubClient': '(deleted — remove reference)',
    'updateData': 'fetchUpdate',
    'executeFetch': 'fetchUpdate',
    'reportMessage': 'reportStatus',
    'AsyncWebServer': 'PsychicHttpServer',
    'ESPAsyncWebServer': 'PsychicHttp',
    'NATIVE_BUILD': 'ESP_PLATFORM',
    'isScrollFinished': 'canAdvance',
    'reportProgress': 'reportStatus',
    'requestReload': '(deleted — remove reference)',
    'APP_SETUP': 'Configuring',
    'WIFI_SETUP': 'Configuring'
}

STATUS_PATTERNS = [
    r'STATUS:\s*WIP',
    r'STATUS:\s*FINALIZED',
    r'STATUS:\s*IMPLEMENTED',
    r'STATUS:\s*SUPERSEDED',
    r'\*\*FOLDED\*\*',
    r'\*\*SUPERSEDED\*\*'
]

def main():
    parser = argparse.ArgumentParser(description='Lint documentation for stale identifiers and STATUS banners.')
    parser.add_argument('--verbose', action='store_true', help='Verbose output')
    args = parser.parse_args()

    start_time = time.time()
    
    repo_root = Path(__file__).parent.parent.resolve()
    
    # Directories/files to scan
    scan_paths = [
        repo_root / 'docs',
        repo_root / 'README.md',
        repo_root / 'CONTRIBUTING.md',
        repo_root / '.agents' / 'skills' / 'framework-expert'
    ]
    
    files_to_scan = []
    for p in scan_paths:
        if p.is_file():
            if p.suffix == '.md':
                files_to_scan.append(p)
        elif p.is_dir():
            for root, _, files in os.walk(p):
                for f in files:
                    if f.endswith('.md'):
                        files_to_scan.append(Path(root) / f)
                        
    # Filter out docs/archive/**
    files_to_scan = [f for f in files_to_scan if 'docs/archive' not in f.as_posix()]
    
    if args.verbose:
        print(f"Found {len(files_to_scan)} files to scan.")

    errors = 0
    status_regex = re.compile('|'.join(STATUS_PATTERNS))
    implemented_regex = re.compile(r'STATUS:\s*IMPLEMENTED')
    fold_forward_regex = re.compile(r'(§[0-9]|design_specification\.md)')
    
    # Precompile word boundary regexes for banned keys to avoid substring matching inside other words
    banned_regexes = {key: re.compile(r'\b' + re.escape(key) + r'\b') for key in BANNED_IDS}

    for filepath in files_to_scan:
        rel_path = filepath.relative_to(repo_root)
        
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                lines = f.readlines()
        except Exception as e:
            if args.verbose:
                print(f"Failed to read {rel_path}: {e}")
            continue
            
        is_design_doc = False
        posix_path = filepath.as_posix()
        if 'docs/design/' in posix_path or posix_path.endswith('-design.md'):
            is_design_doc = True
            
        has_status = False
        is_implemented = False
        has_fold_forward = False
        
        for i, line in enumerate(lines, 1):
            # Check for banned identifiers
            # Skip decision-log entries: lines inside a table row that starts with | D[0-9]
            if re.match(r'^\s*\|\s*D[0-9]+', line):
                continue
                
            for banned_key, replacement in BANNED_IDS.items():
                if banned_regexes[banned_key].search(line):
                    print(f"{rel_path}:{i}: BANNED identifier '{banned_key}' — replace with '{replacement}'")
                    errors += 1
                    
            if is_design_doc:
                if status_regex.search(line):
                    has_status = True
                if implemented_regex.search(line):
                    is_implemented = True
                if fold_forward_regex.search(line):
                    has_fold_forward = True
                    
        if is_design_doc:
            if not has_status:
                print(f"{rel_path}: MISSING STATUS banner")
                errors += 1
            elif is_implemented and not has_fold_forward:
                print(f"{rel_path}: IMPLEMENTED doc without fold-forward reference")
                errors += 1
                
    elapsed = time.time() - start_time
    if args.verbose:
        print(f"Scanned {len(files_to_scan)} files in {elapsed:.3f} seconds.")
        
    if errors > 0:
        sys.exit(1)
    else:
        sys.exit(0)

if __name__ == '__main__':
    main()
