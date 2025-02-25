import os
import re
import struct
import tarfile

def extract_version(config_path):
    """
    Extracts the software version from a config.h file.
    """
    if not os.path.exists(config_path):
        raise FileNotFoundError(f"File not found: {config_path}")

    with open(config_path, 'r') as f:
        content = f.read()

    # Search for the version definition
    match = re.search(r'#define\s+FW_VERSION\s+"([\d\.]+)"', content)
    if not match:
        raise ValueError("Software version not found in config.h")

    return match.group(1)

def compress_directory_tar(input_dir, output_file):
    """
    Compresses a directory into a TAR.GZ file with deterministic contents.
    """
    import time

    print(f"Compressing directory {input_dir} into {output_file}...")

    # Set a constant timestamp (e.g., 0 or a specific date)
    constant_time = 0  # Unix epoch start time

    with tarfile.open(output_file, "w:gz") as tar:
        # Get a sorted list of all files in the directory
        for root, dirs, files in os.walk(input_dir):
            # Sort the directories and files to ensure consistent order
            dirs.sort()
            files.sort()
            for file in files:
                fullpath = os.path.join(root, file)
                arcname = os.path.relpath(fullpath, start=os.path.dirname(input_dir))

                # Create a TarInfo object for the file
                tarinfo = tar.gettarinfo(fullpath, arcname)

                # Set the mtime to a constant value
                tarinfo.mtime = constant_time

                # Set other metadata to default values if needed
                tarinfo.uid = 0
                tarinfo.gid = 0
                tarinfo.uname = ''
                tarinfo.gname = ''

                # Add the file to the archive
                with open(fullpath, 'rb') as f:
                    tar.addfile(tarinfo, f)
        print(f"Compression complete: {output_file}")

def crc32(data):
    """
    Computes a CRC32 checksum of the given data, matching the STM32 CRC peripheral without output inversion.
    """
    import zlib
    crc = zlib.crc32(data)  # Compute the CRC
    # No final XOR of the CRC
    return crc & 0xFFFFFFFF  # Ensure CRC is 32 bits

def create_update_file(config_path, output_dir):
    """
    Creates a binary update file containing CM7, CM4, and a compressed archive of additional data.
    """
    # Relative paths of required files
    cm7_path = "../CM7/Release/CM7.bin"
    cm4_path = "../CM4/Release/CM4.bin"
    external_dir = "./External_MAX8"
    compressed_archive = "./External_MAX8.tar.gz"

    # Compress the External_MAX8 directory
    # compress_directory_tar(external_dir, compressed_archive)

    # Check existence of required files
    paths_to_check = [cm7_path, cm4_path, compressed_archive, config_path]
    for path in paths_to_check:
        print(f"Checking: {path}")
        if not os.path.exists(path):
            raise FileNotFoundError(f"File not found: {path}")
        if not os.path.isfile(path):
            raise IsADirectoryError(f"Specified path is not a file: {path}")

    # Read the software version
    version = extract_version(config_path)
    print(f"Detected version: {version}")

    # Read binary files
    print("Reading files...")
    with open(cm7_path, 'rb') as f:
        cm7_data = f.read()
        print(f"CM7 firmware size: {len(cm7_data)} bytes")

    with open(cm4_path, 'rb') as f:
        cm4_data = f.read()
        print(f"CM4 firmware size: {len(cm4_data)} bytes")

    with open(compressed_archive, 'rb') as f:
        external_data_tar = f.read()
        print(f"TAR archive size: {len(external_data_tar)} bytes")

    # Create the header
    header = struct.pack(
        '<4sIII8s',
        b'BOOT',                     # Magic number
        len(cm7_data),               # CM7 firmware size
        len(cm4_data),               # CM4 firmware size
        len(external_data_tar),      # Compressed archive size
        version.encode('utf-8')[:8]  # Version (truncated to 8 bytes if necessary)
    )

    # Concatenate all data
    full_data = header + cm7_data + cm4_data + external_data_tar

    # Compute CRC for validation
    crc = crc32(full_data)
    print(f"Computed CRC: {crc:#010x}")  # Ajout du print du CRC

    # Add the footer
    footer = struct.pack('<I', crc)
    full_data += footer

    # Generate output file name based on version
    output_path = os.path.join(output_dir, f"cis_package_{version}.bin")

    # Write to the output file
    with open(output_path, 'wb') as f:
        f.write(full_data)

    print(f"Update file generated: {output_path}")

if __name__ == "__main__":
    # Path to config.h file
    print("Current directory:", os.getcwd())
    config_path = "../Common/Inc/config.h"

    def test_read_ascii(config_path):
        print(f"Exists: {os.path.exists(config_path)}")

        try:
            with open(config_path, 'r', encoding='ascii') as f:
                content = f.read()
            print("Successfully read (ASCII):")
            print(content[:100])  # Display the first 100 characters
        except UnicodeDecodeError as e:
            print(f"Encoding error during reading: {e}")
        except Exception as e:
            print(f"Unexpected error: {e}")

    # Test reading the file to verify it works
    test_read_ascii(config_path)

    # Output directory
    output_dir = "./"

    # Create the update file
    create_update_file(config_path, output_dir)