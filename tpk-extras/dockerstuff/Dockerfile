# Use an official Ubuntu as the base image
FROM ubuntu:22.04

# Set environment variables to non-interactive
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary dependencies
RUN apt-get update && \
    apt-get install -y \
        libSDL2-2.0-0 \
        libgl1-mesa-glx \
        libpng16-16 \
        libfreetype6 \
        libopenal1 \
        libvorbisfile3 \
        libcurl4 \
        libboost-filesystem1.74.0 \
        libboost-system1.74.0 \
        && rm -rf /var/lib/apt/lists/*

# Create a directory for the application
WORKDIR /app

# Copy the supertuxkart binary and config file into the container
COPY stk-code/cmake_build/bin/supertuxkart ./stk-code/supertuxkart
COPY stk-code/cmake_build/bin/stk-tpk-config.xml ./stk-code/stk-tpk-config.xml
COPY stk-code/data ./stk-code/data 
COPY stk-assets/library ./stk-assets/library
COPY stk-assets/models ./stk-assets/models
COPY stk-assets/music ./stk-assets/music
COPY stk-assets/sfx ./stk-assets/sfx
COPY stk-assets/textures ./stk-assets/textures
COPY stk-assets/tracks ./stk-assets/tracks
COPY stk-assets/karts ./stk-assets/karts

# Ensure the binary has execute permissions
RUN chmod +x ./stk-code/supertuxkart

# Expose necessary ports (adjust if your server uses different ports)
EXPOSE 2757/udp
EXPOSE 2757/tcp
EXPOSE 2759/udp
EXPOSE 2759/tcp

# Define the default command to run the application
CMD ["./stk-code/supertuxkart", "--server-config=./stk-code/stk-tpk-config.xml", "--lan-server=stk-tpk-server", "--network-console"]

