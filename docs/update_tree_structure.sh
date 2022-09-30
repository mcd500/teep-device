# Tree file
DOCS_FOLDER="docs"
TREE_MD_FILE_TEMPLATE="${DOCS_FOLDER}/tree_view_dir_template.md"
TREE_MD_FILE="${DOCS_FOLDER}/tree_view_dir.md"
UPD_TAG="DYNAMIC_SOURCE_UPDATE_TAG"

# Copy the 
cp ${TREE_MD_FILE_TEMPLATE} ${TREE_MD_FILE}

if grep -Fxq ${UPD_TAG} ${TREE_MD_FILE};
 then
 	# Remove the tag
 	 sed -i "s/${UPD_TAG}/\n/g" ${TREE_MD_FILE}

	# Append the tree structure to end of the script
	echo '```sh' >> ${TREE_MD_FILE}
	# tree -d -L 2 ../ >> ${TREE_MD_FILE}
	# ls -R -1 | grep ":$" | sed -e 's/:$//' -e 's/[^-][^\/]*\//---/g' -e 's/^/   /' -e 's/--/|/'  >> ${TREE_MD_FILE}
	find . -maxdepth 2 -type d -not -path '*/.*' | sed -e "s/[^-][^\/]*\//  |/g" -e "s/|\([^ ]\)/|-\1/"  >> ${TREE_MD_FILE}
	echo '```' >> ${TREE_MD_FILE}
fi
