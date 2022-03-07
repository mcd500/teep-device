cat docs/overview_of_teep-device.md docs/building_with_docker.md > README.md
sed -i 's/@image html /![](/g' README.md
sed -i '/^\!\[\]/ s/$/)/' README.md