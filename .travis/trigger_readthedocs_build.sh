#!/bin/sh

# READTHEDOCS_TOKEN can be obtained from configuring a generic webhook.
# See https://docs.readthedocs.io/en/stable/webhooks.html#using-the-generic-api-integration

curl \
  --request POST \
  --data 'branches=master' \
  --data "token=$READTHEDOCS_TOKEN" \
  'https://readthedocs.org/api/v2/webhook/scipp/94775/'
