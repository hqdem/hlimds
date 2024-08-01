#!/bin/sh

# SPDX-License-Identifier: Apache-2.0

# Creates new image using Dockerfile and uploads it to the project's container registry.

domain="gitlab.ispras.ru"
port="4567"
container_registry="mvg/utopia-eda"
basename="base"

if [ -z "$UTOPIA_HOME" ]; then
    echo "UTOPIA_HOME env var is not set!"
    exit 1
fi

echo "Please enter the tag (name) of the new image:"
echo "Image tag should have no spaces"
echo "Press \"Enter\" to use default image tag"
read name

if [ -z "$name" ]; then
    name="latest"
    echo "Default image tag used: $name"
fi

if echo "$name" | grep -q ' '; then
    echo "Image tag should be one word!"
    exit 1
fi

echo "Authentication in Container Registry..."
docker login $domain:$port
if [ $? -ne 0 ]; then
    echo "Authentication errors occurred!"
    exit 1
fi
echo "Building base image for Utopia EDA..."
docker build -f ${PWD}/Dockerfile.base -t $domain:$port/$container_registry:$basename ${UTOPIA_HOME}
if [ $? -ne 0 ]; then
    echo "Building errors occurred!"
    exit 1
fi
docker push $domain:$port/$container_registry:$basename
if [ $? -ne 0 ]; then
    echo "Uploading errors occurred!"
    exit 1
fi
echo "Building developers' image for Utopia EDA..."
docker build -f ${PWD}/Dockerfile.dev -t $domain:$port/$container_registry:$name ${UTOPIA_HOME}
if [ $? -ne 0 ]; then
    echo "Building errors occurred!"
    exit 1
fi
docker push $domain:$port/$container_registry:$name
if [ $? -ne 0 ]; then
    echo "Uploading errors occurred!"
    exit 1
fi
echo "Images are successfully updated and uploaded."
echo "Do you want to save image? (Y/n)"
read answ
if [ $(echo "$answ" | grep '^[nнNН]') ]; then
  docker rmi $domain:$port/$container_registry:$basename
  if [ $? -ne 0 ]; then
    echo "Deleting errors occurred!"
    exit 1
  fi
  echo "Image successfully deleted"
  exit 0
fi
echo "Image successfully saved"
