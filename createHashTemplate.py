#!/usr/bin/env python3
"""
	Interactive script to create a new hash C++ file template for BioHasher.
"""

import os
import re
import sys
from datetime import datetime


# Available source status options
SRC_STATUS_OPTIONS = ["UNKNOWN","FROZEN", "STABLEISH", "ACTIVE"]

# Available license options
LICENSE_OPTIONS = {
    "1": ("MIT License", "Licensed under the MIT License."),
    "2": ("Apache 2.0", "Licensed under the Apache 2.0 License."),
    "3": ("BSD 2-Clause", "Licensed under the BSD 2-Clause License."),
    "4": ("BSD 3-Clause", "Licensed under the BSD 3-Clause License."),
    "5": ("GPL v2", "Licensed under the GNU General Public License v2.0."),
    "6": ("GPL v3", "Licensed under the GNU General Public License v3.0."),
    "7": ("Public Domain", "Released into the Public Domain / CC0."),
    "8": ("Custom", None),  # User will enter custom text
}

# Common hash bit sizes
BITS_OPTIONS = [32, 64, 128, 256, 512]

TEMPLATE_HEADER = '''/*
 * {hash_name}
 * Copyright (C) {year} {author_name}
 *
 * {license_text}
 */
#include "Platform.h"
#include "Hashlib.h"

//------------------------------------------------------------
// {hash_name} implementation
'''

TEMPLATE_HASH_FUNCTION_START = '''
//------------------------------------------------------------
template <bool bswap>
static void {hash_name}{suffix}( const void * in, const size_t len, const seed_t seed, void * out ) {{
{hash_vars}
	// Copy the hash state to the output.
{put_calls}
}}
'''

TEMPLATE_FAMILY_REGISTER = '''
//------------------------------------------------------------
REGISTER_FAMILY({family_name},
   $.src_url    = "{repo_url}",
   $.src_status = HashFamilyInfo::SRC_{src_status}
 );
'''

TEMPLATE_HASH_REGISTER = '''
REGISTER_HASH({hash_name}{suffix},
   $.desc            = "{description}{bits_desc}",
   $.hash_flags      =
		 0,
   $.impl_flags      =
		 0,
   $.bits            = {bits},
   $.verification_LE = 0x0,
   $.verification_BE = 0x0,
   $.hashfn_native   = {hash_name}{suffix}<false>,
   $.hashfn_bswap    = {hash_name}{suffix}<true>
 );
'''


def generate_hash_function_body(bits: int) -> tuple[str, str]:
    """
    Generate the hash variable declarations and PUT_U* calls based on bit size.
    Returns (hash_vars, put_calls).
    """
    if bits == 32:
        hash_vars = "\t// Output: 32 bits\n\tuint32_t hash = 0;"
        put_calls = "\tPUT_U32<bswap>(hash, (uint8_t *)out, 0);"
    elif bits == 64:
        hash_vars = "\t// Output: 64 bits\n\tuint64_t hash = 0;"
        put_calls = "\tPUT_U64<bswap>(hash, (uint8_t *)out, 0);"
    else:
        # For non-standard bit sizes, add warning comments
        num_u64 = bits // 64
        remainder = bits % 64
        
        # Generate variable declarations with warning
        var_lines = [
            f"\t// Output: {bits} bits",
            "\t// WARNING: This is not a standard output size (32 or 64 bits).",
            "\t// Please change the datatype of hash value storage and",
            "\t// the memory copy mechanism below according to your implementation.",
            "\t//"
        ]
        
        # Add example structure
        for i in range(num_u64):
            var_lines.append(f"\tuint64_t hash{i} = 0;  // bits {i*64} to {(i+1)*64 - 1}")
        if remainder > 0:
            start_bit = num_u64 * 64
            end_bit = bits - 1
            if remainder <= 32:
                var_lines.append(f"\tuint32_t hash{num_u64} = 0;  // bits {start_bit} to {end_bit}")
            else:
                var_lines.append(f"\tuint64_t hash{num_u64} = 0;  // bits {start_bit} to {end_bit}")
        hash_vars = '\n'.join(var_lines)
        
        # Generate PUT calls
        put_lines = []
        offset = 0
        for i in range(num_u64):
            put_lines.append(f"\tPUT_U64<bswap>(hash{i}, (uint8_t *)out, {offset});")
            offset += 8
        if remainder > 0:
            if remainder <= 32:
                put_lines.append(f"\tPUT_U32<bswap>(hash{num_u64}, (uint8_t *)out, {offset});")
            else:
                put_lines.append(f"\tPUT_U64<bswap>(hash{num_u64}, (uint8_t *)out, {offset});")
        put_calls = '\n'.join(put_lines)
    
    return hash_vars, put_calls


