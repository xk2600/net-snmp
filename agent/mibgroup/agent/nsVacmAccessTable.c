/*
 * Note: this file originally auto-generated by mib2c using
 *  : mib2c.iterate.conf,v 5.17 2005/05/09 08:13:45 dts12 Exp $
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-features.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/library/vacm.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "nsVacmAccessTable.h"

netsnmp_feature_require(table_iterator_insert_context)
netsnmp_feature_require(check_vb_storagetype)
netsnmp_feature_require(check_vb_type_and_max_size)

/** Initializes the nsVacmAccessTable module */
void
init_register_nsVacm_context(const char *context)
{
    /*
     * Initialize the nsVacmAccessTable table by defining its
     *   contents and how it's structured
     */
    const oid nsVacmAccessTable_oid[]   = { 1,3,6,1,4,1,8072,1,9,1 };
    netsnmp_handler_registration    *reg;
    netsnmp_iterator_info           *iinfo;
    netsnmp_table_registration_info *table_info;

#ifndef NETSNMP_NO_WRITE_SUPPORT
    reg = netsnmp_create_handler_registration(
        "nsVacmAccessTable", nsVacmAccessTable_handler,
        nsVacmAccessTable_oid, OID_LENGTH(nsVacmAccessTable_oid),
        HANDLER_CAN_RWRITE);
#else /* !NETSNMP_NO_WRITE_SUPPORT */
    reg = netsnmp_create_handler_registration(
        "nsVacmAccessTable", nsVacmAccessTable_handler,
        nsVacmAccessTable_oid, OID_LENGTH(nsVacmAccessTable_oid),
        HANDLER_CAN_RONLY);
#endif /* !NETSNMP_NO_WRITE_SUPPORT */

    table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(table_info,
                                     ASN_OCTET_STR, /* index: vacmGroupName */
                                     ASN_OCTET_STR, /* index: vacmAccessContextPrefix */
                                     ASN_INTEGER,   /* index: vacmAccessSecurityModel */
                                     ASN_INTEGER,   /* index: vacmAccessSecurityLevel */
                                     ASN_OCTET_STR, /* index: nsVacmAuthType */
                                     0);
    table_info->min_column = COLUMN_NSVACMCONTEXTMATCH;
    table_info->max_column = COLUMN_NSVACMACCESSSTATUS;

    iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
    iinfo->get_first_data_point = nsVacmAccessTable_get_first_data_point;
    iinfo->get_next_data_point  = nsVacmAccessTable_get_next_data_point;
    iinfo->table_reginfo = table_info;

    if ( context && context[0] )
        reg->contextName = strdup(context);

    netsnmp_register_table_iterator2(reg, iinfo);
}

void
init_nsVacmAccessTable(void)
{
    init_register_nsVacm_context("");
}


/*
 * Iterator hook routines
 */
static int nsViewIdx;   /* This should really be handled via the 'loop_context'
                           parameter, but it's easier (read lazier) to use a
                           global variable as well.  Bad David! */

netsnmp_variable_list *
nsVacmAccessTable_get_first_data_point(void **my_loop_context,
                                       void **my_data_context,
                                       netsnmp_variable_list *put_index_data,
                                       netsnmp_iterator_info *mydata)
{
    vacm_scanAccessInit();
    *my_loop_context = vacm_scanAccessNext();
    nsViewIdx = 0;
    return nsVacmAccessTable_get_next_data_point(my_loop_context,
                                                 my_data_context,
                                                 put_index_data, mydata);
}

