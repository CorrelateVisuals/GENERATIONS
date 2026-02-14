#!/usr/bin/env python3
"""
Check versions of vendored header-only libraries.
Run as part of quarterly maintenance to verify library versions.
"""

import re
import sys
from pathlib import Path
from typing import Optional

def extract_version(file_path: Path, patterns: list[str]) -> Optional[str]:
    """Try multiple regex patterns to extract version from file."""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            # Read first 20KB (versions usually in header comments)
            content = f.read(20000)
            
        for pattern in patterns:
            match = re.search(pattern, content, re.IGNORECASE | re.MULTILINE)
            if match:
                return match.group(1)
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
    
    return None

def main():
    repo_root = Path(__file__).parent.parent
    libs_dir = repo_root / "libraries"
    
    # Define libraries and their version detection patterns
    libraries = {
        'vk_mem_alloc.h': [
            r'VMA_VERSION\s+(\d+\.\d+\.\d+)',
            r'version\s+(\d+\.\d+\.\d+)',
            r'@version\s+(\d+\.\d+\.\d+)',
        ],
        'stb_image.h': [
            r'stb_image\s+-\s+v(\d+\.\d+)',
            r'version\s+(\d+\.\d+)',
            r'stbi__\w+\s+v(\d+\.\d+)',
        ],
        'stb_image_write.h': [
            r'stb_image_write\s+-\s+v(\d+\.\d+)',
            r'version\s+(\d+\.\d+)',
        ],
        'tiny_obj_loader.h': [
            r'version\s+(\d+\.\d+\.\d+(?:rc\d+)?)',
            r'TINYOBJLOADER_VERSION\s+"([^"]+)"',
            r'@version\s+(\d+\.\d+\.\d+)',
        ],
    }
    
    print("=" * 60)
    print("Vendored Library Version Check")
    print("=" * 60)
    print()
    
    all_found = True
    results = []
    
    for lib_name, patterns in libraries.items():
        lib_path = libs_dir / lib_name
        
        if not lib_path.exists():
            result = f"❌ {lib_name}: FILE NOT FOUND"
            results.append(result)
            all_found = False
            continue
        
        version = extract_version(lib_path, patterns)
        
        if version:
            result = f"✅ {lib_name}: {version}"
            results.append(result)
        else:
            result = f"⚠️  {lib_name}: VERSION NOT DETECTED"
            results.append(result)
            all_found = False
    
    # Print results
    for result in results:
        print(result)
    
    print()
    print("=" * 60)
    
    if not all_found:
        print()
        print("⚠️  Action Required:")
        print("   - Some versions could not be automatically detected")
        print("   - Manually check library headers for version information")
        print("   - Update libraries/VERSIONS.md with correct versions")
        print()
        print("📝 Next Steps:")
        print("   1. Open each library file and search for version info")
        print("   2. Update libraries/VERSIONS.md")
        print("   3. Consider adding version comments to file headers")
        return 1
    
    print()
    print("✅ Success!")
    print("   All library versions detected successfully.")
    print()
    print("📝 Next Steps:")
    print("   1. Update libraries/VERSIONS.md with these versions")
    print("   2. Check upstream repositories for updates")
    print("   3. Review security advisories for these versions")
    return 0

if __name__ == "__main__":
    sys.exit(main())