def print_header():
    """Print script header."""
    print("=" * 60)
    print("  BioHasher - New Hash File Generator")
    print("=" * 60)
    print()


def print_step(step_num: int, total_steps: int, title: str):
    """Print step header."""
    print(f"\n[Step {step_num}/{total_steps}] {title}")
    print("-" * 40)


def validate_hash_name(name: str) -> tuple[bool, str]:
    """
    Validate hash name.
    Returns (is_valid, error_message).
    """
    if not name:
        return False, "Hash name cannot be empty."
    
    if len(name) < 2:
        return False, "Hash name must be at least 2 characters long."
    
    if len(name) > 50:
        return False, "Hash name must be 50 characters or less."
    
    if not name[0].isalpha():
        return False, "Hash name must start with a letter."
    
    if not re.match(r'^[a-zA-Z][a-zA-Z0-9_]*$', name):
        return False, "Hash name can only contain letters, numbers, and underscores (no spaces or special characters)."
    
    # Check for reserved C++ keywords
    reserved_keywords = {'if', 'else', 'while', 'for', 'do', 'switch', 'case', 'break',
                         'continue', 'return', 'void', 'int', 'char', 'float', 'double',
                         'class', 'struct', 'enum', 'union', 'typedef', 'static', 'const',
                         'template', 'typename', 'namespace', 'using', 'public', 'private',
                         'protected', 'virtual', 'override', 'new', 'delete', 'this', 'true', 'false'}
    if name.lower() in reserved_keywords:
        return False, f"'{name}' is a reserved C++ keyword and cannot be used."
    
    return True, ""


def validate_author_name(name: str) -> tuple[bool, str]:
    """
    Validate author name.
    Returns (is_valid, error_message).
    """
    if not name:
        return False, "Author name cannot be empty."
    
    if len(name) < 2:
        return False, "Author name must be at least 2 characters long."
    
    if len(name) > 100:
        return False, "Author name must be 100 characters or less."
    
    # Allow letters, spaces, hyphens, apostrophes, and periods
    if not re.match(r"^[a-zA-Z][a-zA-Z\s\-'.]*$", name):
        return False, "Author name can only contain letters, spaces, hyphens, apostrophes, and periods."
    
    return True, ""


def validate_family_name(name: str) -> tuple[bool, str]:
    """
    Validate family name (same rules as hash name).
    Returns (is_valid, error_message).
    """
    if not name:
        return False, "Family name cannot be empty."
    
    return validate_hash_name(name)


def validate_url(url: str) -> tuple[bool, str]:
    """
    Validate repository URL.
    Returns (is_valid, error_message).
    """
    if not url:
        return False, "Repository URL cannot be empty."
    
    # Basic URL pattern check
    url_pattern = r'^https?://[^\s<>"{}|\\^`\[\]]+$'
    if not re.match(url_pattern, url):
        return False, "Invalid URL format. URL must start with http:// or https://"
    
    if len(url) > 500:
        return False, "URL must be 500 characters or less."
    
    return True, ""


def validate_description(desc: str) -> tuple[bool, str]:
    """
    Validate hash description.
    Returns (is_valid, error_message).
    """
    if not desc:
        return False, "Description cannot be empty."
    
    if len(desc) < 5:
        return False, "Description must be at least 5 characters long."
    
    if len(desc) > 200:
        return False, "Description must be 200 characters or less."
    
    # Check for problematic characters in C++ strings
    if '"' in desc and '\\' not in desc:
        return False, "Description contains unescaped quotes. Use \\\" for quotes."
    
    return True, ""


def validate_bits(bits_str: str) -> tuple[bool, str, int]:
    """
    Validate hash bits.
    Returns (is_valid, error_message, bits_value).
    """
    try:
        bits = int(bits_str)
    except ValueError:
        return False, "Bits must be a valid number.", 0
    
    if bits <= 0:
        return False, "Bits must be a positive number.", 0
    
    if bits > 1024:
        return False, "Bits must be 1024 or less.", 0
    
    if bits % 8 != 0:
        return False, "Bits should be a multiple of 8.", 0
    
    return True, "", bits


