#!/usr/bin/env python3
"""
Convert SPIR-V binary file to C header file with uint32_t array.

Usage: python3 spv_to_header.py input.spv output.h variable_name
"""
import sys
import struct

def main():
    if len(sys.argv) != 4:
        print("Usage: spv_to_header.py <input.spv> <output.h> <variable_name>")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]
    var_name = sys.argv[3]

    with open(input_path, 'rb') as f:
        data = f.read()

    # SPIR-V must be a multiple of 4 bytes
    if len(data) % 4 != 0:
        print("Warning: SPIR-V binary size is not a multiple of 4 bytes")

    # Pack as uint32_t array (little-endian, matching most Android devices)
    word_count = len(data) // 4
    words = struct.unpack('<%dI' % word_count, data[:word_count * 4])

    with open(output_path, 'w') as f:
        f.write("// Auto-generated SPIR-V bytecode\n")
        f.write("#pragma once\n")
        f.write("#include <cstdint>\n")
        f.write("#include <cstddef>\n\n")
        f.write("static const uint32_t %s[] = {\n" % var_name)

        for i in range(0, len(words), 8):
            chunk = words[i:i+8]
            hex_vals = ["0x%08x" % w for w in chunk]
            f.write("    " + ", ".join(hex_vals) + ",\n")

        f.write("};\n")
        f.write("static const size_t %s_size = sizeof(%s);\n" % (var_name, var_name))

    print("Generated %s (%d words, %d bytes)" % (output_path, len(words), len(data)))

if __name__ == "__main__":
    main()
