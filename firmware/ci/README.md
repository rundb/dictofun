# Dictofun CI setup

Collection of scripts related to CI operation for Dictofun project.

## Usage

1. Build Docker image:

```
docker build -t dictofun_builder .
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