def get_input_with_validation(prompt: str, validator, default: str | None = None) -> str:
    """
    Get user input with validation.
    """
    while True:
        if default:
            user_input = input(f"{prompt} [{default}]: ").strip()
            if not user_input:
                user_input = default
        else:
            user_input = input(f"{prompt}: ").strip()
        
        result = validator(user_input)
        if len(result) == 2:
            is_valid, error_msg = result
            if is_valid:
                return user_input
            print(f"   Error: {error_msg}")
        else:
            is_valid, error_msg, value = result
            if is_valid:
                return value
            print(f"   Error: {error_msg}")


def get_choice_input(prompt: str, options: list, default: str | None = None) -> str:
    """
    Get user input from a list of choices.
    """
    print(f"\n{prompt}")
    print("Available options:")
    for i, opt in enumerate(options, 1):
        marker = " (default)" if default and opt == default else ""
        print(f"  {i}. {opt}{marker}")
    
    while True:
        if default:
            user_input = input(f"Enter choice [1-{len(options)}] or press Enter for default: ").strip()
            if not user_input:
                return default
        else:
            user_input = input(f"Enter choice [1-{len(options)}]: ").strip()
        
        try:
            choice_idx = int(user_input) - 1
            if 0 <= choice_idx < len(options):
                return options[choice_idx]
            print(f"   Error: Please enter a number between 1 and {len(options)}.")
        except ValueError:
            # Check if user typed the option name directly
            if user_input.upper() in [opt.upper() for opt in options]:
                for opt in options:
                    if opt.upper() == user_input.upper():
                        return opt
            print(f"   Error: Invalid input. Enter a number or option name.")


def get_license_input() -> str:
    """
    Get license choice from user.
    """
    print("\nAvailable license options:")
    for key, (name, _) in LICENSE_OPTIONS.items():
        print(f"  {key}. {name}")
    
    while True:
        user_input = input("Enter choice [1-8] or press Enter for MIT License: ").strip()
        
        if not user_input:
            return LICENSE_OPTIONS["1"][1]
        
        if user_input in LICENSE_OPTIONS:
            name, text = LICENSE_OPTIONS[user_input]
            if text is None:  # Custom license
                custom_text = input("Enter your custom license text: ").strip()
                if not custom_text:
                    print("   Error: Custom license text cannot be empty.")
                    continue
                return custom_text
            return text
        
        print(f"   Error: Please enter a number between 1 and {len(LICENSE_OPTIONS)}.")


def get_bits_input() -> list[int]:
    """
    Get hash bits from user. Allows multiple selections.
    Returns a sorted list of unique bit sizes.
    """
    print("\nCommon hash bit sizes:")
    for i, bits in enumerate(BITS_OPTIONS, 1):
        marker = " (default)" if bits == 32 else ""
        print(f"  {i}. {bits} bits{marker}")
    print(f"  {len(BITS_OPTIONS) + 1}. Custom")
    print("\nYou can select multiple sizes by separating with commas (e.g., '1,2,3' or '32,64,128') \n Custom bit size should be multiple of 8.")
    
    while True:
        user_input = input(f"Enter choice(s) [1-{len(BITS_OPTIONS) + 1}] or press Enter for 32 bits: ").strip()
        
        if not user_input:
            return [32]
        
        selected_bits = set()
        parts = [p.strip() for p in user_input.split(',')]
        has_error = False
        
        for part in parts:
            if not part:
                continue
            
            try:
                choice_idx = int(part)
                if 1 <= choice_idx <= len(BITS_OPTIONS):
                    selected_bits.add(BITS_OPTIONS[choice_idx - 1])
                elif choice_idx == len(BITS_OPTIONS) + 1:
                    # Custom bits
                    custom_input = input("Enter custom bit size: ").strip()
                    is_valid, error_msg, bits = validate_bits(custom_input)
                    if is_valid:
                        selected_bits.add(bits)
                    else:
                        print(f"   Error: {error_msg}")
                        has_error = True
                elif choice_idx > 8:
                    # Might be a direct bit value
                    is_valid, error_msg, bits = validate_bits(part)
                    if is_valid:
                        selected_bits.add(bits)
                    else:
                        print(f"   Error for '{part}': {error_msg}")
                        has_error = True
                else:
                    print(f"   Error: Invalid choice '{part}'. Please enter 1-{len(BITS_OPTIONS) + 1}.")
                    has_error = True
            except ValueError:
                # Try parsing as direct bits value
                is_valid, error_msg, bits = validate_bits(part)
                if is_valid:
                    selected_bits.add(bits)
                else:
                    print(f"   Error for '{part}': Invalid input. Enter a choice number or bit size.")
                    has_error = True
        
        if has_error:
            print("\nPlease try again.")
            continue
        
        if not selected_bits:
            print("   Error: No valid bit sizes selected.")
            continue
        
        result = sorted(selected_bits)
        print(f"  Selected: {', '.join(str(b) + ' bits' for b in result)}")
        confirm = input("  Confirm selection? [Y/n]: ").strip().lower()
        if confirm in ('', 'y', 'yes'):
            return result
        # Otherwise loop again


