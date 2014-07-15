# Pulls the minimal Arch Linux image and uses it as the base.
# (Why arch? It has the required libpng version in it's repositories.)
FROM base/arch

# Updates all installed packages.
RUN pacman -Syu --noconfirm --ignore filesystem
# Installs the dependencies.
RUN pacman -S --noconfirm cmake openssl libpng gcc git make libevent

# Changes the working directory to `/root` so the repository won't be cloned to `/`.
WORKDIR /root

# Clones libevhtp and builds it.
RUN git clone https://github.com/ellzey/libevhtp.git
WORKDIR /root/libevhtp/build
RUN cmake .. && make install
WORKDIR /root/

# Removes the unneeded directory.
RUN rm -rf libevhtp

# Moves files to container and builds CAvatar.
#  Alternative command:
#   RUN git clone https://github.com/JuhaniImberg/CAvatar.git
ADD . /root/CAvatar
WORKDIR /root/CAvatar/build
RUN cmake .. && make

# Sets the first command that is run when the container is started.
WORKDIR /root/CAvatar
ENTRYPOINT ./build/cavatar

# Tells Docker that the container will be listening on port 3002 at runtime.
EXPOSE 3002
