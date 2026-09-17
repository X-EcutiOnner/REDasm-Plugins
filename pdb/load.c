#include "load.h"
#include "dbi.h"
#include "pdb.h"
#include "symbols.h"

static RDCommandValue pdb_load_execute(RDContext* ctx,
                                       const RDCommandValue* args) {
    const char* pdb_filepath = args[0].s;
    const char* pdb_guid = args[1].s;
    u32 pdb_age = (u32)args[2].u;

    PDBFile pdb = {0};
    PDBStream dbi_stream = {0};
    PDBSectionHeaderList sections = {0};

    if(!pdb_open(pdb_filepath, &pdb)) goto cleanup;
    if(!pdb_verify(&pdb, pdb_guid, pdb_age)) goto cleanup;

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
