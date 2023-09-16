#!/usr/bin/env bash

set -e -x

apt-get update

apt-get install -y \
  gpg \
  wget \
  software-properties-common

  wget -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add -
  echo "deb https://apt.postgresql.org/pub/repos/apt/ `lsb_release -cs`-pgdg main" | tee /etc/apt/sources.list.d/pgdg.list

  apt-get update

  # Don't speak to me
  export DEBIAN_FRONTEND=noninteractive

  apt-get install -y \
    postgresql-client-13