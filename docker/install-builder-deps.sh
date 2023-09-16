#!/usr/bin/env bash

set -e -x

apt-get update

apt-get install -y \
  gpg \
  wget \
  software-properties-common

# Make LLVM repos visible for `apt`.
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
add-apt-repository -s "deb https://apt.llvm.org/focal/ llvm-toolchain-focal-12 main"

# Make CMake repos visible for `apt`.
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ focal main" | tee /etc/apt/sources.list.d/kitware.list

# Make Postgres repos visible for `apt`.
wget -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add -
echo "deb https://apt.postgresql.org/pub/repos/apt/ `lsb_release -cs`-pgdg main" | tee /etc/apt/sources.list.d/pgdg.list

apt-get update

# Don't speak to me
export DEBIAN_FRONTEND=noninteractive

apt-get install -y \
  clang-12

apt-get install -y vim

apt-get install -y \
  make \
  cmake
#  python3 \
#  python3-pip

apt-get install -y \
  build-essential \
  libboost-all-dev \
  libasio-dev

apt-get install -y \
 postgresql-13 \
 libpq-dev \
 postgresql-server-dev-all