netsnmp_variable_list *
nsVacmAccessTable_get_next_data_point(void **my_loop_context,
                                      void **my_data_context,
                                      netsnmp_variable_list *put_index_data,
                                      netsnmp_iterator_info *mydata)
{
    struct vacm_accessEntry *entry =
        (struct vacm_accessEntry *) *my_loop_context;
    netsnmp_variable_list *idx;
    int len;
    char *cp;

newView:
    idx = put_index_data;
    if ( nsViewIdx == VACM_MAX_VIEWS ) {
        entry = vacm_scanAccessNext();
        nsViewIdx = 0;
    }
    if (entry) {
        len = entry->groupName[0];
        snmp_set_var_value(idx, (u_char *)entry->groupName+1, len);
        idx = idx->next_variable;
        len = entry->contextPrefix[0];
        snmp_set_var_value(idx, (u_char *)entry->contextPrefix+1, len);
        idx = idx->next_variable;
        snmp_set_var_value(idx, (u_char *)&entry->securityModel,
                           sizeof(entry->securityModel));
        idx = idx->next_variable;
        snmp_set_var_value(idx, (u_char *)&entry->securityLevel,
                           sizeof(entry->securityLevel));
        /*
         * Find the next valid authType view - skipping unused entries
         */
        idx = idx->next_variable;
        for (; nsViewIdx < VACM_MAX_VIEWS; nsViewIdx++) {
            if ( entry->views[ nsViewIdx ][0] )
                break;
        }
        if ( nsViewIdx == VACM_MAX_VIEWS )
            goto newView;
        cp = se_find_label_in_slist(VACM_VIEW_ENUM_NAME, nsViewIdx++);
        DEBUGMSGTL(("nsVacm", "nextDP %s:%s (%d)\n", entry->groupName+1, cp, nsViewIdx-1));
        snmp_set_var_value(idx, (u_char *)cp, strlen(cp));
        idx = idx->next_variable;
        *my_data_context = (void *) entry;
        *my_loop_context = (void *) entry;
        return put_index_data;
    } else {
        return NULL;
    }
}


/** handles requests for the nsVacmAccessTable table */
int
nsVacmAccessTable_handler(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info *reqinfo,
                          netsnmp_request_info *requests)
{

    netsnmp_request_info *request;
    netsnmp_table_request_info *table_info;
    netsnmp_variable_list      *idx;
    struct vacm_accessEntry    *entry;
    char atype[20];
    int  viewIdx, ret;
    char *gName, *cPrefix;
    int  model, level;

    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request = requests; request; request = request->next) {
            entry = (struct vacm_accessEntry *)
                netsnmp_extract_iterator_context(request);
            table_info = netsnmp_extract_table_info(request);

            /* Extract the authType token from the list of indexes */
            idx = table_info->indexes->next_variable->next_variable->next_variable->next_variable;
            memset(atype, 0, sizeof(atype));
            strncpy(atype, (char *)idx->val.string, idx->val_len);
            viewIdx = se_find_value_in_slist(VACM_VIEW_ENUM_NAME, atype);
            DEBUGMSGTL(("nsVacm", "GET %s (%d)\n", idx->val.string, viewIdx));

            if (!entry)
                continue;

            switch (table_info->colnum) {
            case COLUMN_NSVACMCONTEXTMATCH:
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           entry->contextMatch);
                break;
            case COLUMN_NSVACMVIEWNAME:
                snmp_set_var_typed_value(request->requestvb, ASN_OCTET_STR,
                               (u_char *)entry->views[ viewIdx ],
                                  strlen(entry->views[ viewIdx ]));
                break;
            case COLUMN_VACMACCESSSTORAGETYPE:
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           entry->storageType);
                break;
            case COLUMN_NSVACMACCESSSTATUS:
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           entry->status);
                break;
            }
        }
        break;

