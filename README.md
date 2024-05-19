# image-antivirus
Fix virus infected images
 - Only works on images with malicious PNG chunks.
 - Does not work on images with zlib exploits.
 - Removes ICC profiles, all text chunks, unknown chunks, and exif chunks.

  

## Build

Requires [libspng.so](https://libspng.org/)
I recommend that you build libspng from source yourself.

    ./build.sh

## Run
    build/image-antivirus

Inputs all files with PNG extension in the current directory, non-recursively. Outputs them (overwriting if they exist) to a new directory named 'output' in the current working directory.

## Binary
Included binary is a 64 bit elf binary, made for x86_64-linux-gnu
