# Getting started with Docker

In this guide the instruction on how to build and run Utopia EDA with Docker
is described.
The guide was checked on Ubuntu 20.04 operating system.

## Table of contents

- [Installation](#docker-installation)
- [Authentication](#docker-authentication)
- [Dockerfile](#image-building)
  - [Image building](#building-from-dockerfile)
  - [Image modifying](#modifying-docker-image)
- [Uploading images](#uploading-to-gitlab-container-registry)
- [Using Docker](#using-docker)
  - [Main commands](#main-commands)
  - [Data exchanging](#data-exchanging)
    - [Docker Volumes](#volumes)
    - [Shared directory](#shared-directory)
- [Examples](#example-of-usage)
  - [Container running](#run-utopia-eda-image-as-a-container)
  - [Image modifying](#update-utopia-eda-image-and-upload-it-to-gitlab-container-registry)

---

## Docker installation

To install Docker, do following:

```console
    sudo apt update
    sudo apt install -y apt-transport-https ca-certificates curl software-properties-common
    curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
    sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu focal stable"
    sudo apt update
    apt-cache policy docker-ce
    sudo apt install docker-ce
    sudo setfacl --modify user:<user name or ID>:rw /var/run/docker.sock
```

*Run* `whoami` *or* `id` *to get your username or id.*

---

## Docker authentication

If you want to share and upload your images to GitLab Container Registry or
DockerHub, do authentication.

If you want to pull and push images to GitLab Container Registry at gitlab.ispras.ru, login as follows:

```console
    docker login gitlab.ispras.ru:4567
    *Enter GitLab login & password, if needed*
```

If you want to share your images on DockerHub, then:

Create account on [Docker Hub](https://hub.docker.com "Click here to go to the website")

```console
    docker login
*Enter DockerHub login & password, if needed*
```

--

## Image building

In the following sections \<image name\> has this format:
`name:tag`
Tag is necessary.

In this section instruction on how to build and update Docker image using
Dockerfile is described. Use this to install new utilities and libraries on
image to work with Utopia EDA.

### Building from `Dockerfile`

To build new image from Dockerfile, use:

```console
    docker build -t <image name> <path/to/dir/with/Dockerfile>
```

*Example:*

```console
    docker build -t new_image:test /home/user/workdir/utopia-eda
```

Run `docker images` to check images on your machine.
Run `docker rmi <image name/id`> to delete the image.

### Modifying image

The Docker image can be updated by modifying the Dockerfile and then
rebuilding the image.

To install utilities/libraries or create/modify files at the image, use:

```text
    RUN <command>
```

*Example:*

```text
    RUN touch file.txt
    RUN apt-get update && apt install -y git
```

*Do not use "sudo" for* `RUN` *commands*.
*Use "install" with* -y *key*.

To change directory, use:

```text
    WORKDIR /path/to/dir
```

*Example:*

```text
    WORKDIR /workdir/CUDD
```

To copy file from your host to image, use:

```text
    COPY /path/on/host /path/on/image
```

*Example:*

```text
    COPY /home/user/yosys /workdir
```

---

## Uploading to Gitlab Container Registry

To upload your images to GitLab Container Registry or DockerHub, use:

```console
    docker push <image name>
```

Command `docker push` does not work when images are named arbitrarily.
To push image to GitLab Container Registry, name it as follows:

```text
    gitlab.ispras.ru:4567/<GitLab user login>/utopia-eda/<image name>
```

*Go to your GitLab profile and check [login settings]("https://gitlab.ispras.ru/my_login")
to get your GitLab user login.*

To push image to GitLab Container Registry use `docker push <image name>`.

*Example:*

```console
    docker build -t gitlab.ispras.ru:4567/your_login/utopia-eda/your_imagename /path/to/Dockerfile
    docker push gitlab.ispras.ru:4567/your_login/utopia-eda/your_imagename
```

Enter GitLab login & password, if needed.

Go to `GitLab Project -> Deploy -> Container Registry` to see registry's contents.

---

## Using Docker

### Main commands

Here the main commands for Docker are described.

| Command | Description | Notes |
|---------|-------------|-------|
| `docker pull <image name>` | Download image | |
| `docker images` | List of images on your machine | |
| `docker rmi <image name/id>` | Remove image | |
| `docker run -it --name <container name> <image name>` | Run and open the image. Make container from image | Press Ctrl+D to exit from container terminal |
| `docker ps -a` | List of containers | Container status (started or stopped) |
| `docker rm <container name/id>` | Remove container | |
| `docker commit <container id> <image name>` | Make image from container | Save the state of the container |
| `docker start/stop <container name/id>` | Run/stop backround container | Only running containers can be opened |
| `docker exec -it <container name/id> /bin/bash` | Open a running container | Press Ctrl+D to exit from container terminal |
| `docker exec -it <container name/id> ls -alRF /` | To check info about container's directories | Work only with running containers |
| `docker push gitlab.ispras.ru:4567/<GitLab username>/utopia-eda/<image name>` | Push image to GitLab Container Registry | To push image to your Utopia EDA GitLab Container Registry, name image: `gitlab.ispras.ru:4567/<GitLab username>/utopia-eda/<image name>` |
| `docker tag <image name> <new image name>` | Make copy of image with new name | |
| `docker inspect <image name>` | To check info about image | In the section *GraphDriver->Data* path on host where the image data is stored |

---

### Data exchanging

After running the image you get a container.
There are several ways to make a data exchange between host and container.

- Volumes
- Shared directory

#### Volumes

Docker volume is a host directory which is shared between OS and Docker
container. You can get data from volume after container finished.

To create volume:

```console
    docker volume create <volume name>
```

To connect volume with container:

```console
    docker run -v <volume name>:/path/in/container -it --name <container name> <image name>
```

After that, all changes in */path/in/container* will be available in volume
after container ceases to operate.

To check information about volume, use:

```console
    docker volume inspect <volume name>
```

To list all volumes, use:

```console
    docker volume ls
```

To remove volume, use:

```console
    docker volume rm <volume name>
```

*Example:*

```console
    docker volume create vol
    docker volume inspect vol
```

*Output:*

```text
    [
      {
         "CreatedAt": "2023-11-16T16:18:21+03:00",
         "Driver": "local",
         "Labels": null,
         "Mountpoint": "/var/lib/docker/volumes/vol/_data",
         "Name": "vol",
         "Options": null,
         "Scope": "local"
      }
    ]
```

```console
    cp /home/workdir/test_data/example.ril /var/lib/docker/volumes/vol/_data
    docker run -v vol:/workdir/utopia-eda/io_data -it --name container_for_running_utopia-eda image-with-utopia-eda
```

After that, use container terminal:

```console
    export UTOPIA_HOME=workdir/utopia-eda
    ${UTOPIA_HOME}/build/src/umain rtl io_data/example.ril
    cp ${UTOPIA_HOME}/output ${UTOPIA_HOME}/io_data
```

Press `Ctrl + D` to exit from container terminal and stop the container.

Then you can check changes in volume on the host.

#### Shared directory

You can embed host directory in the container file system.

For sharing host directory with container, use:

```console
    docker run -v /path/in/host:/path/in/container -it --name <container name> <image name>
```

This command embeds */path/in/host* in */path/in/container*. All changes made
on the host will be available in container and vice versa.

*Example:*

```console
    docker run -v path/on/host/to/input_data:/workdir/utopia-eda/io_data -it --name container_for_running_utopia-eda image-with-utopia-eda
```

After that, use container terminal:

```console
    export UTOPIA_HOME=workdir/utopia-eda
    ${UTOPIA_HOME}/build/src/umain rtl io_data/example.ril
    cp ${UTOPIA_HOME}/output ${UTOPIA_HOME}/io_data
```

Press `Ctrl + D` to exit from container terminal and stop the container.

Then you can check changes in `path/on/host/to/input_data` on the host.

---

## Example of usage

This section provides examples of how to use Docker for Utopia EDA:

- 1st example shows how to download an image and run Utopia EDA;

- 2nd example shows how to update image and upload it to the registry.

### Run Utopia EDA image as a container

After installing Docker, you can download image from DockerHub
using command `docker pull`. Use [Docker authentication](#docker-authentication).

This example includes the following steps:

- Download the image
- Create volume to exchange data with container
- Run container from the image and connect it to volume
- Run Utopia EDA on input data that are stored at the volume
- Exit from container and save the result in volume
- Delete container and check that all changes are available in the volume

Terminal:

```console
    docker pull gitlab.ispras.ru:4567/mvg/utopia-eda/utopia-eda:build
    docker volume create vol
    docker volume inspect vol
```

---

Output:

```text
    [
      {
         "CreatedAt": "2023-11-16T16:18:21+03:00",
         "Driver": "local",
         "Labels": null,
         "Mountpoint": "/var/lib/docker/volumes/vol/_data",
         "Name": "vol",
         "Options": null,
         "Scope": "local"
      }
    ]
```

---

```console
    cp example.ril /var/lib/docker/volumes/vol/_data
    docker run -v vol:/workdir/utopia-eda/io_data -it --name runutopia gitlab.ispras.ru:4567/mvg/utopia-eda/utopia-eda:build
```

Container terminal:

```console
    cd workdir/utopia-eda
    export UTOPIA_HOME=workdir/utopia-eda
    rm -rf ${UTOPIA_HOME}/output
    ./build/src/umain rtl /workdir/utopia-eda/io_data/example.ril --lec 0 > ./io_data/output_data.txt
```

Press `Ctrl + D` to exit from container terminal and stop the container.

Terminal:

```console
    docker rm runutopia
    cat /var/lib/docker/volumes/vol/_data/output_data.txt
```

---

### Update Utopia EDA image and upload it to GitLab Container Registry

In the root of Utopia EDA there is a Dockerfile that can be used to update image.
For updating image, put neccesary commands at the end of Dockerfile
before `CMD["/bin/bash"]`.

In this example, we edit Dockerfile so that:

- the files in current directory will be copied to `/workdir` on image;
- change the current directory on image to `/workdir/lib`;
- install valgrind.

After all, we build the image with name
`gitlab.ispras.ru:4567/mvg/utopia-eda/utopia-eda:updated` and push it
to project's Container Registry.

Dockerfile:

```text
    ...
    COPY . /workdir
    WORKDIR /workdir/lib
    RUN apt-get update && apt install -y valgrind
    CMD["/bin/bash"]
```

Terminal:

```console
    docker build -t gitlab.ispras.ru:4567/mvg/utopia-eda/utopia-eda:updated /home/user/utopia-eda
    docker push gitlab.ispras.ru:4567/mvg/utopia-eda/utopia-eda:updated
```