#ifndef NETSNMP_NO_WRITE_SUPPORT
        /*
         * Write-support
         */
    case MODE_SET_RESERVE1:
        for (request = requests; request; request = request->next) {
            entry = (struct vacm_accessEntry *)
                netsnmp_extract_iterator_context(request);
            table_info = netsnmp_extract_table_info(request);
            ret = SNMP_ERR_NOERROR;

            switch (table_info->colnum) {
            case COLUMN_NSVACMCONTEXTMATCH:
                ret = netsnmp_check_vb_int_range(request->requestvb, 1, 2);
                break;
            case COLUMN_NSVACMVIEWNAME:
                ret = netsnmp_check_vb_type_and_max_size(request->requestvb,
                                                         ASN_OCTET_STR,
                                                         VACM_MAX_STRING);
                break;
            case COLUMN_VACMACCESSSTORAGETYPE:
                ret = netsnmp_check_vb_storagetype(request->requestvb,
                          (/*entry ? entry->storageType :*/ SNMP_STORAGE_NONE));
                break;
            case COLUMN_NSVACMACCESSSTATUS:
                /*
                 * The usual 'check_vb_rowstatus' call is too simplistic
                 *   to be used here.  Because we're implementing a table
                 *   within an existing table, it's quite possible for a
                 *   the vacmAccessTable entry to exist, even if this is
                 *   a "new" nsVacmAccessEntry.
                 *
                 * We can check that the value being assigned is suitable
                 *   for a RowStatus syntax object, but the transition
                 *   checks need to be done explicitly.
                 */
                ret = netsnmp_check_vb_rowstatus_value(request->requestvb);
                if ( ret != SNMP_ERR_NOERROR )
                    break;

                /*
                 * Extract the authType token from the list of indexes
                 */
                idx = table_info->indexes->next_variable->next_variable->next_variable->next_variable;
                memset(atype, 0, sizeof(atype));
                strncpy(atype, (char *)idx->val.string, idx->val_len);
                viewIdx = se_find_value_in_slist(VACM_VIEW_ENUM_NAME, atype);
                if ( viewIdx < 0 ) {
                    ret = SNMP_ERR_NOCREATION;
                    break;
                }
                switch ( *request->requestvb->val.integer ) {
                case RS_ACTIVE:
                case RS_NOTINSERVICE:
                    /* Check that this particular view is already set */
                    if ( !entry || !entry->views[viewIdx][0] )
                        ret = SNMP_ERR_INCONSISTENTVALUE;
                    break;
                case RS_CREATEANDWAIT:
                case RS_CREATEANDGO:
                    /* Check that this particular view is not yet set */
                    if ( entry && entry->views[viewIdx][0] )
                        ret = SNMP_ERR_INCONSISTENTVALUE;
                    break;
                }
                break;
            } /* switch(colnum) */
            if ( ret != SNMP_ERR_NOERROR ) {
                netsnmp_set_request_error(reqinfo, request, ret);
                return SNMP_ERR_NOERROR;
            }
        }
        break;

    case MODE_SET_RESERVE2:
        for (request = requests; request; request = request->next) {
            entry = (struct vacm_accessEntry *)
                netsnmp_extract_iterator_context(request);
            table_info = netsnmp_extract_table_info(request);

            switch (table_info->colnum) {
            case COLUMN_NSVACMACCESSSTATUS:
                switch (*request->requestvb->val.integer) {
                case RS_CREATEANDGO:
                case RS_CREATEANDWAIT:
                    if (!entry) {
                         idx = table_info->indexes; gName = (char*)idx->val.string;
                         idx = idx->next_variable;  cPrefix = (char*)idx->val.string;
                         idx = idx->next_variable;  model = *idx->val.integer;
                         idx = idx->next_variable;  level = *idx->val.integer;
                         entry = vacm_createAccessEntry( gName, cPrefix, model, level );
                         entry->storageType = ST_NONVOLATILE;
                         netsnmp_insert_iterator_context(request, (void*)entry);
                    }
                }
            }
        }
        break;

    case MODE_SET_FREE:
    case MODE_SET_UNDO:
        /* XXX - TODO */
        break;

    case MODE_SET_ACTION:
        /* ??? Empty ??? */
        break;

    case MODE_SET_COMMIT:
        for (request = requests; request; request = request->next) {
            entry = (struct vacm_accessEntry *)
                netsnmp_extract_iterator_context(request);
            table_info = netsnmp_extract_table_info(request);
            if ( !entry )
                continue;  /* Shouldn't happen */

            /* Extract the authType token from the list of indexes */
            idx = table_info->indexes->next_variable->next_variable->next_variable->next_variable;
            memset(atype, 0, sizeof(atype));
            strncpy(atype, (char *)idx->val.string, idx->val_len);
            viewIdx = se_find_value_in_slist(VACM_VIEW_ENUM_NAME, atype);

            switch (table_info->colnum) {
            case COLUMN_NSVACMCONTEXTMATCH:
                entry->contextMatch = *request->requestvb->val.integer;
                break;
            case COLUMN_NSVACMVIEWNAME:
                memset( entry->views[viewIdx], 0, VACMSTRINGLEN );
                memcpy( entry->views[viewIdx], request->requestvb->val.string,
                                               request->requestvb->val_len);
                break;
            case COLUMN_VACMACCESSSTORAGETYPE:
                entry->storageType = *request->requestvb->val.integer;
                break;
            case COLUMN_NSVACMACCESSSTATUS:
                switch (*request->requestvb->val.integer) {
                case RS_DESTROY:
                    memset( entry->views[viewIdx], 0, VACMSTRINGLEN );
                    break;
                }
                break;
            }
        }
        break;
#endif /* !NETSNMP_NO_WRITE_SUPPORT */
    }
    return SNMP_ERR_NOERROR;
}
