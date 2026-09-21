#include "load.h"
#include "dbi.h"
#include "pdb.h"
#include "symbols.h"
#include <ctype.h>
#include <string.h>

static bool _pdb_validate_path_component(const char* s) {
    if(!s || !*s) return false;
    if(strstr(s, "..")) return false; // no traversal

    for(const char* p = s; *p; p++) {
        u8 c = (u8)*p;
        if(c < 0x20 || c == 0x7F) return false; // no control chars
    }

    return true;
}

static bool _pdb_try_case_variants(const char* dir, const char* name,
                                   RDScratchBuffer* out) {
    const char* exact = rd_path_join(dir, name);
    if(rd_path_isfile(exact)) {
        rd_scratch_puts(out, exact);
        return true;
    }

    RDScratchBuffer* lower = rd_scratch_create();
    for(const char* p = name; *p; p++)
        rd_scratch_putchar(lower, (char)tolower((u8)*p));

    const char* candidate = rd_path_join(dir, rd_scratch_data(lower));
    bool found = rd_path_isfile(candidate);
    if(found) rd_scratch_puts(out, candidate);

    rd_scratch_destroy(lower);
    return found;
}

static bool _pdb_resolve_local(const char* binary_path, const char* pdb_name,
                               const char* expected_guid, u32 expected_age,
                               RDScratchBuffer* out_path, PDBFile* out_pdb) {
    RDScratchBuffer* candidate_buf = rd_scratch_create();
    bool has_candidate =
        _pdb_try_case_variants(binary_path, pdb_name, candidate_buf);

    if(!has_candidate) {
        rd_scratch_destroy(candidate_buf);
        return false;
    }

    const char* candidate = rd_scratch_data(candidate_buf);
    bool ok = pdb_open(candidate, out_pdb) &&
              pdb_verify(out_pdb, expected_guid, expected_age);

    if(ok)
        rd_scratch_puts(out_path, candidate);
    else
        pdb_close(out_pdb);

    rd_scratch_destroy(candidate_buf);
    return ok;
}

static RDCommandValue pdb_load_execute(RDContext* ctx,
                                       const RDCommandValue* args) {
    const char* pdb_input = args[0].s;
    const char* pdb_guid = args[1].s;
    u32 pdb_age = (u32)args[2].u;

    PDBFile pdb = {0};
    PDBStream dbi_stream = {0};
    PDBSectionHeaderList sections = {0};
    RDScratchBuffer* resolved_buf = NULL;
    const char* pdb_filepath = NULL;
    bool pdb_ready = false;

    if(!_pdb_validate_path_component(pdb_input)) goto cleanup;

    resolved_buf = rd_scratch_create();
    pdb_filepath = pdb_input;

    if(rd_path_isfile(pdb_filepath)) {
        pdb_ready =
            pdb_open(pdb_filepath, &pdb) && pdb_verify(&pdb, pdb_guid, pdb_age);
        if(!pdb_ready) pdb_close(&pdb);
    }
    else {
        const char* basename = rd_path_filename(pdb_input);
        const char* binary_path = rd_get_working_dir(ctx);
        pdb_ready = _pdb_resolve_local(binary_path, basename, pdb_guid, pdb_age,
                                       resolved_buf, &pdb);
    }

    if(!pdb_ready) {
        RD_LOG_FAIL("PDB not found or verification failed: %s", pdb_input);
        goto cleanup;
    }

    RD_LOG_INFO("loading PDB '%s'", rd_path_isfile(pdb_input)
                                        ? pdb_input
                                        : rd_scratch_data(resolved_buf));

    if(!pdb_read_stream_by_index(&pdb, PDB_STREAM_DBI, &dbi_stream))
        goto cleanup;

    PDBDbiHeader dbi;
    if(!pdb_read_dbi_header(&dbi_stream, &dbi)) goto cleanup;

    PDBDbiDbgHeader dbg;
    if(!pdb_read_dbi_dbg_header(&dbi_stream, &dbi, &dbg)) goto cleanup;
    pdb_stream_destroy(&dbi_stream);

    if(!pdb_read_section_headers(&pdb, dbg.SectionHdr, &sections)) goto cleanup;

    pdb_apply_symbols(ctx, &pdb, dbi.SymRec, &sections);

cleanup:
    rd_scratch_destroy(resolved_buf);
    pdb_section_header_list_destroy(&sections);
    pdb_stream_destroy(&dbi_stream);
    pdb_close(&pdb);
    return (RDCommandValue){0};
}

static const RDCommandParam PDB_LOAD_PARAMS[] = {
    {RD_CMDARG_STRING, "path"},
    {RD_CMDARG_STRING, "guid"},
    {RD_CMDARG_UINT, "age"},
    {RD_CMDARG_VOID},
};

const RDCommandPlugin PDB_LOAD = {
    .id = "pdb_load",
    .name = "Load PDB File",
    .params = PDB_LOAD_PARAMS,
    .execute = pdb_load_execute,
};
