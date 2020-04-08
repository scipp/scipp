#!/bin/bash

set -ex

owner='scipp'
package='scipp'
label='dev'

one_month_ago_ms="$(date -d 'last month' '+%s')000"

old_packages=$(curl \
                 -X GET \
                 --header 'Accept: application/json' \
                 --header "Authorization: token $ANACONDA_CLOUD_TOKEN" \
                 "https://api.anaconda.org/package/$owner/$package/files" \
               | jq -r ".[] | select(.labels == [\"dev\"] and .attrs.timestamp < $one_month_ago_ms) | .full_name")

while IFS= read -r full_name; do
  curl \
    -X DELETE \
    --header 'Accept: application/json' \
    --header "Authorization: token $ANACONDA_CLOUD_TOKEN" \
    "https://api.anaconda.org/dist/$full_name"
done <<< "$old_packages"
