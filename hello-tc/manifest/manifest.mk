.PHONY: manifest clean-manifest upload-download-manifest upload-embed-manifest
.PHONY: upload-download-manifest upload-embed-manifest

manifest: $(MANIFEST_OUT_DIR)/signed-download-tc.suit $(MANIFEST_OUT_DIR)/signed-embed-tc.suit

clean-manifest:
	rm -f $(MANIFEST_OUT_DIR)/signed-download-tc.suit $(MANIFEST_OUT_DIR)/signed-embed-tc.suit
	rm -f $(MANIFEST_OUT_DIR)/download-tc.suit $(MANIFEST_OUT_DIR)/embed-tc.suit
	rm -f $(MANIFEST_OUT_DIR)/embed-tc.suit.tmp
	rm -f $(MANIFEST_OUT_DIR)/download.json $(MANIFEST_OUT_DIR)/embed.json

$(MANIFEST_OUT_DIR)/download-tc.suit: $(MANIFEST_DIR)/manifest.json.in
	sed $(MANIFEST_DIR)/manifest.json.in \
		-e 's!@URI@!$(TC_URI)!' \
		-e 's!@DIGEST@!00112233445566778899aabbccddeeff0123456789abcdeffedcba9876543210!' \
		-e 's!@SIZE@!34768!' \
		>$(MANIFEST_OUT_DIR)/download.json
	suit-tool create -i $(MANIFEST_OUT_DIR)/download.json -o $@

$(MANIFEST_OUT_DIR)/embed-tc.suit: $(MANIFEST_DIR)/manifest.json.in $(TC_BINARY)
	sed $(MANIFEST_DIR)/manifest.json.in \
		-e 's!@URI@!#tc!' \
		-e 's!@DIGEST@!00112233445566778899aabbccddeeff0123456789abcdeffedcba9876543210!' \
		-e 's!@SIZE@!34768!' \
		>$(MANIFEST_OUT_DIR)/embed.json
	suit-tool create -i $(MANIFEST_OUT_DIR)/embed.json -o $(MANIFEST_OUT_DIR)/embed-tc.suit.tmp
	$(MANIFEST_DIR)/add-payload.py \
		--key "#tc" --payload $(TC_BINARY) --in $(MANIFEST_OUT_DIR)/embed-tc.suit.tmp --out $@

$(MANIFEST_OUT_DIR)/signed-download-tc.suit: $(MANIFEST_OUT_DIR)/download-tc.suit
	suit-tool sign -m $< -k $(SUIT_PRIV_KEY) -o $@

$(MANIFEST_OUT_DIR)/signed-embed-tc.suit: $(MANIFEST_OUT_DIR)/embed-tc.suit
	suit-tool sign -m $< -k $(SUIT_PRIV_KEY) -o $@

upload-download-manifest:
	curl $(TAM_URL)/panel/upload \
		-F "file=@$(MANIFEST_OUT_DIR)/signed-download-tc.suit;filename=integrated-payload-manifest.cbor"
	curl $(TAM_URL)/panel/upload \
		-F "file=@$(TC_BINARY);filename=8d82573a-926d-4754-9353-32dc29997f74.ta"

upload-embed-manifest:
	curl $(TAM_URL)/panel/upload \
		-F "file=@$(MANIFEST_OUT_DIR)/signed-embed-tc.suit;filename=integrated-payload-manifest.cbor"
