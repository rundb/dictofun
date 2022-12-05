# Dictofun CI setup

Collection of scripts related to CI operation for Dictofun project.

## Usage

1. Build Docker image. If image is build for ARM hosts, add argument `--build-arg HOST_ARCHITECTURE="armv7"`:

```
docker build -t dictofun_builder .
```
OR
```
docker build -t dictofun_builder --build-arg HOST_ARCHITECTURE="arm" .
```

2. (manual build) Step one folder up and launch docker:

```
cd ../
docker run -it -v $(pwd):/code dictofun_builder
```

3. (manual build) Inside the docker container enter `/code/build` folder and launch build (might be necessary to create `build` folder first):

```
mkdir -p /code/build
cd /code/build
../ci/build_dictofun.sh
```

2. (automated build) Launch following command from `firmware` folder

```
docker run -v $(pwd):/code  dictofun_builder /bin/bash -c "mkdir -p /code/build && cd /code/build && ../ci/build_dictofun.sh"
```