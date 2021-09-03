#include <stdio.h>
#include <string.h>
#include "nocbor.h"
#include "teesuit.h"
#include "teelog.h"

void suit_context_init(suit_context_t *p)
{
    memset(p, 0, sizeof *p);
}

bool suit_context_add_envelope(suit_context_t *p, nocbor_range_t envelope_bstr)
{
    if (p->n_manifest == SUIT_MANIFEST_MAX) return false;
    suit_manifest_t *manifest = &p->manifests[p->n_manifest];
    if (!suit_envelope_get_manifest(envelope_bstr, manifest)) return false;

    p->n_manifest++;
    return true;
}

suit_manifest_t *suit_context_get_root_manifest(suit_context_t *p)
{
    if (p->n_manifest == 0) return NULL;
    return &p->manifests[0];
}
