#!/bin/bash

set -x

# Obtain DOCKER_BUILD_TRIGGER_URL from the Builds > Edit > Build Triggers
# option in the Docker Cloud web UI.

curl \
  --head \
  --request POST \
  --output /dev/null \
  --write-out '%{http_code}' \
  "$DOCKER_BUILD_TRIGGER_URL"