def confirm_creation(config: dict) -> bool:
    """
    Show summary and ask for confirmation.
    """
    bits_list = config['bits']
    bits_display = ', '.join(str(b) for b in bits_list)
    
    print("\n" + "=" * 60)
    print("  Summary - Please Review")
    print("=" * 60)
    print(f"  Hash Name:     {config['hash_name']}")
    print(f"  Family Name:   {config['family_name']}")
    print(f"  Author:        {config['author_name']}")
    print(f"  License:       {config['license_text'][:50]}...")
    print(f"  Repository:    {config['repo_url']}")
    print(f"  Source Status: {config['src_status']}")
    print(f"  Description:   {config['description']}")
    print(f"  Bit Sizes:     {bits_display} ({len(bits_list)} variant{'s' if len(bits_list) > 1 else ''})")
    print(f"  Output File:   {config['filepath']}")
    print("=" * 60)
    
    while True:
        confirm = input("\nCreate this file? [Y/n]: ").strip().lower()
        if confirm in ('', 'y', 'yes'):
            return True
        elif confirm in ('n', 'no'):
            return False
        print("  Please enter 'y' for yes or 'n' for no.")


def check_file_exists(filepath: str) -> bool:
    """
    Check if file exists and ask user what to do.
    """
    if os.path.exists(filepath):
        print(f"\n Warning: File already exists: {filepath}")
        while True:
            choice = input("Overwrite? [y/N]: ").strip().lower()
            if choice in ('', 'n', 'no'):
                return False
            elif choice in ('y', 'yes'):
                return True
            print("  Please enter 'y' for yes or 'n' for no.")
    return True


def update_hashsrc_cmake(filename: str, script_dir: str) -> bool:
    """
    Add the new hash file to hashes/Hashsrc.cmake just before the closing bracket.
    Returns True if successful, False otherwise.
    """
    cmake_path = os.path.join(script_dir, "hashes", "Hashsrc.cmake")
    
    if not os.path.exists(cmake_path):
        print(f"   Warning: Could not find {cmake_path}")
        return False
    
    try:
        with open(cmake_path, 'r') as f:
            content = f.read()
        
        # Check if the file is already listed
        entry = f"hashes/{filename}"
        if entry in content:
            print(f"   '{entry}' is already in Hashsrc.cmake")
            return True
        
        # Find the closing bracket and insert before it
        # Look for the closing ')' that ends the set() command
        # We want to insert just before the final ')'
        lines = content.split('\n')
        insert_index = -1
        
        # Find the last non-empty line before ')'
        for i in range(len(lines) - 1, -1, -1):
            stripped = lines[i].strip()
            if stripped == ')':
                insert_index = i
                break
        
        if insert_index == -1:
            print(f"    Warning: Could not find closing bracket in Hashsrc.cmake")
            return False
        
        # Insert the new entry with proper indentation
        new_entry = f"  {entry}"
        lines.insert(insert_index, new_entry)
        
        # Write back
        with open(cmake_path, 'w') as f:
            f.write('\n'.join(lines))
        
        print(f"   Added '{entry}' to Hashsrc.cmake")
        return True
        
    except Exception as e:
        print(f"    Warning: Could not update Hashsrc.cmake: {e}")
        return False


def create_hash_file(config: dict) -> str:
    """
    Create the hash C++ file from the template.
    Generates separate functions and REGISTER_HASH blocks for each bit size.
    """
    bits_list = config['bits']
    
    # Generate header
    content = TEMPLATE_HEADER.format(
        hash_name=config['hash_name'],
        year=datetime.now().year,
        author_name=config['author_name'],
        license_text=config['license_text']
    )
    
    # Generate a function for each bit size
    for bits in bits_list:
        if len(bits_list) > 1:
            suffix = f"_{bits}"
        else:
            suffix = ""
        
        hash_vars, put_calls = generate_hash_function_body(bits)
        content += TEMPLATE_HASH_FUNCTION_START.format(
            hash_name=config['hash_name'],
            suffix=suffix,
            hash_vars=hash_vars,
            put_calls=put_calls
        )
    
    # Add REGISTER_FAMILY
    content += TEMPLATE_FAMILY_REGISTER.format(
        family_name=config['family_name'],
        repo_url=config['repo_url'],
        src_status=config['src_status']
    )
    
    # Generate REGISTER_HASH for each bit size
    for bits in bits_list:
        if len(bits_list) > 1:
            suffix = f"_{bits}"
            bits_desc = f" ({bits}-bit)"
        else:
            suffix = ""
            bits_desc = ""
        
        content += TEMPLATE_HASH_REGISTER.format(
            hash_name=config['hash_name'],
            suffix=suffix,
            description=config['description'],
            bits_desc=bits_desc,
            bits=bits
        )
    
    with open(config['filepath'], 'w') as f:
        f.write(content)
    
    return config['filepath']


