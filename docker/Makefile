# DOCKER TASKS
# Build the container

DOCKER_REPO_USER = facebookincubator
DOCKER_IMAGE_NAME = docker-ffmpeg-rush
DOCKER_IMAGE_VERSION = 1.1

# Import docker repo (docker hub creds)

.PHONY: build tag_latest last_built_date shell

build: Dockerfile
	docker build -t $(DOCKER_REPO_USER)/$(DOCKER_IMAGE_NAME):$(DOCKER_IMAGE_VERSION) --rm .

tag_latest:
	docker tag $(DOCKER_REPO_USER)/$(DOCKER_IMAGE_NAME):$(DOCKER_IMAGE_VERSION) $(DOCKER_REPO_USER)/$(DOCKER_IMAGE_NAME):latest

last_built_date:
	docker inspect -f '{{ .Created }}' $(DOCKER_REPO_USER)/$(DOCKER_IMAGE_NAME):$(DOCKER_IMAGE_VERSION)

shell:
	docker run --rm -it --cap-add=NET_ADMIN --entrypoint /bin/bash $(DOCKER_REPO_USER)/$(DOCKER_IMAGE_NAME):$(DOCKER_IMAGE_VERSION)

default: build
