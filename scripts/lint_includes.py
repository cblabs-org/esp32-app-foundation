import os
import re
import sys
from collections import defaultdict

def get_includes(filepath):
    includes = []
    include_re = re.compile(r'^\s*#include\s+["<]([^">]+)[">]')
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            for line in f:
                match = include_re.match(line)
                if match:
                    includes.append(match.group(1))
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
    return includes

def check_architectural_boundaries(root_dir):
    # Rule 1: Files in modules/ should not include files from src/
    violations = []
    modules_dir = os.path.join(root_dir, 'modules')
    if not os.path.exists(modules_dir):
        return []
        
    for root, _, files in os.walk(modules_dir):
        for file in files:
            if file.endswith(('.cpp', '.h', '.hpp', '.c')):
                filepath = os.path.join(root, file)
                includes = get_includes(filepath)
                for inc in includes:
                    # Very basic heuristic: if it includes something that looks like an app header
                    if inc.startswith('app_') or inc.startswith('../src/') or inc == 'main.h':
                        violations.append((filepath, inc, "Modules should not depend on application (src) headers"))
    return violations

def build_dependency_graph(root_dir):
    graph = defaultdict(list)
    
    dirs_to_check = ['src', 'modules', 'include']
    
    file_to_module = {}
    
    # First pass: identify all headers and their corresponding modules/components
    for d in dirs_to_check:
        base_path = os.path.join(root_dir, d)
        if not os.path.exists(base_path):
            continue
            
        for root, _, files in os.walk(base_path):
            for file in files:
                if file.endswith(('.h', '.hpp')):
                    # Use filename as node for graph
                    file_to_module[file] = d
                    
    # Second pass: extract dependencies
    for d in dirs_to_check:
        base_path = os.path.join(root_dir, d)
        if not os.path.exists(base_path):
            continue
            
        for root, _, files in os.walk(base_path):
            for file in files:
                if file.endswith(('.cpp', '.c', '.h', '.hpp')):
                    filepath = os.path.join(root, file)
                    includes = get_includes(filepath)
                    
                    # If this is a header, map its includes
                    if file.endswith(('.h', '.hpp')):
                        source_node = file
                        for inc in includes:
                            inc_filename = os.path.basename(inc)
                            if inc_filename in file_to_module:
                                graph[source_node].append(inc_filename)
    return graph

def find_cycles(graph):
    visited = set()
    path = []
    cycles = []
    
    def dfs(node):
        if node in path:
            cycle_start = path.index(node)
            cycles.append(path[cycle_start:] + [node])
            return
        if node in visited:
            return
            
        visited.add(node)
        path.append(node)
        
        for neighbor in graph.get(node, []):
            dfs(neighbor)
            
        path.pop()

    for node in list(graph.keys()):
        if node not in visited:
            dfs(node)
            
    return cycles

def main():
    root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    print(f"Linting includes in {root_dir}...")
    
    has_errors = False
    
    # 1. Check architectural boundaries
    violations = check_architectural_boundaries(root_dir)
    if violations:
        print("\n❌ Architectural boundary violations found:")
        for filepath, inc, reason in violations:
            rel_path = os.path.relpath(filepath, root_dir)
            print(f"  {rel_path}: includes '{inc}' - {reason}")
        has_errors = True
    else:
        print("✅ No architectural boundary violations found.")

    # 2. Check for circular dependencies
    graph = build_dependency_graph(root_dir)
    cycles = find_cycles(graph)
    
    if cycles:
        print("\n❌ Circular dependencies found:")
        for cycle in cycles:
            print("  " + " -> ".join(cycle))
        has_errors = True
    else:
        print("✅ No circular dependencies found.")
        
    if has_errors:
        sys.exit(1)
    else:
        print("\nAll include linting checks passed!")
        sys.exit(0)

if __name__ == '__main__':
    main()