def main():
    print_header()
    
    total_steps = 8
    config = {}
    
    # Determine output directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(script_dir, "hashes")
    
    # Step 1: Hash Name
    print_step(1, total_steps, "Hash Name")
    print("Enter the name for your hash function.")
    print("Rules: Must start with a letter, only letters/numbers/underscores allowed.")
    config['hash_name'] = get_input_with_validation(
        "Hash name",
        validate_hash_name
    )
    
    # Step 2: Author Name
    print_step(2, total_steps, "Author Name")
    print("Enter your name for the copyright notice.")
    config['author_name'] = get_input_with_validation(
        "Author name",
        validate_author_name
    )
    
    # Step 3: License
    print_step(3, total_steps, "License")
    print("Select a license for your hash implementation.")
    config['license_text'] = get_license_input()
    
    # Step 4: Family Name
    print_step(4, total_steps, "Hash Family Name")
    print("Enter the hash family name (can be same as hash name).")
    print(f"Press Enter to use '{config['hash_name']}' as the family name.")
    family_input = input(f"Family name [{config['hash_name']}]: ").strip()
    if family_input:
        is_valid, error_msg = validate_family_name(family_input)
        while not is_valid:
            print(f"  Error: {error_msg}")
            family_input = input(f"Family name [{config['hash_name']}]: ").strip()
            if not family_input:
                break
            is_valid, error_msg = validate_family_name(family_input)
        config['family_name'] = family_input if family_input else config['hash_name']
    else:
        config['family_name'] = config['hash_name']
    
    # Step 5: Repository URL
    print_step(5, total_steps, "Repository URL")
    print("Enter the URL of your source code repository.")
    config['repo_url'] = get_input_with_validation(
        "Repository URL",
        validate_url,
        default="https://github.com/username/repo_name"
    )
    
    # Step 6: Source Status
    print_step(6, total_steps, "Source Status")
    print("Select the source code status.")
    print("  UNKNOWN	- No Information available")
    print("  FROZEN    	- Code is finalized, no changes expected")
    print("  STABLEISH 	- Code is stable but minor updates possible")
    print("  ACTIVE    	- Code is actively being developed")
    config['src_status'] = get_choice_input(
        "Select source status:",
        SRC_STATUS_OPTIONS,
        default="STABLEISH"
    )
    
    # Step 7: Description
    print_step(7, total_steps, "Hash Description")
    print("Enter a brief description of your hash function.")
    config['description'] = get_input_with_validation(
        "Description",
        validate_description
    )
    
    # Step 8: Bits
    print_step(8, total_steps, "Hash Output Size")
    print("Select the hash output size in bits.")
    config['bits'] = get_bits_input()
    
    # Determine file path
    filename = config['hash_name'].lower() + ".cpp"	# the file name should be in lowercase
    config['filepath'] = os.path.join(output_dir, filename)
    
    # Check if file exists
    if not check_file_exists(config['filepath']):
        print("\n Operation cancelled. File not created.")
        sys.exit(0)
    
    # Confirm creation
    if not confirm_creation(config):
        print("\n Operation cancelled. File not created.")
        sys.exit(0)
    
    # Create the file
    try:
        filepath = create_hash_file(config)
        print(f"\n Successfully created: {filepath}")
        
        # Update Hashsrc.cmake
        print("\n Updating Hashsrc.cmake...")
        update_hashsrc_cmake(filename, script_dir)
        
        print(f"\n Next steps:")
        print(f"	1. To test if your {config['hash_name']}Hash function has been added to BioHasher:")
        print(f"	2. Build BioHasher using $mkdir build > $cd build > $make")
        print(f" 	3. run BioHasher using, ./BioHasher --list | grep {config['hash_name']}. It should list your hash.")
        print(f"   	4. Implement the logic of your Hash in the {config['hash_name']}Hash function.")
        
    except Exception as e:
        print(f"\n Error creating file: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n Operation cancelled by user.")
        sys.exit(0)
